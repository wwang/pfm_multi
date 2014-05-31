/*
 * Wrapper functions for setting up perfmon monitoring sessions
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>

/* 
 * We use libpfm and helper functions from Stephane Eranian 
 */
#include "perf_util.h" 

#include "pfm_operations.h"
#include "pfm_common.h"

typedef struct __thread_pfm_context{
	perf_event_desc_t *fds;
	int num_fds;
	pid_t tid;
	int grouped;
	int enabled;
}thread_pfm_context_t;

thread_pfm_context_t thread_ctxs[MAX_NUM_THREADS];
int thr_ctx_idx;

typedef struct __core_pfm_context{
	perf_event_desc_t *fds;
	int num_fds;
	int cpu;
	int grouped;
	int enabled;
}core_pfm_context_t;

core_pfm_context_t core_ctxs[MAX_NUM_CORES];
int core_ctx_idx;

void read_counts(perf_event_desc_t *fds, int num);
void print_thread_counts(pid_t tid, perf_event_desc_t *fds, int num);
void print_core_counts(int cpu, perf_event_desc_t *fds, int num);

/*
 * Initilization
 * Paramters:
 *	NONE
 * Retun value:
 * Return value:
 *      0       --> success
 *      other   --> failed
 */
int pfm_operations_init()
{
  if (pfm_initialize() != PFM_SUCCESS)
    {
      warnx("libpfm initialization failed");
      return -1;
    }
  
  thr_ctx_idx = 0;
  
  return 0;
}

/*
 * Attach to a thread for PMU readings
 * Parameters:
 * 	tid	--> thread id to attach
 *	evns 	--> list of evns to monitor, comma seperated list in a string
 *	flags	--> Interval flags used by pfm_operations, not confused kernel perf flags
 *                  See the header file for available flags
 *	options	--> options for PMU monitoring
 * Return value:
 *      0       --> success
 *      other   --> failed
 */
int pfm_attach_thread(pid_t tid, char * evns, int flags, pfm_operations_options_t * options)
{
  int ret;
  int i;
  int group_fd;
  perf_event_desc_t * fds;

  thread_ctxs[thr_ctx_idx].tid = tid;
  thread_ctxs[thr_ctx_idx].fds = NULL;
  thread_ctxs[thr_ctx_idx].num_fds = 0;
  if(flags & PFM_OP_START_DISABLED)
	  thread_ctxs[thr_ctx_idx].enabled = 0;
  else 
	  thread_ctxs[thr_ctx_idx].enabled = 1;

  ret = perf_setup_list_events(evns, &(thread_ctxs[thr_ctx_idx].fds), &(thread_ctxs[thr_ctx_idx].num_fds));
  if(ret || !(thread_ctxs[thr_ctx_idx].num_fds))
    return -1;

  fds = thread_ctxs[thr_ctx_idx].fds;

  for(i = 0; i < thread_ctxs[thr_ctx_idx].num_fds; i++)
    {
      int is_group_leader;
      
      if(options->grouped)
	{
	  is_group_leader = perf_is_group_leader(fds, i);
	}
      else
	{
	  is_group_leader = 1; /* if not grouped then every body is the leader of itself */
	}
      
      if(is_group_leader)
	{
	  group_fd = -1; 
	}
      else
	{
	  group_fd = fds[fds[i].group_leader].fd;
	}
      
      /*
       * create leader disabled with enable_on-exec
       */
      if(flags & PFM_OP_ENABLE_ON_EXEC){
	      DPRINTF("Thread [%d] pfm enable on exec\n", tid);
	      fds[i].hw.disabled = is_group_leader;
	      fds[i].hw.enable_on_exec = is_group_leader;
      }
      else if (flags & PFM_OP_START_DISABLED){
	      fds[i].hw.disabled = 1;
	      fds[i].hw.enable_on_exec = 0;
      }
      else{
	      fds[i].hw.disabled = 0;
	      fds[i].hw.enable_on_exec = 0;
      }
      
      fds[i].hw.read_format = PERF_FORMAT_SCALE;
     
      fds[i].hw.inherit = 0; /* only monitor the current thread */
      
      if (options->pinned && is_group_leader)
	fds[i].hw.pinned = 1;
     
      fds[i].fd = perf_event_open(&fds[i].hw, tid, -1, group_fd, 0);
      if (fds[i].fd == -1) 
	{
	  warn("cannot attach event%d %s to thread [%d]", i, fds[i].name, tid);
	  goto error;
	}
      DPRINTF("PMU context opened for thread [%d]\n", tid);
    }
  
  thr_ctx_idx++;

  return 0;

 error:
  free(fds);
  
  return -1;
}

/*
 * Cleanup
 */
int pfm_operations_cleanup()
{
  int i;

  for(i = 0; i < thr_ctx_idx; i++)
    {
      if(thread_ctxs[i].fds)
	free(thread_ctxs[i].fds);
    }


  /* free libpfm resources cleanly */
  pfm_terminate();
  

  return 0;
}

