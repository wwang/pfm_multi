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
 *       1: failed to open message queue
 *       2: open shared memory failed
 */
int pfm_trigger_user_init();
// Fortran interface
void pfm_trigger_init_();

/*
 * enable monitoring for this thread
 * Parameters:
 *      enable: 1 to enable; 0 to disable
 *
 * Return values:
 *      0:  success
 *      1:  no message queue; pfm_trigger not initialized
 *      2:  failed to send message
 */
int pfm_trigger_user_enable_thread(int enable);
// Fortran interface
void pfm_trigger_enable_thread_(int *enable);

/*
 * enable monitoring for one cpu
 * Parameters:
 *      cpu: the cpu whose monitoring is to be enabled/disabing
 *      enable: 1 to enable; 0 to disable
 *
 * Return values:
 *      0:  success
 *      1:  no message queue; pfm_trigger not initialized
 *      2:  failed to send message
 */
int pfm_trigger_user_enable_core(int cpu, int enable);
// Fortran interface
void pfm_trigger_enable_core_(int *cpu, int *enable);

/*
 * enable monitoring for all threads/cpus
 * Parameters:
 *      enable: 1 to enable; 0 to disable
 *
 * Return values:
 *      0:  success
 *      1:  no message queue; pfm_trigger not initialized
 *      2:  failed to send message
 */
int pfm_trigger_user_enable_all(int enable);
// Fortran interface
void pfm_trigger_enable_all_(int *enable);

/*
 * tell pfm_multi to stop the trigger message processing thread
 * Parameters:
 *
 * Return values:
 *      0:  success
 *      1:  no message queue; pfm_trigger not initialized
 *      2:  failed to send message
 */
int pfm_trigger_user_stop();
// Fortran interface
void pfm_trigger_stop_();


/*
 * cleanup the semaphore and the shared memory
 * Parameters:
 *       
 * Return values:
 *       0: success
 *       other: failed
 */
int pfm_trigger_user_cleanup();
// Fortran interface
void pfm_trigger_cleanup_();

#ifdef __cplusplus
}
#endif
#endif
