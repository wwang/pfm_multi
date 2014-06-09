/*
 * This file contains the pfm_trigger module for pfm_multi.
 * pfm_trigger allows the threads being monitored to enable and disable 
 * their own performance monitoring. Check out the pfm_trigger_user.h/c
 * for information on using this module in monitored threads.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */


#include <stdio.h>
#include <stdlib.h>

// for message queue
#include <messageQx.h>

#include "pfm_common.h"
#include "pfm_trigger.h"
#include "pfm_trigger_common.h"
#include "pfm_operations.h"

// data used by pfm_trigger
typedef struct _pfm_trigger_info{
	void *msgq;
	int is_sys_wide;
	int *enable_new;
	// options is pfm_operations_options_t type, kept for future extension
	void *pfm_op_options;
}trigger_info;

int pfm_trigger_init(void **handle, int is_sys_wide, int *enable_new, 
		     void *pfm_op_options)
{
	void * msgq;
	int ret_val;
	trigger_info *t;

	if(handle == NULL || enable_new == NULL || pfm_op_options == NULL)
		return 2;

	*handle = NULL;

	// create the message queue for communication
	ret_val = msgqx_create(PFM_TRIGGER_MSG_NAME, sizeof(trigger_msg), 
			       MAX_NUM_THREADS, &(msgq));
	if(ret_val){
		DPRINTF("Failed to create message queue for pfm trigger %d\n",
			ret_val);
		return 1;
	}
	
	// create the pfm_trigger handle and initialize it
	t = (trigger_info*)calloc(1, sizeof(trigger_info));
	t->msgq = msgq;
	t->is_sys_wide = is_sys_wide;
	t->enable_new = enable_new;
	*enable_new = 0;
	t->pfm_op_options = pfm_op_options;
	*handle = (void*)t;
	
	return 0;
}


int pfm_trigger_close(void *handle)
{
	trigger_info *t = (trigger_info*)handle;
	int ret_val = 0;

	if(t == NULL || t->msgq == NULL)
		return 1;
	
	ret_val = msgqx_close(t->msgq);
	ret_val = msgqx_destroy(PFM_TRIGGER_MSG_NAME);
	
	return ret_val;
}


int pfm_trigger_begin_process(void *handle)
{
	trigger_info *t = (trigger_info*)handle;
	int ret_val = 0;
	trigger_msg msg;
	int quit = 0;

	if(t == NULL || t->msgq == NULL)
		return 1;

	while(1){
		ret_val = msgqx_receive(t->msgq, &msg);

		if(ret_val){
			fprintf(stderr, "pfm_trigger message wait error: %d",
				ret_val);
			continue;
		}
		
		switch(msg.msg){
		case thr_enable:
			DPRINTF("pfm_trigger enabling thread %d\n", msg.id);
			pfm_enable_mon_thread(t->pfm_op_options, msg.id, 1);
			break;
		case thr_disable:
			DPRINTF("pfm_trigger disabling thread %d\n", msg.id);
			pfm_enable_mon_thread(t->pfm_op_options, msg.id, 0);
			break;
		case cpu_enable:
			DPRINTF("pfm_trigger enabling cpu %d\n", msg.id);
			pfm_enable_mon_core(t->pfm_op_options, msg.id, 1);
			break;
		case cpu_disable:
			DPRINTF("pfm_trigger disabling cpu %d\n", msg.id);
			pfm_enable_mon_core(t->pfm_op_options, msg.id, 0);
			break;
		case all_enable:
			DPRINTF("pfm_trigger enabling all\n");
			*t->enable_new = 1;
			if(t->is_sys_wide)
				pfm_enable_mon_all_cores(t->pfm_op_options, 1);
			else
				pfm_enable_mon_all_threads(t->pfm_op_options, 
							   1);
			break;
		case all_disable:
			DPRINTF("pfm_trigger disabling all\n");
			*t->enable_new = 0;
			if(t->is_sys_wide)
				pfm_enable_mon_all_cores(t->pfm_op_options, 0);
			else
				pfm_enable_mon_all_threads(t->pfm_op_options, 
							   0);
			break;
		case quit_trigger:
			DPRINTF("pfm_trigger quiting\n");
			quit = 1;
			break;
		default:
			fprintf(stderr, "Unknown message type in pfm_trigger: "
				"%d\n", msg.msg);
		}

		if(quit) break;
	}

	return 0;
}
