/* 
 * This program is used to read the performance counters of a multithreaded task, 
 * and reports the PMU readings of each thread, or of each core
 * Usage: use option "-h" or see function "usage".
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */ 

#define _GNU_SOURCE             // for cpu affinity
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <err.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <string.h>
#include <time.h>
#include <sched.h>
#include <pthread.h>

#include <common_toolx.h>

#include "pfm_operations.h"
#include "pfm_trigger.h"

#ifdef __PFM_MULTI_DEBUG__
#define DPRINTF(fmt, ...) \
  do { fprintf(stderr, "pfm_multi debug: " fmt , ## __VA_ARGS__);} while (0);
#else
#define DPRINTF(fmt, ...) \
  do {} while(0);
#endif

#define DEFAULT_PMU_EVENTS "PERF_COUNT_HW_CPU_CYCLES,PERF_COUNT_HW_INSTRUCTIONS"

typedef struct __options{
	long print_interval;
	pfm_operations_options_t pfm_options;
	char * events;
	int is_sys_wide_mon;
	char * cores;
	int use_trigger;
	void *trigger_info;
	int use_dummy_thread;
	int dummy_result; // store the results from dummy threads
	int * run_cores; // cores to run application threads
	int run_core_cnt; // the number of run cores
}options_t;

options_t options;

int enable_logging;

int child_continue(pid_t tid, unsigned long sig)
{
	int ret;
	
	ret = ptrace(PTRACE_CONT, tid, NULL, (void *)sig);
	if (ret == -1){
		warn("cannot restart thread [%d], error:", tid);
	}
	return ret;
}

/*
 * Wrapper function for getting the new thread id, return value -1 means 
 * function failed
 */
pid_t get_new_thread_id(pid_t tid)
{
	int ret;
	pid_t new_tid = -1;
	
	ret = ptrace (PTRACE_GETEVENTMSG, tid, NULL, (void *) &new_tid);
	if(ret == -1){
		warn("cannot get the new thread id created by thread [%d], "
		     "error:", tid);
	}
	
	return new_tid;
}

int child(char ** args)
{
	
	// execute the requested command
	execvp(args[0], args);
	errx(1, "cannot exec: %s\n", args[0]);
	/* not reached */
}

int monitor_new_thread(pid_t tid, int flags, options_t *options)
{
	if(tid == -1)
		return 1;
	return pfm_attach_thread(tid, options->events,flags, 
				 &(options->pfm_options));
}

int pin_thread(pid_t tid, int core_id)
{
	cpu_set_t cpuset;
	int ret_val;
	
	CPU_ZERO(&cpuset);
	CPU_SET(core_id, &cpuset);

	ret_val = sched_setaffinity(tid, sizeof(cpuset), &cpuset);
	DPRINTF("Pin thread %d to core %d with result %d\n", tid, core_id,
		ret_val);
	
	return ret_val;
}

/*
 * handle the sigtrap signal for a child. 
 *
 * Input parameters:
 *   tid: the tid that generates the sigtrap
 *   status: the status of the thread
 *   flags: performance monitoring flags
 *   run_core_idx: the run core index to which the new thread is pinned
 * Return value:
 *   the signal to send to child
 */
int handle_sigtrap(int tid, int status, int flags, int *run_core_idx)
{
	int new_tid = -1;
	int ret = 0;
	int event;
	
	/*
	 * extract event code from status (should be in
	 * some macro)
	 */
	event = status >> 16;
	switch(event) {
	case PTRACE_EVENT_FORK:
		new_tid = get_new_thread_id(tid);
		DPRINTF("FORK called by thread [%d], new process created with "
			"pid [%d]\n", tid, new_tid);
	
		break;
	case PTRACE_EVENT_CLONE:
		new_tid = get_new_thread_id(tid);
		DPRINTF("CLONE called by thread [%d], new thread created with "
			"tid [%d]\n", tid, new_tid);
		break;
	case PTRACE_EVENT_VFORK:
		new_tid = get_new_thread_id(tid);
		DPRINTF("VFORK called by thread [%d], new process created with "
			"pid [%d]\n", tid, new_tid);
		break;
	case PTRACE_EVENT_EXEC:
		DPRINTF("EXEC called by thread [%d]\n", tid);
		break;
	case  0:
		DPRINTF("Event 0 by thread [%d]\n", tid);
		break;
	default: 
		DPRINTF("Got unknown event %d, event not handled\n", event);
	}
	

	if(new_tid != -1){
		// pin thread
		if(options.run_core_cnt != 0){
			pin_thread(new_tid, options.run_cores[*run_core_idx]);
			(*run_core_idx)++;
			*run_core_idx = *run_core_idx % options.run_core_cnt;
		}
		// attach monitoring session to thread
		if(!options.is_sys_wide_mon)
			ret = monitor_new_thread(new_tid, flags, &options);
	}
	
	return ret;
}


/*
 * Parent process for per-thread monitoring
 */
int parent_threadmon(char ** args)
{
	pid_t pid;
	pid_t tid;
	int ret;
	
	int status;
	unsigned long ptrace_flags;
	struct rusage rusage;
	int wait_type;
	unsigned long sig;
	int flags;
	int run_core_idx = 0;
	
	/* initialize PMU monitoring */
	ret = pfm_operations_init();
	if(ret != 0 )
		errx(1, "PMU initialization failed\n");
	
	/*
	 * create the child task
	 */
	if ((pid=fork()) == -1)
		err(1, "Cannot fork process");
	
	/*
	 * Child process
	 */
	if(pid == 0){
		/*
		 * allow paren to trace
		 */
		ret = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		if (ret == -1) {
			errx(1, "cannot ptrace self\n");
		}
		
		exit(child(args));
		/* not reached */
	}
	
	
	/*
	 * parent process
	 */
	
	
	/* wait for the child to exec */
	waitpid(pid, &status, WUNTRACED);
	
	/* attach to the first child thread */
	//pfm_attach(pid, options.events, PFM_OP_ENABLE_ON_EXEC, 
	// &(options.pfm_options));
	flags = 0;
	if(options.run_core_cnt != 0){
		pin_thread(pid, options.run_cores[run_core_idx]);
		run_core_idx++;
		run_core_idx %= options.run_core_cnt;
	}
	pfm_attach_thread(pid, options.events, flags, &(options.pfm_options));

	/*
	 * child is stopped here
	 */
	ptrace_flags = 0UL | PTRACE_O_TRACEEXEC | PTRACE_O_TRACEFORK | 
		PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE;
	ret = ptrace(PTRACE_SETOPTIONS, pid, NULL, (void *)ptrace_flags);
	if (ret == -1) 
		errx(1, "cannot set ptrace options on child porcess [%d]\n", 
		     pid);
		
  
	/*
	 * let the child continue
	 */
	child_continue(pid, 0);
  
	/*
	 * WUNTRACED: PTtrace events
	 * WNBOHANG : not block, return -1 instead
	 * __WALL   : return info about all threads
	 */
	//wait_type = WUNTRACED|__WALL;
	wait_type = __WALL;

	/*
	 * main loop that handle the child's traces
	 */
	while((tid = wait4(-1, &status, wait_type, &rusage)) > 0){

		if (WIFEXITED(status) || WIFSIGNALED(status)){
			DPRINTF("Thread [%d] terminated\n", tid);
		  
			if(tid == pid) /* main process quit */
				break;
			
			continue; /* nothing else todo */
		}
		
		if(WIFSTOPPED(status)){
			sig = WSTOPSIG(status);
			if (sig == SIGTRAP){
				/*
				 * do not propagate the signal, it was for us
				 */
				sig = 0;
				sig = handle_sigtrap(tid, status, flags, 
						     &run_core_idx);
			}
			else{
				DPRINTF("Awake for thread [%d] with sig %lu, "
					"event not handled\n", tid, sig);
				/* 
				 * Interestingly, I don't know what caused these
				 * stops, so I just let the program proceed 
				 */
				//sig = 0;
			}
		}

		
		/*
		 * let the child continue
		 */
		child_continue(tid, sig);
	}

	DPRINTF("Child process [%d] terminated\n", tid);
	
	/* print results */
	pfm_read_all_threads(&(options.pfm_options));  
	
	/* cleanup PMU monitoring */
	pfm_operations_cleanup();
	
	return 0;
}

void * dummy_thread(void * param){
	int *p = (int*) param;
	int sum;
	struct sched_param sp;
	int tid = gettid();

	sp.sched_priority = 0;
	//sched_setscheduler(tid, SCHED_IDLE, &sp);
	sched_setscheduler(tid, SCHED_BATCH, &sp);

	while(p != NULL)
		sum += sum;

	*p += sum;

	return p;
}

/*
 * Parent process for system-wide (per-core) monitoring
 */
int parent_coremon(char ** args)
{
	pid_t pid, tid;
	int ret;
	int * cpus;
	int i, cpu_num;
	int flags;
	
	int status;
	unsigned long ptrace_flags;
	struct rusage rusage;
	int wait_type;
	unsigned long sig;

	pthread_t pt;
	pthread_attr_t attr;
	cpu_set_t cpuset;

	int run_core_idx = 0;
	
	/* process cpu list */
	if(options.cores == NULL){
		/* no cpu specified, monitor all cpus */
		cpu_num = (int)sysconf(_SC_NPROCESSORS_ONLN);
		cpus = malloc(sizeof(int) * cpu_num);
		for(i = 0; i < cpu_num; i++)
			cpus[i] = i;
	}
	else{
		/* monitor only specified cpus */
		ret = parse_value_list(options.cores, (void**)&cpus, &cpu_num, 
				       0);
		if(ret != 0)
			errx(1, "Parsing CPU list failed with error %d\n", ret);
	}

	/* initialize PMU monitoring */
	ret = pfm_operations_init();
	if(ret != 0 )
		errx(1, "PMU initialization failed\n");

	/*
	 * create the child task
	 */
	if ((pid=fork()) == -1)
		err(1, "Cannot fork process");
	
	/*
	 * Child process
	 */
	if(pid == 0){
		/*
		 * allow paren to trace
		 */
		ret = ptrace(PTRACE_TRACEME, 0, NULL, NULL);
		if (ret == -1) 
			errx(1, "cannot ptrace self\n");

		exit(child(args));
		/* not reached */
	}
  
	/*
	 * paren process
	 */
	
	/* wait for the child to exec */
	waitpid(pid, &status, WUNTRACED);
	if(options.run_core_cnt != 0){
		pin_thread(pid, options.run_cores[run_core_idx]);
		run_core_idx++;
		run_core_idx %= options.run_core_cnt;		
	}
  
	/* attach CPU monitoring contexts */
	flags = 0;
	if(options.use_dummy_thread)
		// set the scheduling properties for the dummy threads
		pthread_attr_init(&attr);
	for(i = 0; i < cpu_num; i++){
		pfm_attach_core(cpus[i], options.events, flags, 
				&(options.pfm_options));
		if(options.use_dummy_thread){
			CPU_ZERO(&cpuset);
			CPU_SET(cpus[i], &cpuset);
			pthread_attr_setaffinity_np(&attr, sizeof(cpuset),
						    &cpuset);
			pthread_create(&pt, &attr, dummy_thread, 
				       &options.dummy_result);
		}
	}
	if(options.use_dummy_thread)
		pthread_attr_destroy(&attr);
  
	/* child is stopped here */
	/* Detach ptrace, we don't need it anymore */
	/* ret = ptrace(PTRACE_DETACH, pid, NULL, NULL); */
	/* if(ret == -1) */
	/* 	errx(1, "cannot detach ptrace from child\n"); */
	/* wait for the child process to quit */
	/* waitpid(pid, &status, 0); */

	
	ptrace_flags = 0UL | PTRACE_O_TRACEEXEC | PTRACE_O_TRACEFORK | 
		PTRACE_O_TRACEVFORK | PTRACE_O_TRACECLONE;
	ret = ptrace(PTRACE_SETOPTIONS, pid, NULL, (void *)ptrace_flags);
	if (ret == -1) 
		errx(1, "cannot set ptrace options on child porcess [%d]\n", 
		     pid);
		
  
	/*
	 * let the child continue
	 */
	child_continue(pid, 0);
  
	/*
	 * WUNTRACED: PTtrace events
	 * WNBOHANG : not block, return -1 instead
	 * __WALL   : return info about all threads
	 */
	//wait_type = WUNTRACED|__WALL;
	wait_type = __WALL;

	/*
	 * main loop that handle the child's traces
	 */
	while((tid = wait4(-1, &status, wait_type, &rusage)) > 0){

		if (WIFEXITED(status) || WIFSIGNALED(status)){
			DPRINTF("Thread [%d] terminated\n", tid);
		  
			if(tid == pid) /* main process quit */
				break;
			
			continue; /* nothing else todo */
		}
		
		if(WIFSTOPPED(status)){
			sig = WSTOPSIG(status);
			if (sig == SIGTRAP){
				/*
				 * do not propagate the signal, it was for us
				 */
				sig = 0;
				sig = handle_sigtrap(tid, status, flags, 
						     &run_core_idx);
			}
			else{
				DPRINTF("Awake for thread [%d] with sig %lu, "
					"event not handled\n", tid, sig);
				/* 
				 * Interestingly, I don't know what caused these
				 * stops, so I just let the program proceed 
				 */
				//sig = 0;
			}
		}

		
		/*
		 * let the child continue
		 */
		child_continue(tid, sig);
	}
  	
	DPRINTF("Child process [%d] terminated\n", pid);
	
	/* print results */
	pfm_read_all_cores(&(options.pfm_options));  
  
	/* cleanup PMU monitoring */
	pfm_operations_cleanup();
	
	return 0;
}

void usage(void)
{
	printf("usage: pfm_multi [-h] [-C] [-c cpu]  [-i interval] [-g] [-p] "
	       "[-e event1,event2,...] cmd\n"
	       "-h\t\tshow this help\n"
	       "-i\t\tprint counts every interval nanosecond\n"
	       "-g\t\tgroup events\n"
	       "-p\t\tpin events to cpu\n"
	       "-C\t\tsystem wide monitoring (per-core instead of per-thread), "
	       "all cores are monitored if not specified by -c\n"
	       "-c\t\tcores to monitor (comma separated list), must be used "
	       "with -C\n"
	       "-t\t\tAllow monitored threads to enable/disable monitoring\n"
	       "-e ev,ev\tgroup of events to measure (multiple -e switches are "
	       "allowed)\n"
	       "-D\t\tCreate low-priority threads doing dummy work on cores "
	       "being system-wide monitored\n"
	       "-P\t\tcores to run application threads (comma separated list)\n"
	       );
}

void parse_cmdln_params(int argc, char **argv)
{
	int c;
	int ret = 0;
	

	/* get command line parameters */
	options.pfm_options.grouped = 0;
	options.pfm_options.pinned = 0;
	options.pfm_options.enable_new = 1;
	options.print_interval = 0;
	options.events = NULL;
	options.is_sys_wide_mon = 0;
	options.cores = NULL;
	options.use_trigger = 0;
	options.trigger_info = NULL;
	options.use_dummy_thread = 0;
	options.run_core_cnt = 0;
	while ((c=getopt(argc, argv,"+hgpCc:i:e:tDP:")) != -1) {
		switch(c) {
		case 'e':
			options.events = strdup(optarg);
			break;
		case 'i':
			options.print_interval = atol(optarg);
			enable_logging = 1;
			DPRINTF("Printing interval is %ld\n", 
				options.print_interval);
			break;
		case 'h':
			usage();
			exit(0);
			break;
		case 'g':
			options.pfm_options.grouped = 1;
			DPRINTF("Grouped is %d\n", options.pfm_options.grouped);
			break;
		case 'p':
			options.pfm_options.pinned = 1;
			DPRINTF("Pinned is %d\n", options.pfm_options.pinned);
			break;
		case 'C':
			options.is_sys_wide_mon = 1;
			DPRINTF("Is System wide monitoring\n");
			break;
		case 'c':
			options.cores = strdup(optarg);
			DPRINTF("Monitoring cores %s\n", options.cores);
			break;
		case 't':
			options.use_trigger = 1;
			options.pfm_options.enable_new = 0;
			DPRINTF("Allow monitored threads trigger "
				"monitoring\n");
			break;
		case 'D':
			options.use_dummy_thread = 1;
			DPRINTF("Enable dummy threads\n");
			break;
		case 'P':
			ret = parse_value_list(strdup(optarg), 
					       (void**)&options.run_cores, 
					       &options.run_core_cnt, 0);
			if(ret != 0)
				errx(1, "Parsing run-core-list failed with "
				     "error %d\n", ret);

			DPRINTF("Pin threads %s\n", optarg);
			break;
		default:
			errx(1, "unknown parameter, use option \"-h\" to get "
			     "usage\n");
		}
	}

}

void * logging_thread(void * param)
{
	struct timespec wait_length;
	
	wait_length.tv_sec = options.print_interval / 1000000000UL;
	wait_length.tv_nsec = options.print_interval % 1000000000UL;
	
	while(enable_logging){
		nanosleep(&wait_length, NULL);
		if(options.is_sys_wide_mon)
			pfm_read_all_cores(&(options.pfm_options));
		else
			pfm_read_all_threads(&(options.pfm_options));
	}
	
	return NULL;
}

void * trigger_thread(void * param)
{
	options_t * options = (options_t*)param;

	DPRINTF("Begin pfm trigger processing\n");
	pfm_trigger_begin_process(options->trigger_info);
	DPRINTF("End pfm trigger processing\n");

	return NULL;
}


int main(int argc, char **argv)
{
	pthread_t logger;
	pthread_t trigger_thr;
	
	setlocale(LC_ALL, "");
  
	enable_logging = 0;
	
	parse_cmdln_params(argc, argv);
	
	if (!argv[optind])
		errx(1, "you must specify a command to execute\n");
	
	if(options.events == NULL)
		options.events = DEFAULT_PMU_EVENTS;

	DPRINTF("Executing command %s\n", argv[optind]);

	/* create a thread for periodical PMU result output */
	if(enable_logging)
		pthread_create(&logger, NULL, logging_thread, NULL); 

	if(options.use_trigger){
		pfm_trigger_init(&options.trigger_info, options.is_sys_wide_mon,
				 &options.pfm_options.enable_new, 
				 &options.pfm_options);
		pthread_create(&trigger_thr, NULL, trigger_thread, 
			       (void*)&options);
	}

	if(options.is_sys_wide_mon)
		/* system-wide (per-core) monitoring */
		parent_coremon(argv+optind); 
	else
		/* per-thread monitoring */
		parent_threadmon(argv+optind); 
	
	if(options.use_trigger)
		pfm_trigger_close(&options.trigger_info);
	
	
	return 0;
}

