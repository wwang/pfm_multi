/*
 * This file contains the data structure definitions use by
 * pfm_trigger.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __PFM_SEMAPHORE_H__
#define __PFM_SEMAPHORE_H__

/*
 * whether enable or disable performance monitoring of thread 
 */
typedef struct _pfm_thread_monitor_state{
	int tid;
	int cur_state; // 0 disable, 1 enable
	int new_state;
}pfm_thr_mon_state;

/*
 * This structure holds the enable/disable states of all threads;
 * this structure is stored on a shared memory for inter-process
 * communication between parent and children.
 */
typedef struct _pfm_monitor_states{
	int thr_cnt; // how many threads can be tracked
	int syswide_state; // system wide monitoring state
	int pads[13]; // padding to make sure "states" starts at
                      // at a new cache line
	pfm_thr_mon_state states; // array of the states; because memory 
	                          // grow beyond here, keep this member at
	                          // the end of the structure
}pfm_mon_states;

/* 
 * This structure holds the information use for inter-process
 * communication between parent and children.
 */
typedef struct _pfm_trigger_info{
	void * sem; // the semaphore
	int shm_id;
	pfm_mon_states * mem; // the shared memory
}pfm_trigger_info;

#define PFM_TRIGGER_NAME "pfm_multi_trigger"
#define PFM_TRIGGER_MEM_FILE "/tmp/pfm_multi_trigger_mem"
#define PFM_TRIGGER_MEM_PROJ_ID 1

/*
 * create a new semaphore which allows a monitored thread to notify
 * pfm_multi that its enable/disable state has changed.
 *
 * Return 0 when succeed, quit the program if failed
 */
int pfm_open_trigger_sem(pfm_trigger_info * trigger);

/* 
 * create a new shared memory for storing the enabled/disabled status
 *
 * Return 0 when succeed, quit the program if failed
 */
int pfm_open_trigger_mem(pfm_trigger_info * trigger, int is_sys_wide_mon);

/* 
 * remove opened shared memory
 * 
 * return values:
 *    0:  success
 *    1:  can not detach memory
 *    2:  can not close share memory object
 */
int pfm_close_trigger_mem(pfm_trigger_info * trigger);

/* 
 * remove opened semaphore
 * 
 * return values:
 *    0:  success
 *    1:  can not close semaphore
 *    2:  can not unlink semaphore
 */
int pfm_close_trigger_sem(pfm_trigger_info * trigger);

/* 
 * remove opened semaphore
 * 
 * return values:
 *    0:  success
 *    1:  failure
 */
int pfm_trigger_wait_sem(pfm_trigger_info * trigger);

/*
 * initialized the shared memory 
 */
int pfm_trigger_init_mem(pfm_trigger_info * trigger, int is_sys_wide_mon);

/*
 * enable or disable the monitoring.
 * Parameters:
 *   is_sys_wide: whether system wide monitoring is enabled
 *   options:  pointer to the pfm_options object
 *   trigger:  pointer to the pfm_trigger_info object
 * Return values:
 *   0 :  when succeed
 */
int pfm_trigger_enable(int is_sys_wide, void * options, 
		       pfm_trigger_info * trigger);

#endif
