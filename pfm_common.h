/* 
 * This file includes common macros used by pfm_multi
 * 
 * Author: Wei Wang <wwang@virginia.edu>
 */
#ifndef __PFM_MULTI_COMMON_H__
#define __PFM_MULTI_COMMON_H__

#define MAX_NUM_THREADS 512 /* maximum number of threads that we can handle */
#define MAX_NUM_CORES 512 /*maximum number of cores that we can handle */

/* debug logging functions */
#ifdef __PFM_MULTI_DEBUG__
#define DPRINTF(fmt, ...) \
  do { fprintf(stderr, "pfm_multi debug: " fmt , ## __VA_ARGS__);} while (0);
#else
#define DPRINTF(fmt, ...) \
  do {} while(0);
#endif

#endif
