// $Id: modified_euler.cpp 4738 2014-07-03 00:55:39Z dchassin $
// Copyright (C) 2012 Battelle Memorial Institute
// Modified Euler Integration Method

#include <stdlib.h>
#include "gridlabd.h"

static unsigned int mod_eul_solver_version = 1; // change this each time to data structure changes
typedef struct {
	int number_y_variables;				//Dimension of y-variables to functions (defaults to 1)
	int number_x_variables;				//Dimension of x-variables to functions (defaults to 1)
	double *x;							//Initial values of x
	double *x_next;						//Timestep values of x
	double *y;							//Initial values of y
	double *y_intermed;					//Intermediate values of y - for solver
	double *y_first_deriv;				//First derivative of the solver - f'(X_0,Y_0)
	double *y_second_deriv;				//Second derivative of the solver - f'(X_0+dT,Y_intermed)
	double *y_next;						//Next timestep values of y
	double deltaT;						//Timestep value for integration (defaults to 1 second)
	unsigned char (*deriv_function)(double *,double *, double *, double, double);		//Pointer to the derivative function - func(x,y,y',xsze,ysze) (status return 0 = fail, 1 = success)
	unsigned char status_val;			//Status (0=failed, 1=success)
} MEULERDATA;

EXPORT int modified_euler_init(CALLBACKS *fntable)
{
	callback = fntable;
	return 1;
}

EXPORT int modified_euler_set(char *param, ...)
{
	va_list arg;
	va_start(arg,param);
	char *tag = param;

	if (tag!=NULL)
	{
		gl_error("modified_euler_set(char *param='%s',...): tag '%s' is not recognized - no input parameters expected!",param,tag);
		/*  TROUBLESHOOT
		The modified Euler integration solver does not expect any input parameters for setup.  Please pass just a NULL argument
		and set all relevant parameters in the input structure.
		*/
		return 0;
	}
	else
		return 1;
}

EXPORT int modified_euler_get(char *param, ...)
{
	int n=0;
	int loopvar;
	int variable_x_size = 1;
	int variable_y_size = 1;
	va_list arg;
	va_start(arg,param);
	char *tag = param;
	while ( tag!=NULL )
	{
		if ( strcmp(param,"solver_version")==0 )
			*va_arg(arg,unsigned int*) = mod_eul_solver_version;
		else if (strcmp(param,"number_x_variables")==0)	//Get x-variable size
			variable_x_size = va_arg(arg,int);
		else if (strcmp(param,"number_y_variables")==0)	//Get y-variable size
			variable_y_size = va_arg(arg,int);
		else if ( strcmp(param,"init")==0 )
		{
			//Initialize the structure
			MEULERDATA *data = (MEULERDATA*)va_arg(arg,MEULERDATA*);

			//Set initial values
			data->number_x_variables = variable_x_size;
			data->number_y_variables = variable_y_size;

			//Allocate the arrays
			data->x = (double*)malloc(sizeof(double)*variable_x_size);				//X_0 variables
			if (data->x==NULL)
			{
				gl_error("modified_euler_get:init - failed to allocate storage array");
				/*  TROUBLESHOOT
				While attempting to allocate space for one of the variable storage arrays
				in the modified Euler solver, an error was encountered.  Please try again.
				If the error persists, please submit your code and a bug report via the trac website.
				*/
				return n;
			}

			data->x_next = (double*)malloc(sizeof(double)*variable_x_size);			//X_0 + delta_T variables
			if (data->x_next==NULL)
			{
				gl_error("modified_euler_get:init - failed to allocate storage array");
				//TS defined above
				return n;
			}

			data->y = (double*)malloc(sizeof(double)*variable_y_size);				//Y_0 variables
			if (data->y==NULL)
			{
				gl_error("modified_euler_get:init - failed to allocate storage array");
				//TS defined above
				return n;
			}

			data->y_intermed = (double*)malloc(sizeof(double)*variable_y_size);		//Y_i - Y_0 + f'(X_0,Y_0)*delta_T
			if (data->y_intermed==NULL)
			{
				gl_error("modified_euler_get:init - failed to allocate storage array");
				//TS defined above
				return n;
			}

			data->y_first_deriv = (double*)malloc(sizeof(double)*variable_y_size);		//f'(X_0,Y_0)
			if (data->y_first_deriv==NULL)
			{
				gl_error("modified_euler_get:init - failed to allocate storage array");
				//TS defined above
				return n;
			}

			data->y_second_deriv = (double*)malloc(sizeof(double)*variable_y_size);		//f'(X_0+dT,Y_i)
			if (data->y_second_deriv==NULL)
			{
				gl_error("modified_euler_get:init - failed to allocate storage array");
				//TS defined above
				return n;
			}

			data->y_next = (double*)malloc(sizeof(double)*variable_y_size);			//Y_1 - expected next output
			if (data->y_next==NULL)
			{
				gl_error("modified_euler_get:init - failed to allocate storage array");
				//TS defined above
				return n;
			}

			//Loop and zero x-values
			for (loopvar=0; loopvar<variable_x_size; loopvar++)
			{
				data->x[loopvar] = 0.0;
				data->x_next[loopvar] = 0.0;
			}

			//Loop and zero y-values
			for (loopvar=0; loopvar<variable_y_size; loopvar++)
			{
				data->y[loopvar] = 0.0;
				data->y_first_deriv[loopvar] = 0.0;
				data->y_intermed[loopvar] = 0.0;
				data->y_next[loopvar] = 0.0;
				data->y_second_deriv[loopvar] = 0.0;
			}

			data->deltaT = 1.0;				//Defaults to 1 second
			data->deriv_function = (unsigned char(*)(double *, double *, double *,double,double))malloc(sizeof(void*));
			if (data->deriv_function==NULL)
			{
				gl_error("modified_euler_get:init - failed to allocate derivative pointer");
				/*  TROUBLESHOOT
				While attempting to allocate space for derivative function pointer
				in the modified Euler solver, an error was encountered.  Please try again.
				If the error persists, please submit your code and a bug report via the trac website.
				*/
				return 0;
			}

			data->status_val = 0;			//Assume a failure by default
		}
		else
		{
			gl_error("modified_euler_get(char *param='%s',...): tag '%s' is not recognized",param,tag);
			/*  TROUBLESHOOT
			An undexpected parameter was encountered while parsing the modified_euler_get call.  Please ensure
			your parameter specifications are spelled correctly.
			*/
			return n;
		}
		tag = va_arg(arg,char*);
		n++;
	}
	return n;
}

