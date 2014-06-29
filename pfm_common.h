/* 
 * This file includes common macros used by pfm_multi
 * 
 * Author: Wei Wang <wwang@virginia.edu>
 */
#ifndef __PFM_MULTI_COMMON_H__
#define __PFM_MULTI_COMMON_H__

#define MAX_NUM_THREADS 512 /* maximum number of threads that we can handle */
#define MAX_NUM_CORES 512 /*maximum number of cores that we can handle */

/* utput streams; should be FILE * type actually */
extern void * reading_out;
extern void * err_out;

/* debug logging functions */
#ifdef __PFM_MULTI_DEBUG__
#define DPRINTF(fmt, ...) \
	do { fprintf((FILE*) err_out, "pfm_multi debug: " fmt , ## __VA_ARGS__);} while (0);
#else
#define DPRINTF(fmt, ...) \
	do {} while(0);
#endif

/* reading output function */
#define reading_output(fmt, ...)					\
	do { fprintf((FILE*) reading_out, fmt , ## __VA_ARGS__);} while (0);
#endif
