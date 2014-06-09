/*
 * This file contains the data structure definitions use by
 * pfm_trigger.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __PFM_SEMAPHORE_H__
#define __PFM_SEMAPHORE_H__

/*
 * Initialize the pfm_trigger
 * 
 * Input parameters:
 *    is_sys_wide: whether system wide monitoring
 *    enable_new: the address of the flags that marks whether newly-created 
 *                threads are started enabled
 *    pfm_op_options: options is pfm_operations_options_t type, kept for future 
                      extension
 * Output parameter:
 *    handle: the handle of pfm_trigger
 *
 * Return values:
 *    0: success
 *    1: failed to open message queue
 *    2: invalid parameter, handle and/or enable_new are NULL
 *
 */
int pfm_trigger_init(void **handle, int is_sys_wide, int *enable_new, 
		     void *pfm_op_options);

/*
 * Close the pfm_trigger
 * Input parameters:
 *    handle: the handle to the pfm_trigger
 *
 * return values:
 *    0: success
 *    1: invalid handle
 *    2: failed to close and/or destroy message queue
 */
int pfm_trigger_close(void *handle);

/* 
 * Start pfm_trigger to process enabling/disabling messages from monitored 
 * threads. Note that this function should be called within a stand alone
 * thread because this call will block the calling thread.
 * 
 * Input parameter:
 *    handle: the handle to t he pfm_trigger
 * 
 * return values:
 *    0: success
 *    1: invalid handle
 *    2: message queue operation error 
 */
int pfm_trigger_begin_process(void *handle);

#endif