EXPORT int modified_euler_solve(MEULERDATA *data)
{
	int indexval;
	unsigned char returnval;
	unsigned char (*dfx)(double *, double *, double *,double,double) = data->deriv_function;	//Make the function easier to call

	//Get current derivative - f'(X_0,Y_0)
	returnval = (*dfx)(data->x,data->y,data->y_first_deriv,data->number_x_variables,data->number_y_variables);

	//Check it
	if (returnval != 1)
	{
		gl_error("modified_euler_solve - the derivative function did not return success");
		/*  TROUBLESHOOT
		While performing the derivative calculation for the modified Euler method,
		the derivative function did not return a success flag.  Ensure your function
		is set to properly return this flag, or debug the derivative function itself.
		*/
		return 0;
	}

	//Create y-intermediate
	for (indexval=0; indexval<data->number_y_variables; indexval++)
	{
		//Y_intermed = Y_0 + f'(X_0,Y_0)*dT
		data->y_intermed[indexval] = data->y[indexval] + data->y_first_deriv[indexval]*data->deltaT;
	}

	//Get second derivative - f'(X_0+dT,Y_intermed)
	returnval = (*dfx)(data->x_next,data->y_intermed,data->y_second_deriv,data->number_x_variables,data->number_y_variables);

	//Check it
	if (returnval != 1)
	{
		gl_error("modified_euler_solve - the derivative function did not return success");
		//Defined above
		return 0;
	}

	//Put it into the final answer
	for (indexval=0; indexval<data->number_y_variables; indexval++)
	{
		//Y_final = Y_0 + 0.5*[f'(X_0,Y_0)+f'(X_0+dT,Y_intermed)]*dT
		data->y_next[indexval] = data->y[indexval] + 0.5*(data->y_first_deriv[indexval] + data->y_second_deriv[indexval])*data->deltaT;
	}

	//We're always successful for now
	data->status_val = 1;

	//Return the status
	return data->status_val;
}


