/*
 * This is static library called by monitored threads to enabled/disable their 
 * monitoring.
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
#include <errno.h>

// header files for get thread id
#include <sys/syscall.h>
#include <unistd.h>


#include "pfm_trigger.h"
#include "pfm_common.h"
#include "pfm_trigger_lib.h"

sem_t * sem;
int shm_id;
pfm_mon_states * mem;


static int pfm_user_open_trigger_sem()
{
	sem = (void *)sem_open(PFM_TRIGGER_NAME, 0);

	if(SEM_FAILED == sem){
		perror("Unable to open trigger semaphore in user program");
		return 1;
	}
		
	
	return 0;
}

static int pfm_user_open_trigger_mem()
{	
	key_t key;
	int mem_size;

	// convert the path to a system key
	key = ftok(PFM_TRIGGER_MEM_FILE, PFM_TRIGGER_MEM_PROJ_ID);

	if(key < 0){
		perror("Unable to open trigger memory key in user program");
		return 1;
	}

	// open the shared memory
	mem_size = sizeof(pfm_mon_states) + 
		sizeof(pfm_thr_mon_state) * (MAX_NUM_THREADS-1);
	shm_id = shmget(key, mem_size, S_IRUSR | S_IWUSR);
	if(shm_id < 0 && errno == EINVAL){
		// probably system wide monitoring, need new size
		mem_size = sizeof(pfm_mon_states);
		shm_id = shmget(key, mem_size, S_IRUSR | S_IWUSR);
	}
	if(shm_id < 0){
		perror("Unable to open trigger shared memory in user "
		       "program");
		return 2;
	}

	// map the shared memory to this process
	mem = (pfm_mon_states *) shmat(shm_id, NULL, 0);

	if(mem == (pfm_mon_states *)-1){
		perror("Unable to map trigger shared memory in user program");
	        return 2;
	}	
	

	return 0;
}

int pfm_user_trigger_init()
{
	int ret_val;

	ret_val = pfm_user_open_trigger_sem();
	if(ret_val != 0){
		ret_val = 1;
		goto error;
	}
	ret_val = pfm_user_open_trigger_mem();
	if(ret_val != 0){
		ret_val = 2;
		goto error;
	}

	return 0;

 error:
	sem = NULL;
	shm_id = 0;
	mem = NULL;
	return ret_val;
}

int get_tid()
{
	return syscall(SYS_gettid);
}

int pfm_user_trigger_enable_mon(int enable)
{
	int ret_val;
	int tid, t_idx;
	pfm_thr_mon_state * states;
	if(sem == NULL || mem == NULL)
		return 1;

	states = &(mem->states);
	tid = get_tid();

	if(mem->thr_cnt != 0){
		t_idx = tid % mem->thr_cnt;
		if(states[t_idx].cur_state == enable){
			DPRINTF("New monitoring state is the same as old"
				"for thread %d", tid);
			return 3;
		}
		
		states[t_idx].new_state = enable;
		states[t_idx].tid = tid;
	}
	else{
		// this is system wide monitoring
		t_idx = -1;
	}	
	
	
	DPRINTF("User program changing monitoring state for thread %d (%d) to "
		"%d\n", tid, t_idx, enable);

	ret_val = sem_post(sem);
	if(ret_val != 0){
		perror("Failed to post semaphore in user program");
		ret_val = 4;
	}

	return ret_val;
}


int pfm_user_close_trigger_mem(int destroy)
{
	if(sem == NULL || mem == NULL)
		return 1;

	if(shmdt(mem) != 0){
		perror("Unable to detach trigger shared memory in user "
		       "program");
		return 2;
	}

	if(!destroy)
		return 0;

	if(shmctl(shm_id, IPC_RMID, 0) == -1){
		perror("Unable to close trigger share memory object in user "
		       "program");
		return 3;
	}

	return 0;
}

int pfm_user_close_trigger_sem(int destroy)
{
	if(sem == NULL || mem == NULL)
		return 1;
	
	if(sem_close(sem) != 0){
		perror("Unable to close trigger semaphore in user program");
		return 2;
	}
	
	if(!destroy)
		return 0;

	if(sem_unlink(PFM_TRIGGER_NAME) != 0){
		perror("Unable to unlink trigger semaphore in user program");
		return 3;
	}

	return 0;
}

int pfm_user_trigger_cleanup(int destroy)
{
	int ret_val = 0;

	ret_val = pfm_user_close_trigger_mem(destroy);
	if(ret_val != 0)
		ret_val += 1;
	
	ret_val = pfm_user_close_trigger_sem(destroy);
	if(ret_val != 0)
		ret_val += 2;

	return ret_val;
}

/*
 * Fortran interfaces
 */
void pfm_trigger_init_()
{
	pfm_user_trigger_init();
	
	return;
}

void pfm_trigger_enable_()
{
	pfm_user_trigger_enable_mon(1);

	return;
}


void pfm_trigger_disable_()
{
	pfm_user_trigger_enable_mon(0);

	return;
}

void pfm_trigger_cleanup_()
{
	pfm_user_trigger_cleanup(0);

	return;
}
