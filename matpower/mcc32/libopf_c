/*
 * MATLAB Compiler: 4.15 (R2011a)
 * Date: Tue Jun 14 14:54:25 2011
 * Arguments: "-B" "macro_default" "-W" "lib:libopf" "opf.m" "-d" "lib" 
 */

#include <stdio.h>
#define EXPORTING_libopf 1
#include "libopf.h"

static HMCRINSTANCE _mcr_inst = NULL;


#ifdef __cplusplus
extern "C" {
#endif

static int mclDefaultPrintHandler(const char *s)
{
  return mclWrite(1 /* stdout */, s, sizeof(char)*strlen(s));
}

#ifdef __cplusplus
} /* End extern "C" block */
#endif

#ifdef __cplusplus
extern "C" {
#endif

static int mclDefaultErrorHandler(const char *s)
{
  int written = 0;
  size_t len = 0;
  len = strlen(s);
  written = mclWrite(2 /* stderr */, s, sizeof(char)*len);
  if (len > 0 && s[ len-1 ] != '\n')
    written += mclWrite(2 /* stderr */, "\n", sizeof(char));
  return written;
}

#ifdef __cplusplus
} /* End extern "C" block */
#endif

/* This symbol is defined in shared libraries. Define it here
 * (to nothing) in case this isn't a shared library. 
 */
#ifndef LIB_libopf_C_API
#define LIB_libopf_C_API /* No special import/export declaration */
#endif

LIB_libopf_C_API 
bool MW_CALL_CONV libopfInitializeWithHandlers(
    mclOutputHandlerFcn error_handler,
    mclOutputHandlerFcn print_handler)
{
    int bResult = 0;
  if (_mcr_inst != NULL)
    return true;
  if (!mclmcrInitialize())
    return false;
    {
        mclCtfStream ctfStream = 
            mclGetEmbeddedCtfStream((void *)(libopfInitializeWithHandlers), 
                                    168901);
        if (ctfStream) {
            bResult = mclInitializeComponentInstanceEmbedded(   &_mcr_inst,
                                                                error_handler, 
                                                                print_handler,
                                                                ctfStream, 
                                                                168901);
            mclDestroyStream(ctfStream);
        } else {
            bResult = 0;
        }
    }  
    if (!bResult)
    return false;
  return true;
}

LIB_libopf_C_API 
bool MW_CALL_CONV libopfInitialize(void)
{
  return libopfInitializeWithHandlers(mclDefaultErrorHandler, mclDefaultPrintHandler);
}

LIB_libopf_C_API 
void MW_CALL_CONV libopfTerminate(void)
{
  if (_mcr_inst != NULL)
    mclTerminateInstance(&_mcr_inst);
}

LIB_libopf_C_API 
long MW_CALL_CONV libopfGetMcrID() 
{
  return mclGetID(_mcr_inst);
}

LIB_libopf_C_API 
void MW_CALL_CONV libopfPrintStackTrace(void) 
{
  char** stackTrace;
  int stackDepth = mclGetStackTrace(&stackTrace);
  int i;
  for(i=0; i<stackDepth; i++)
  {
    mclWrite(2 /* stderr */, stackTrace[i], sizeof(char)*strlen(stackTrace[i]));
    mclWrite(2 /* stderr */, "\n", sizeof(char)*strlen("\n"));
  }
  mclFreeStackTrace(&stackTrace, stackDepth);
}


LIB_libopf_C_API 
bool MW_CALL_CONV mlxOpf(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[])
{
  return mclFeval(_mcr_inst, "opf", nlhs, plhs, nrhs, prhs);
}

LIB_libopf_C_API 
bool MW_CALL_CONV mlfOpf(int nargout, mxArray** busout, mxArray** genout, mxArray** 
                         branchout, mxArray** f, mxArray** success, mxArray** info, 
                         mxArray** et, mxArray** g, mxArray** jac, mxArray** xr, 
                         mxArray** pimul, mxArray* varargin)
{
  return mclMlfFeval(_mcr_inst, "opf", nargout, 11, -1, busout, genout, branchout, f, success, info, et, g, jac, xr, pimul, varargin);
}
