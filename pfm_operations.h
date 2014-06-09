#ifndef __PFM_OPRATIONS__
#define __PFM_OPRATIONS__

#include <sys/types.h>
#include <unistd.h>

/*
 * Options for PMU reading
 */
typedef struct __pfm_operations_options{
	int grouped; /* whether the PMU events should be grouped */
	int pinned; /* whether the events should be pinned to the CPU */
	int enable_new; /* whether enable monitoring on newly-created 
			   threads/cpus */
}pfm_operations_options_t;

/*
 * Initilization
 * Paramters:
 *	NONE
 * Retun value:
 * Return value:
 *      0       --> success
 *      other   --> failed
 */
int pfm_operations_init();

#define PFM_OP_ENABLE_ON_EXEC		(1U << 0)

/*
 * Attach to a thread for PMU readings
 * Parameters:
 * 	tid	--> thread id to attach
 *	evns 	--> list of evns to monitor, comma separated list in a string
 *	flags	--> Interval flags used by pfm_operations, not confused kernel perf flags
 *                  See following macros for available flags
 *	options	--> options for PMU monitoring
 * Return value:
 *      0       --> success
 *      other   --> failed
 */
int pfm_attach_thread(pid_t tid, char * evns, int flags, pfm_operations_options_t * options); 

/*
 * Read PMU counters for one thread
 * Parameters:
 * 	tid	--> thread id to read
 *	options	--> options for PMU monitoring
 * Return value:
 *      0       --> success
 *      other   --> failed
 * TODO: Implementation...
 */
int pfm_read_one_thread(pid_t tid, pfm_operations_options_t * options); 

/*
 * Read PMU counters for all managed threads
 * Parameters:
 *	options	--> options for PMU monitoring
 * Return value:
 *      0       --> success
 *      other   --> failed
 */
int pfm_read_all_threads(pfm_operations_options_t * options); 


/*
 * Attach to a core for PMU readings
 * Parameters:
 * 	cpu	--> cpu to attach
 *	evns 	--> list of evns to monitor, comma separated list in a string
 *	flags	--> Interval flags used by pfm_operations, not confused kernel perf flags
 *                  See following macros for available flags
 *	options	--> options for PMU monitoring
 * Return value:
 *      0       --> success
 *      other   --> failed
 */
int pfm_attach_core(int cpu, char * evns, int flags, pfm_operations_options_t * options); 


/*
 * Read PMU counters for all managed cores
 * Parameters:
 *	options	--> options for PMU monitoring
 * Return value:
 *      0       --> success
 *      other   --> failed
 */
int pfm_read_all_cores(pfm_operations_options_t * options); 


/*
 * Cleanup
 */
int pfm_operations_cleanup();

/*
 * enable/disable the monitoring for one thread
 * Parameters:
 *	pfm_op_options	--> options for PMU monitoring
 *      tid     --> id of the thread to be enabled/disabled
 *      enabled --> 0 means to disable, 1 means to enable
 * Return value:
 *      0       --> success
 *      1       --> no thread with matching tid found
 *      2       --> error when enabling/disabling thread monitoring
 */
int pfm_enable_mon_thread(void *pfm_op_options, pid_t tid, int enabled);

/*
 * enable/disable the monitoring for all threads
 * Parameters:
 *	pfm_op_options	--> options for PMU monitoring
 *      tid     --> id of the thread to be enabled/disabled
 *      enabled --> 0 means to disable, 1 means to enable
 * Return value:
 *      0       --> success
 *      1       --> error when enabling/disabling some threads
 */
int pfm_enable_mon_all_threads(void *pfm_op_options, int enabled);

/*
 * enable/disable the monitoring for one core
 * Parameters:
 *	pfm_op_options	--> options for PMU monitoring
 *      cpu     --> id of the cpu to be enabled/disabled
 *      enabled --> 0 means to disable, 1 means to enable
 * Return value:
 *      0       --> success
 *      1       --> no core with matching cpu id found
 *      2       --> error when enabling/disabling cpu monitoring
 */
int pfm_enable_mon_core(void *pfm_op_options, int cpu, int enabled);

/*
 * enable/disable monitoring for all cores
 * Parameters:
 *	pfm_op_options	--> options for PMU monitoring
 *      enabled --> 0 means to disable, 1 means to enable
 * Return value:
 *      0       --> success
 *      1       --> error when enabling/disabling some cores
 */
int pfm_enable_mon_all_cores(void *pfm_op_options, int enabled);

#endif
