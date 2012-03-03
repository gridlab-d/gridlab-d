#ifdef WIN32
__declspec(dllexport)
#endif
int myfunction(int nlhs, double *plhs[], int nrhs, double *prhs[])
{
   if ( nlhs==1 && nrhs==1 )
   {
     *plhs[0] = *prhs[0] - 5;
     return 0;
   }
   else
     return -1;
}