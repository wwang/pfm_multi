/*
 * Header file for the pfm trigger user library. The trigger library allowed
 * monitored programs/threads to enable/disable their monitoring.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __PFM_TRIGGER_USER_LIB_H__
#define __PFM_TRIGGER_USER_LIB_H__
#ifdef __cplusplus
extern "C" {
#endif

/*
 * initialized the semaphore and the shared memory
 * 
 * Return values:
 *       0: success
 *       1: open semaphore failed 
 *       2: open shared memory failed
 */
int pfm_user_trigger_init();
// Fortran interface
void pfm_trigger_init_();

/*
 * enable monitoring for this thread
 * Parameters:
 *      enable: 1 to enable; 0 to disable
 *
 * Return values:
 *      0:  success
 *      1:  non shared memory and/or semaphore
 *      2:  shared memory has 0 spaces for threads
 *      3:  current state is the same as the old state
 *      4:  failed to post semaphore
 */
int pfm_user_trigger_enable_mon(int enable);
// Fortran interface
void pfm_trigger_enable_();
void pfm_trigger_disable_();

/*
 * cleanup the semaphore and the shared memory
 * Parameters:
 *       destroy: whether destroy the semaphore and the shared memory;
 *                1: yes; 0: no;
 * Return values:
 *       0: success
 *       1: close semaphore failed 
 *       2: close shared memory failed
 *       3: both operation failed
 */
int pfm_user_trigger_cleanup(int destroy);
// Fortran interface
void pfm_trigger_cleanup_();

#ifdef __cplusplus
}
#endif
#endif
