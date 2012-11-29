//
// MATLAB Compiler: 4.16 (R2011b)
// Date: Tue Feb 21 22:47:35 2012
// Arguments: "-B" "macro_default" "-W" "cpplib:libopf" "-T" "link:lib"
// "../opf.m" 
//

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
                                    560152);
        if (ctfStream) {
            bResult = mclInitializeComponentInstanceEmbedded(   &_mcr_inst,
                                                                error_handler, 
                                                                print_handler,
                                                                ctfStream, 
                                                                560152);
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

LIB_libopf_CPP_API 
void MW_CALL_CONV opf(int nargout, mwArray& busout, mwArray& genout, mwArray& branchout, 
                      mwArray& f, mwArray& success, mwArray& info, mwArray& et, mwArray& 
                      g, mwArray& jac, mwArray& xr, mwArray& pimul, const mwArray& 
                      varargin)
{
  mclcppMlfFeval(_mcr_inst, "opf", nargout, 11, -1, &busout, &genout, &branchout, &f, &success, &info, &et, &g, &jac, &xr, &pimul, &varargin);
}

LIB_libopf_CPP_API 
void MW_CALL_CONV opf(int nargout, mwArray& busout, mwArray& genout, mwArray& branchout, 
                      mwArray& f, mwArray& success, mwArray& info, mwArray& et, mwArray& 
                      g, mwArray& jac, mwArray& xr, mwArray& pimul)
{
  mclcppMlfFeval(_mcr_inst, "opf", nargout, 11, 0, &busout, &genout, &branchout, &f, &success, &info, &et, &g, &jac, &xr, &pimul);
}
