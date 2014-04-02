Why I am doing this...

"pfm_multi" is a remake of the old "pfmon" tool form libpfm2. "pfmon" was a very useful tool. Unfortunately, it disappeared after the introduction of kernel PMU support. Newer tools, such as "perf" or "operf", are supposed to replace "pfmon". However, they miss certain features I need. "perf" does not support per-thread monitoring. "operf" simply did not exist when I needed it. And it is kind hard to configure it to produce result every second, which is required for my research on memory systems. Also, I want something light, simple, easy to build and install.


Compiling pfm_multi:

First you need a Linux box~ You will also need libpfm4, perf_util.c form libpfm4, and some helper functions from https://github.com/wwang/common_toolx. 

Here are the steps:
   1. Get libpfm4, build it.
   2. Link or copy the perf_util.h/c from libpfm4 to pfm_multi directory.
   3. Edit the Makefile of pfm_multi: change the path of the header files and 
      libraries of libpfm4 and common_toolx. Also, I statically link everything  
      in my Makefile. This is a strange habit of mine because I do performance 
      analysis. Remove the "-static" if you want.
   4. Run "make all". 
   5. I do not have "install" here, just copy the "pfm_multi" binary to desired 
      directory (with libpfm4.so if necessary).


A quick note on usage:

usage: pfm_multi [-h] [-C] [-c cpu]  [-i interval] [-g] [-p] [-e event1,event2,...] cmd parameters
-h		show this help
-i INTERVAL	print counts every INTERVAL nanoseconds
-g		group events
-p		pin events to cpu
-C		system wide monitoring (per-core instead of per-thread), all cores 
		are monitored if not specified by -c
-c CORE,CORE...	cores to monitor (comma separated list), must be used with -C
-e "ev,ev"	group of events to measure (multiple -e switches are allowed); 
   		get the list of supported events from showevtinfo of libpfm4
cmd parameters  this is the program and its parameters you want to monitor


If you have questions or comments, please contact me at wwang at virginia dot edu