void read_counts(perf_event_desc_t *fds, int num)
{
  uint64_t values[3];
  int evt, ret;

  for (evt = 0; evt < num; evt++) 
    {
      ret = read(fds[evt].fd, values, sizeof(values));
      if (ret != sizeof(values)) 
	{ /* unsigned */
	  if (ret == -1)
	    warnx("cannot read values event %s", fds[evt].name);
	  
	  /* likely pinned and could not be loaded */
	  warnx("could not read event %d, tried to read %zu bytes, but got %d",
		evt, sizeof(values), ret);
	}


      /*
       * scaling because we may be sharing the PMU and
       * thus may be multiplexed
       */
      fds[evt].prev_value = fds[evt].value;
      fds[evt].value = perf_scale(values);
      fds[evt].enabled = values[1];
      fds[evt].running = values[2];
    }

  return;
}

void print_thread_counts(pid_t tid, perf_event_desc_t *fds, int num)
{
  int i;

  read_counts(fds, num);

  for(i=0; i < num; i++) 
    {
      double ratio;
      uint64_t val;
      
      val = fds[i].value - fds[i].prev_value;
      
      ratio = 0.0;
      if (fds[i].enabled)
	ratio = 1.0 * fds[i].running / fds[i].enabled;
      
      /* separate groups */
      if (perf_is_group_leader(fds, i))
	putchar('\n');
      
      if (fds[i].value < fds[i].prev_value) 
	{
	  printf("inconsistent scaling %s (cur=%'"PRIu64" : prev=%'"PRIu64")\n", fds[i].name, fds[i].value, fds[i].prev_value);
	  continue;
	}
      printf("thread [%d]:%'20"PRIu64" %s (%.2f%% scaling, ena=%'"PRIu64", run=%'"PRIu64")\n",
	     tid,
	     val,
	     fds[i].name,
	     (1.0-ratio)*100.0,
	     fds[i].enabled,
	     fds[i].running);
    }

  return;
}

void print_core_counts(int cpu, perf_event_desc_t *fds, int num)
{
  int i;

  read_counts(fds, num);

  for(i=0; i < num; i++) 
    {
      double ratio;
      uint64_t val;
      
      val = fds[i].value - fds[i].prev_value;
      
      ratio = 0.0;
      if (fds[i].enabled)
	ratio = 1.0 * fds[i].running / fds[i].enabled;
      
      /* separate groups */
      if (perf_is_group_leader(fds, i))
	putchar('\n');
      
      if (fds[i].value < fds[i].prev_value) 
	{
	  printf("inconsistent scaling %s (cur=%'"PRIu64" : prev=%'"PRIu64")\n", fds[i].name, fds[i].value, fds[i].prev_value);
	  continue;
	}
      printf("CPU <%d>:%'20"PRIu64" %s (%.2f%% scaling, ena=%'"PRIu64", run=%'"PRIu64")\n",
	     cpu,
	     val,
	     fds[i].name,
	     (1.0-ratio)*100.0,
	     fds[i].enabled,
	     fds[i].running);
    }

  return;
}


/*
 * Read PMU counters for one thread
 * Parameters:
 * 	tid	--> thread id to read
 *	options	--> options for PMU monitoring
 * Return value:
 *      0       --> success
 *      other   --> failed
 */
int pfm_read_one_thread(pid_t tid, pfm_operations_options_t * options)
{
  return 0;
}

/*
 * Read PMU counters for all managed threads
 * Parameters:
 *	options	--> options for PMU monitoring
 * Return value:
 *      0       --> success
 *      other   --> failed
 */
int pfm_read_all_threads(pfm_operations_options_t * options)
{

  int i;
  
  for(i = 0; i < thr_ctx_idx; i++)
    {
      if(thread_ctxs[i].fds && thread_ctxs[i].enabled)
	{
	  print_thread_counts(thread_ctxs[i].tid, thread_ctxs[i].fds, thread_ctxs[i].num_fds);
	}
    }

  return 0;
}

/*
 * Attach to a core for PMU readings
 * Parameters:
 * 	cpu	--> cpu to attach
 *	evns 	--> list of evns to monitor, comma separated list in a string
 *	flags	--> Interval flags used by pfm_operations, not confused kernel perf flags
 *                  See header file for available flags
 *	options	--> options for PMU monitoring
 * Return value:
 *      0       --> success
 *      other   --> failed
 */
