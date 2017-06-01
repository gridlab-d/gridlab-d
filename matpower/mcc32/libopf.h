//
// MATLAB Compiler: 4.16 (R2011b)
// Date: Tue Feb 21 22:47:35 2012
// Arguments: "-B" "macro_default" "-W" "cpplib:libopf" "-T" "link:lib"
// "../opf.m" 
//

#ifndef __libopf_h
#define __libopf_h 1

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

#ifdef EXPORTING_libopf
#define PUBLIC_libopf_C_API __global
#else
#define PUBLIC_libopf_C_API /* No import statement needed. */
#endif

#define LIB_libopf_C_API PUBLIC_libopf_C_API

#elif defined(_HPUX_SOURCE)

#ifdef EXPORTING_libopf
#define PUBLIC_libopf_C_API __declspec(dllexport)
#else
#define PUBLIC_libopf_C_API __declspec(dllimport)
#endif

#define LIB_libopf_C_API PUBLIC_libopf_C_API


#else

#define LIB_libopf_C_API

#endif

/* This symbol is defined in shared libraries. Define it here
 * (to nothing) in case this isn't a shared library. 
 */
#ifndef LIB_libopf_C_API 
#define LIB_libopf_C_API /* No special import/export declaration */
#endif

extern LIB_libopf_C_API 
bool MW_CALL_CONV libopfInitializeWithHandlers(
       mclOutputHandlerFcn error_handler, 
       mclOutputHandlerFcn print_handler);

extern LIB_libopf_C_API 
bool MW_CALL_CONV libopfInitialize(void);

extern LIB_libopf_C_API 
void MW_CALL_CONV libopfTerminate(void);



extern LIB_libopf_C_API 
void MW_CALL_CONV libopfPrintStackTrace(void);

extern LIB_libopf_C_API 
bool MW_CALL_CONV mlxOpf(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[]);

extern LIB_libopf_C_API 
long MW_CALL_CONV libopfGetMcrID();


#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

/* On Windows, use __declspec to control the exported API */
#if defined(_MSC_VER) || defined(__BORLANDC__)

#ifdef EXPORTING_libopf
#define PUBLIC_libopf_CPP_API __declspec(dllexport)
#else
#define PUBLIC_libopf_CPP_API __declspec(dllimport)
#endif

#define LIB_libopf_CPP_API PUBLIC_libopf_CPP_API

#else

#if !defined(LIB_libopf_CPP_API)
#if defined(LIB_libopf_C_API)
#define LIB_libopf_CPP_API LIB_libopf_C_API
#else
#define LIB_libopf_CPP_API /* empty! */ 
#endif
#endif

#endif

extern LIB_libopf_CPP_API void MW_CALL_CONV opf(int nargout, mwArray& busout, mwArray& genout, mwArray& branchout, mwArray& f, mwArray& success, mwArray& info, mwArray& et, mwArray& g, mwArray& jac, mwArray& xr, mwArray& pimul, const mwArray& varargin);

extern LIB_libopf_CPP_API void MW_CALL_CONV opf(int nargout, mwArray& busout, mwArray& genout, mwArray& branchout, mwArray& f, mwArray& success, mwArray& info, mwArray& et, mwArray& g, mwArray& jac, mwArray& xr, mwArray& pimul);

#endif
#endif
