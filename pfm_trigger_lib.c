/*
 * This is static library called by monitored threads to enabled/disable their 
 * monitoring.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#include <stdio.h>
#include <stdlib.h>

#include <messageQx.h>
#include <common_toolx.h>

#include "pfm_trigger.h"
#include "pfm_common.h"
#include "pfm_trigger_lib.h"
#include "pfm_trigger_common.h"

void * msgq = NULL;

int pfm_trigger_user_init()
{
	int ret_val = 0;
	int size;

	msgq = NULL;
	ret_val = msgqx_open(PFM_TRIGGER_MSG_NAME, &msgq, &size);

	if(ret_val){
		msgq = NULL;
		return 1;
	}
	if(size != sizeof(trigger_msg)){
		msgqx_close(msgq);
		msgq = NULL;
		return 2;
	}
	
	return 0;
}

int pfm_trigger_user_enable_thread(int enable)
{
	int ret_val;
	trigger_msg msg;

	if(msgq == NULL)
		return 1;

	msg.id = gettid();
	if(enable)
		msg.msg = thr_enable;
	else
		msg.msg = thr_disable;

	
	ret_val = msgqx_send(msgq, &msg);
	DPRINTF("Message sent for thread %d changing monitoring state to %d\n",
		msg.id, enable);

	if(ret_val){
		fprintf(stderr, "Failed to send message %d from thread %d with "
			"return value %d\n", msg.msg, msg.id, ret_val);
		ret_val = 2;
	}
	
	return ret_val;
}


int pfm_trigger_user_enable_core(int cpu, int enable)
{
	int ret_val;
	trigger_msg msg;

	if(msgq == NULL)
		return 1;

	msg.id = cpu;
	if(enable)
		msg.msg = cpu_enable;
	else
		msg.msg = cpu_disable;

	ret_val = msgqx_send(msgq, &msg);
	DPRINTF("Message sent for cpu %d changing monitoring state to %d\n",
		msg.id, enable);

	if(ret_val){
		fprintf(stderr, "Failed to send message %d for cpu %d with "
			"return value %d\n", msg.msg, msg.id, ret_val);
		ret_val = 2;
	}
	
	return ret_val;
}

int pfm_trigger_user_enable_all(int enable)
{
	int ret_val;
	trigger_msg msg;

	if(msgq == NULL)
		return 1;

	msg.id = 0;
	if(enable)
		msg.msg = all_enable;
	else
		msg.msg = all_disable;

	ret_val = msgqx_send(msgq, &msg);
	DPRINTF("Message sent for changing all monitoring states to %d\n",
		enable);

	if(ret_val){
		fprintf(stderr, "Failed to send all-state-changing message %d "
			"with return value %d\n", msg.msg, ret_val);
		ret_val = 2;
	}
	
	return ret_val;
}


int pfm_trigger_user_stop()
{
	int ret_val;
	trigger_msg msg;

	if(msgq == NULL)
		return 1;

	msg.id = 0;

	msg.msg = quit_trigger;

	ret_val = msgqx_send(msgq, &msg);
	DPRINTF("Message sent for quit trigger\n");

	if(ret_val){
		fprintf(stderr, "Failed to send quit-trigger message "
			"with return value %d\n", ret_val);
		ret_val = 2;
	}
	
	return ret_val;
}


int pfm_trigger_user_cleanup()
{
	int ret_val = 0;

	if(msgq == NULL)
		return 1;

	ret_val = msgqx_close(msgq);

	if(ret_val)
		DPRINTF("Failed to close message queue for user: %d\n", 
			ret_val);

	return ret_val;
}

/*
 * Fortran interfaces
 */
void pfm_trigger_init_()
{
	pfm_trigger_user_init();
	
	return;
}

void pfm_trigger_enable_thread_(int *enable)
{
	pfm_trigger_user_enable_thread(*enable);

	return;
}

void pfm_trigger_enable_core_(int *cpu, int *enable)
{
	pfm_trigger_user_enable_core(*cpu, *enable);

	return;
}

void pfm_trigger_enable_all_(int *enable)
{
	pfm_trigger_user_enable_all(*enable);

	return;
}


void pfm_trigger_stop_()
{
	pfm_trigger_user_stop();
	
	return;
}

void pfm_trigger_cleanup_()
{
	pfm_trigger_user_cleanup();

	return;
}
