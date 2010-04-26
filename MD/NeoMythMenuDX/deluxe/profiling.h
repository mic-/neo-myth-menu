#ifndef __profiling__h__
#define __profiling__h__

/*profiling?*/
#undef __DO_PROFILING__

/*profiling begin*/
#ifdef __DO_PROFILING__
	static int profiling_time;
	static char __attribute__((aligned(16))) profiling_fn_buf[64];
	static char __attribute__((aligned(16))) profiling_res_buf[256];

	#define profiling_result profiling_res_buf

	#define profiling_begin(__FUNC__)\
	{\
		profiling_res_buf[0] = '\0';\
		utility_strcpy(profiling_fn_buf,__FUNC__);\
		ints_on();\
		Z80_THREAD_IDLE();\
		Z80_THREAD_TIMING();\
		profiling_time = 0;\
	}

	#define FRAC_BITS ( 16 - 10 )
	#define FIX16b(__V__)((short int) ((__V__) * (1 << FRAC_BITS)))

	#define profiling_end()\
	{\
		Z80_THREAD_RESULT(profiling_time);\
		Z80_THREAD_IDLE();\
		utility_strcpy(profiling_res_buf,profiling_fn_buf);\
		sprintf(profiling_fn_buf,"[%d][%d]",(profiling_time/100) > FIX16b(0.5) ?(profiling_time/1000) + 1 :(profiling_time/1000) ,profiling_time );\
		utility_strcat(profiling_res_buf,profiling_fn_buf);\
		profiling_fn_buf[0] = '\0';\
	}

	#define do_profiling(__FUNCNAME__,__FUNC__)\
	{\
		profiling_begin(__FUNCNAME__);\
		__FUNC__;\
		profiling_end();\
	}

	#define do_profilingPrint(__FUNCNAME__,__FUNC__,__X__,__Y__)\
	{\
		profiling_begin(__FUNCNAME__);\
		__FUNC__;\
		profiling_end();\
		printToScreen(profiling_result,__X__,__Y__,0);\
	}

#else
	#define profiling_result
	#define profiling_begin(__FUNC__)
	#define profiling_end()
	#define do_profiling(__FUNCNAME__,__FUNC__) __FUNC__;
	#define do_profilingPrint(__FUNCNAME__,__FUNC__,__X__,__Y__) __FUNC__;
#endif
/*profiling end*/

#endif


