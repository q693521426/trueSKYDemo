#pragma once

#include <iostream>

#ifdef _WIN32
#define strerror_r(err_code, sys_msg, sizeofsys_msg) strerror_s(sys_msg, sizeofsys_msg, err_code)
#endif

#ifdef __ORBIS__
#define strerror_r(err_code, sys_msg, sizeofsys_msg) strerror_s(sys_msg, sizeofsys_msg, err_code)
#include <libdbg.h>
#endif
extern void CheckGLError();
extern void BreakIfDebugging();
#define PSFX_CERR std::cerr<<__FILE__<<"("<<__LINE__<<"): "
/// This errno check can be disabled for production.
#if 0
	#define PSFX_ERRNO_CHECK
	#define PSFX_ERRNO_BREAK
#else
	#define PSFX_ERRNO_CHECK \
		if(errno!=0)\
		{\
			char e[101];\
			strerror_r(errno,e,100);\
			PSFX_CERR<<"warning B0001: "<<"WARNING: errno!=0: "<<e<<std::endl;\
			errno=0;\
		}
	#define PSFX_ERRNO_BREAK \
		if(errno!=0)\
		{\
			char e[101];\
			strerror_r(errno,e,100);\
			PSFX_CERR<<": error B0001: "<<"WARNING: errno!=0: "<<e<<std::endl;\
			errno=0;\
			BreakIfDebugging();\
		}
#endif