int pfm_attach_core(int cpu, char * evns, int flags, pfm_operations_options_t * options)
{
  int ret;
  int i;
  int group_fd;
  perf_event_desc_t * fds;

  core_ctxs[core_ctx_idx].cpu = cpu;
  core_ctxs[core_ctx_idx].fds = NULL;
  core_ctxs[core_ctx_idx].num_fds = 0;
  if(flags & PFM_OP_START_DISABLED)
	  core_ctxs[core_ctx_idx].enabled = 0;
  else
  	  core_ctxs[core_ctx_idx].enabled = 1;

  ret = perf_setup_list_events(evns, &(core_ctxs[core_ctx_idx].fds), &(core_ctxs[core_ctx_idx].num_fds));
  if(ret || !(core_ctxs[core_ctx_idx].num_fds))
    return -1;

  fds = core_ctxs[core_ctx_idx].fds;

  for(i = 0; i < core_ctxs[core_ctx_idx].num_fds; i++)
    {
      int is_group_leader;
      
      if(options->grouped)
	{
	  is_group_leader = perf_is_group_leader(fds, i);
	}
      else
	{
	  is_group_leader = 1; /* if not grouped then every body is the leader of itself */
	}
      
      if(is_group_leader)
	{
	  group_fd = -1; 
	}
      else
	{
	  group_fd = fds[fds[i].group_leader].fd;
	}
      
      /*
       * create PMU context disabled?
       */
      if(flags & PFM_OP_START_DISABLED)
	      fds[i].hw.disabled = 1;
      else
	      fds[i].hw.disabled = 0;
      DPRINTF("CPU <%d> pfm created disabled %d\n", cpu, fds[i].hw.disabled);
	  
      fds[i].hw.read_format = PERF_FORMAT_SCALE;
      
      if (options->pinned && is_group_leader)
	fds[i].hw.pinned = 1;
     
      fds[i].fd = perf_event_open(&fds[i].hw, -1, cpu, group_fd, 0);
      if (fds[i].fd == -1) 
	{
	  warn("cannot attach event%d %s to CPU <%d>", i, fds[i].name, cpu);
	  goto error;
	}
      DPRINTF("PMU context opened for CPU <%d>\n", cpu);
    }
  
  core_ctx_idx++;

  return 0;

 error:
  free(fds);
  
  return -1;
}

int pfm_read_all_cores(pfm_operations_options_t * options)
{

  int i;
  
  for(i = 0; i < core_ctx_idx; i++)
    {
      if(core_ctxs[i].fds && core_ctxs[i].enabled)
	{
	  print_core_counts(core_ctxs[i].cpu, core_ctxs[i].fds, 
			    core_ctxs[i].num_fds);
	}
    }

  return 0;
}

int pfm_enable_mon_thread(pfm_operations_options_t * options, pid_t tid, 
			  int enabled)
{
	int i;
	int evt;
	long request;
	int ret_val;

	if(enabled)
		request = PERF_EVENT_IOC_ENABLE;
	else
		request = PERF_EVENT_IOC_DISABLE;
	
	for(i = 0; i < thr_ctx_idx; i++){
		if(thread_ctxs[i].tid == tid){
			thread_ctxs[i].enabled = enabled;
			if(!thread_ctxs[i].fds){
				// strange no event assoicated with this thread
				DPRINTF("No events for thread %d when trying "
					"to enable its events\n", tid);
				return 2;
			}

			// print out current reading if monitoring is to 
			// be disabled
			if(!enabled)
				print_thread_counts(tid, thread_ctxs[i].fds, 
						    thread_ctxs[i].num_fds);
			// disable the counters
			for (evt = 0; evt < thread_ctxs[i].num_fds; evt++){
				ret_val = ioctl(thread_ctxs[i].fds[evt].fd, 
						request);
				if(ret_val == -1){
					DPRINTF("Error when enable/disable "
						"event %s for thread %d: ",
						thread_ctxs[i].fds[evt].name,
						tid);
					perror("");
				}
			}
			return 0;
		}
	}
	
	return 1;
}


int pfm_enable_mon_core(pfm_operations_options_t * options, int enabled)
{
	int i;
	int evt;
	long request;
	int ret_val;
	int failed = 0;

	if(enabled)
		request = PERF_EVENT_IOC_ENABLE;
	else
		request = PERF_EVENT_IOC_DISABLE;
	
	for(i = 0; i < core_ctx_idx; i++){
		core_ctxs[i].enabled = enabled;
		if(core_ctxs[i].fds){
			// print out current reading if monitoring is to 
			// be disabled
			if(!enabled)
				print_core_counts(core_ctxs[i].cpu, 
						  core_ctxs[i].fds, 
						  core_ctxs[i].num_fds);
			for (evt = 0; evt < core_ctxs[i].num_fds; evt++){
				ret_val = ioctl(core_ctxs[i].fds[evt].fd, 
						request);
				if(ret_val == -1){
					DPRINTF("Error when enable/disable "
						"event %s for cpu %d: ",
						core_ctxs[i].fds[evt].name,
						core_ctxs[i].cpu);
					perror("");
					failed = 1;
				}
			}
		}
	}
	
  return failed;
}
