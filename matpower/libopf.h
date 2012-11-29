//
// MATLAB Compiler: 4.18 (R2012b)
// Date: Thu Nov 29 06:52:10 2012
// Arguments: "-B" "macro_default" "-W" "cpplib:MatPowerGLD" "-T" "link:lib"
// "-d" "/root/MatPowerGLD/src" "-w" "enable:specified_file_mismatch" "-w"
// "enable:repeated_file" "-w" "enable:switch_ignored" "-w"
// "enable:missing_lib_sentinel" "-w" "enable:demo_license" "-v"
// "/root/gridlab/trunk/matpower/matpower40_src/opf.m" 
//

#ifndef __MatPowerGLD_h
#define __MatPowerGLD_h 1

#if defined(__cplusplus) && !defined(mclmcrrt_h) && defined(__linux__)
#  pragma implementation "mclmcrrt.h"
#endif
#include "mclmcrrt.h"
#include "mclcppclass.h"
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__SUNPRO_CC)
/* Solaris shared libraries use __global, rather than mapfiles
 * to define the API exported from a shared library. __global is
 * only necessary when building the library -- files including
 * this header file to use the library do not need the __global
 * declaration; hence the EXPORTING_<library> logic.
 */

#ifdef EXPORTING_MatPowerGLD
#define PUBLIC_MatPowerGLD_C_API __global
#else
#define PUBLIC_MatPowerGLD_C_API /* No import statement needed. */
#endif

#define LIB_MatPowerGLD_C_API PUBLIC_MatPowerGLD_C_API

#elif defined(_HPUX_SOURCE)

#ifdef EXPORTING_MatPowerGLD
#define PUBLIC_MatPowerGLD_C_API __declspec(dllexport)
#else
#define PUBLIC_MatPowerGLD_C_API __declspec(dllimport)
#endif

#define LIB_MatPowerGLD_C_API PUBLIC_MatPowerGLD_C_API


#else

#define LIB_MatPowerGLD_C_API

#endif

/* This symbol is defined in shared libraries. Define it here
 * (to nothing) in case this isn't a shared library. 
 */
#ifndef LIB_MatPowerGLD_C_API 
#define LIB_MatPowerGLD_C_API /* No special import/export declaration */
#endif

extern LIB_MatPowerGLD_C_API 
bool MW_CALL_CONV MatPowerGLDInitializeWithHandlers(
       mclOutputHandlerFcn error_handler, 
       mclOutputHandlerFcn print_handler);

extern LIB_MatPowerGLD_C_API 
bool MW_CALL_CONV MatPowerGLDInitialize(void);

extern LIB_MatPowerGLD_C_API 
void MW_CALL_CONV MatPowerGLDTerminate(void);



extern LIB_MatPowerGLD_C_API 
void MW_CALL_CONV MatPowerGLDPrintStackTrace(void);

extern LIB_MatPowerGLD_C_API 
bool MW_CALL_CONV mlxOpf(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[]);


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

/* On Windows, use __declspec to control the exported API */
#if defined(_MSC_VER) || defined(__BORLANDC__)

#ifdef EXPORTING_MatPowerGLD
#define PUBLIC_MatPowerGLD_CPP_API __declspec(dllexport)
#else
#define PUBLIC_MatPowerGLD_CPP_API __declspec(dllimport)
#endif

#define LIB_MatPowerGLD_CPP_API PUBLIC_MatPowerGLD_CPP_API

#else

#if !defined(LIB_MatPowerGLD_CPP_API)
#if defined(LIB_MatPowerGLD_C_API)
#define LIB_MatPowerGLD_CPP_API LIB_MatPowerGLD_C_API
#else
#define LIB_MatPowerGLD_CPP_API /* empty! */ 
#endif
#endif

#endif

extern LIB_MatPowerGLD_CPP_API void MW_CALL_CONV opf(int nargout, mwArray& busout, mwArray& genout, mwArray& branchout, mwArray& f, mwArray& success, mwArray& info, mwArray& et, mwArray& g, mwArray& jac, mwArray& xr, mwArray& pimul, const mwArray& varargin);

extern LIB_MatPowerGLD_CPP_API void MW_CALL_CONV opf(int nargout, mwArray& busout, mwArray& genout, mwArray& branchout, mwArray& f, mwArray& success, mwArray& info, mwArray& et, mwArray& g, mwArray& jac, mwArray& xr, mwArray& pimul);

#endif
#endif
