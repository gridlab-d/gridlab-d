typedef struct s_data {
	void *data; // pointer to the data itself
	void *info; // pointer to the property info
} DATA;
#ifdef WIN32
__declspec(dllexport)
#endif
int myfunction(int nlhs, DATA *plhs, int nrhs, DATA *prhs)
{
	if ( nlhs==1 && nrhs==1 )
	{	// assume data is double
		*(double*)(plhs[0].data) = *(double*)(prhs[0].data) - 5;
		return 0;
	}
	else
		return -1;
}