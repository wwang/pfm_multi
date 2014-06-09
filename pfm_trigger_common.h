/*
 * Common declaration shared by the pfm_trigger module in pfm_multi and in the 
 * pfm_trigger lib used by monitored programs.
 *
 * Author: Wei Wang <wwang@virginia.edu>
 */

#ifndef __PFM_TRIGGER_COMMON_H__
#define __PFM_TRIGGER_COMMON_H__

#define PFM_TRIGGER_MSG_NAME "PFMTRIGGERMSGQ"

// message types
typedef enum _pfm_trigger_msg_type{
	thr_enable,
	thr_disable,
	cpu_enable,
	cpu_disable,
	all_enable,
	all_disable,
	quit_trigger
}trigger_msg_ty;


// a enabling/disabling message
typedef struct _pfm_trigger_msg{
	int id; //thread id or cpu id
	trigger_msg_ty msg;
}trigger_msg;



#endif
