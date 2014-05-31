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

// header files used by semaphore 
#include <fcntl.h>    
#include <sys/stat.h> 
#include <semaphore.h>

// header files used by shared memory
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "pfm_trigger.h"
#include "pfm_common.h"
#include "pfm_operations.h"

int pfm_open_trigger_sem(pfm_trigger_info * trigger)
{
	trigger->sem = (void *)sem_open(PFM_TRIGGER_NAME, O_CREAT, 
					S_IRUSR | S_IWUSR, 0);

	if(SEM_FAILED == trigger->sem){
		perror("Unable to open trigger semaphore");
		exit(1);
	}
		
	
	return 0;
}

int pfm_open_trigger_mem(pfm_trigger_info * trigger, int is_sys_wide_mon)
{	
	key_t key;
	int mem_size;

	// convert the path to a system key
	key = ftok(PFM_TRIGGER_MEM_FILE, PFM_TRIGGER_MEM_PROJ_ID);

	if(key < 0){
		perror("Unable to open trigger memory key");
		exit(1);
	}

	// create the shared memory
	if(!is_sys_wide_mon)
		mem_size = sizeof(pfm_mon_states) + 
			sizeof(pfm_thr_mon_state) * (MAX_NUM_THREADS-1);
	else
		// if system wide monitoring, then we do not need thread states
		mem_size = sizeof(pfm_mon_states);
	// create the shared memory
	trigger->shm_id = shmget(key, mem_size, 
			S_IRUSR | S_IWUSR | IPC_CREAT);
	if(trigger->shm_id < 0){
		perror("Unable to create trigger shared memory");
		exit(1);
	}

	// map the shared memory to this process
	trigger->mem = (pfm_mon_states *) shmat(trigger->shm_id, NULL, 0);

	if(trigger->mem == (pfm_mon_states *)-1){
		perror("Unable to map trigger shared memory");
		exit(1);
	}	
	

	return 0;
}

int pfm_trigger_init_mem(pfm_trigger_info * trigger, int is_sys_wide_mon)
{
	pfm_thr_mon_state * states = &(trigger->mem->states);
	int i;

	// if system wide monitoring, then we do not need thread states
	trigger->mem->thr_cnt = is_sys_wide_mon? 0 : MAX_NUM_THREADS;
	// system wide monitoring is disabled at beginning
	trigger->mem->syswide_state = 0; 
	// initialize per thread state
	for(i = 0; i < trigger->mem->thr_cnt; i++){
		// initialized to be disabled
		states[i].cur_state = states[i].new_state = 0;
	}

	return 0;
}


int pfm_close_trigger_mem(pfm_trigger_info * trigger)
{
	if(shmdt(trigger->mem) != 0){
		perror("Unable to detach trigger shared memory");
		return 1;
	}

	if(shmctl(trigger->shm_id, IPC_RMID, 0) == -1){
		perror("Unable to close trigger share memory object");
		return 2;
	}

	return 0;
}

int pfm_close_trigger_sem(pfm_trigger_info * trigger)
{
	if(sem_close((sem_t*)trigger->sem) != 0){
		perror("Unable to close trigger semaphore");
		return 1;
	}

	if(sem_unlink(PFM_TRIGGER_NAME) != 0){
		perror("Unable to unlink trigger semaphore");
		return 2;
	}

	return 0;
}

int pfm_trigger_wait_sem(pfm_trigger_info * trigger)
{
	int ret_val;
	
	ret_val = sem_wait(trigger->sem);

	if(ret_val != 0){
		perror("Waiting on trigger semaphore failed");
		return 1;
	}
	
	DPRINTF("Trigger semaphore posted\n");

	return 0;
}

int pfm_trigger_enable(int is_sys_wide, void * options, 
		       pfm_trigger_info * trigger)
{
	int i;
	pfm_thr_mon_state * states = &(trigger->mem->states);
	
	if(is_sys_wide){
		// revert system-wide monitoring state
		trigger->mem->syswide_state = !(trigger->mem->syswide_state);
		pfm_enable_mon_core(options, trigger->mem->syswide_state);
	}
	else{
		for(i = 0; i < trigger->mem->thr_cnt; i++){
			if(states[i].cur_state != states[i].new_state){
				DPRINTF("New state found for thread %d (%d) to "
					"%d \n",
					states[i].tid, i, states[i].new_state);
				states[i].cur_state = states[i].new_state;
				pfm_enable_mon_thread(options, states[i].tid,
						      states[i].new_state);
			}
		}
	}

	return 0;
	
}
