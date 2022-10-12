/**
 * @file cblock.h
 * @brief Header file defining the public API for linear control block
 *
 */

#ifndef CBLOCK_H
#define CBLOCK_H

#include <math.h>
#include <algorithm>

/**
  DELTAMODESTAGE - Stage in Delta mode calculation

  Enum to specify delta mode calculation stage.
*/
typedef enum {
  PREDICTOR, // Predictor update
  CORRECTOR  // Corrector update
}DeltaModeStage;


/*
  Linear control block base class - This is the base class for linear blocks. All 
  control blocks inherit from this class. It expresses a first order linear transfer
  function in observable canonical form to obtain a first order state-space 
  representation. The state x and output y can be bounded by limits.

  Linear control block:
  input  : u 
  output : y
  state  : x
      
                 xmax
                ----
               /             ymax
              /             -----
        -------------      /
        | b0s + b1  |     /
  u ----| --------- |----------- y
        | a0s + a1  |    /
        -------------   /
             /       ---
            /        ymin
        ----
        xmin


  For a first order transfer function, 

  Y(s)    b0s + b1
  ----- = ----------
  U(s)    a0s + a1

  The equivalent state-space representation in the observable canonical form is given by

  dx_dt = Ax + Bu
    y   = Cx + Du

  here u,x,y \in R^1 

  A = -a1/a0, B = (b1 - a1b0)/a0, C = 1, D = b0/a0

  Output:
   y = Cx + D*u,  ymin <= y <= ymax

*/ 
class Cblock
{
 protected:
  double p_A[1]; /* A */
  double p_B[1]; /* B */
  double p_C[1]; /* C */
  double p_D[1]; /* D */

  double p_dxdt[1];   /* State derivative */
  double p_xhat[1];   /* Predictor stage x */

  // p_order is kept for future extensions if and
  // when the order of the transfer function > 1
  int    p_order; /* order of the control block */

  double p_xmax,p_xmin; /* Max./Min. limits on state X */
  double p_ymax,p_ymin; /* Max./Min. limits on output Y */

  /**
     UPDATESTATE - Updates the linear control block state variable

     Inputs:
       u               Input to the control block
       dt              Integration time-step
       DeltaModeStage  Stage of delta mode calculation, PREDICTOR or CORRECTOR

     Output:
       x               Control state variable
                         x = \hat{x}_{n+1} for PREDICTOR stage
			 x = x_{n+1}       for CORRECTOR stage

     Note: State update calculation
       PREDICTOR (Forward Euler):

         \hat{x}_{n+1} = x_{n} + dt*dx_dt(x_{n},u_{n})

       CORRECTOR (Trapezoidal):

         x_{n+1} = x_{n} + 0.5*dt*(dx_dt(x_{n}) + dx_dt(\hat{x}_{n+1},u_{n+1}))


    Note here that GridLab-D does a network solve after every predictor/corrector
    call. So, during the corrector stage the input u is updated (u_{n+1}) 
  **/
  double updatestate(double u, double dt,DeltaModeStage stage);

  /**
     UPDATESTATE - Update linear control block state variable enforcing limits

     Inputs:
       u               Input to the control block
       dt              Integration time-step
       xmin            Min. limiter for state x
       xmax            Max. limiter for state x
       DeltaModeStage  Stage of delta mode calculation, PREDICTOR or CORRECTOR

     Output:
       x               Control state variable
                         x = \hat{x}_{n+1} for PREDICTOR stage
			 x = x_{n+1}       for CORRECTOR stage

     Note: State update calculation
       PREDICTOR (Forward Euler):

         \hat{x}_{n+1} = x_{n} + dt*dx_dt(x_{n},u_{n})

	 \hat{x}_{n+1} = max(xmin,min(\hat{x}_{n+1},xmax)

       CORRECTOR (Trapezoidal):

         x_{n+1} = x_{n} + 0.5*dt*(dx_dt(x_{n}) + dx_dt(\hat{x}_{n+1},u_{n+1})) 

	 x_{n+1} = max(xmin,min(x_{n+1},xmax)

    Note here that GridLab-D does a network solve after every predictor/corrector
    call. So, during the corrector stage the input u is updated (u_{n+1}) 
  **/
  double updatestate(double u, double dt,double xmin, double xmax, DeltaModeStage stage);

  /**
     GETDERIVATIVE - Returns the time derivative of the linear control block state variable

     Inputs:
       x          State variable
       u          Control block input

     Outputs:
       dx_dt      State derivative
  **/
  double getderivative(double x,double u);

 public:
  Cblock();

  // This is made public so that it can be accessed via
  // PADDR() method
  double x[1];      /* State variable x */

  /**
     SETCOEFFS - Sets the coefficients a,b for the control block transfer function

     Inputs:
       a           An array of size 2,[a0,a1] for setting coefficients for the denominator
       b           An array of size 2,[b0,b1] for setting coefficients for the numerator

     Notes:
       The transfer function for the linear control block is expressed in the form
       Y(s)   b0*s + b1
       --- = -----------
       U(s)   a0*s + a1

       The user is expected to provide the coefficients for the transfer function in arrays a and b

       As an example, let's assume the transfer function is (2s + 3)/(s + 2), then the arrays a and b
       passed to setcoeffs would be a = [1,2] and b = [2,3]
  **/
  void setcoeffs(double *a,double *b);


  /**
     SETXLIMITS - Sets limits for the state variable x

     Inputs:
       xmin          Min. limit for x
       xmax          Max. limit for x
  **/
  void setxlimits(double xmin, double xmax);

  /**
     SETYLIMITS - Sets limits for output y

     Inputs:
       ymin          Min. limit for y
       ymax          Max. limit for y
  **/
  void setylimits(double ymin, double ymax);

  /**
     INIT - Initializes the control block - calculates x[0]

     Inputs:
       u           Control block input u (u[0])
       y           Control block (expected) output y (y[0])
  **/
  void init(double u, double y);

  /**
     GETOUPUT - Returns output y of the control block

     Inputs:
       u               Input to the control block
       dt              Integration time-step
       DeltaModeStage  Stage of delta mode calculation, PREDICTOR or CORRECTOR

     Output:
       y               Control block output

     Note: Output calculation
       PREDICTOR :
	 y_{n+1} = C\hat{x}_{n+1} + Du_{n}

       CORRECTOR :

	 y_{n+1} = Cx_{n+1} + Du_{n+1}

    Note here that GridLab-D does a network solve after every predictor/corrector
    call. So, during the corrector stage the input u is updated (u_{n+1}) 
  **/
  double getoutput(double u,double dt,DeltaModeStage stage);

  /**
     GETOUTPUT - Returns control block output y enforcing limits on state and output.

     Inputs:
       u               Input to the control block
       dt              Integration time-step
       xmin            Min. limit for state variable
       xmax            Max. limit for state variable
       ymin            Min. limit for output y
       ymax            Max. limit for output y
       DeltaModeStage  Stage of delta mode calculation, PREDICTOR or CORRECTOR

     Output:
       y               Control block output

     Note: Output calculation
       PREDICTOR :

	 y_{n+1} = C\hat{x}_{n+1} + Du_{n}

	 y_{n+1} = max(ymin,min(y_{n+1},ymax)

       CORRECTOR :
	 y_{n+1} = Cx_{n+1} + Du_{n+1}

	 y_{n+1} = max(ymin,min(y_{n+1},ymax)

    Note here that GridLab-D does a network solve after every predictor/corrector
    call. So, during the corrector stage the input u is updated (u_{n+1}) 
  **/
  double getoutput(double u,double dt,double xmin, double xmax, double ymin, double ymax, DeltaModeStage stage);

  /**
     GETSTATE - Returns the internal state variable x for the control block

     Input:
       stage          Stage of delta mode calculation, PREDICTOR or CORRECTOR

     Output:
       x              Control block state variable
  **/
  double getstate(DeltaModeStage stage);

  ~Cblock(void);
};

/*
  PI control block:
  input  : u 
  output : y
  state  : x (integrator)
      
                 xmax
                ----
               /             ymax
              /             -----
        -------------      /
        |           |     /
  u ----| Kp + Ki/s |----------- y
        |           |    /
        -------------   /
             /       ---
            /        ymin
        ----
        xmin

   Differential equation:
       dx_dt = Ki*u

   Output:
    y = x + Kp*u,  ymin <= y <= ymax

*/
class PIControl: public Cblock
{
 public:
  PIControl();

  /**
     SETPARAMS - Set the PI controller gains

     INPUTS:
       Kp         Proportional gain
       Ki         Integral gain
  **/
  void setparams(double Kp, double Ki);

  /**
     SETPARAMS - Set the PI controller gains and limits

     INPUTS:
       Kp         Proportional gain
       Ki         Integral gain
       xmin       Min. limit for state variable
       xmax       Max. limit for state variable
       ymin       Min. limit for output y
       ymax       Max. limit for output y
  **/
  void setparams(double Kp, double Ki,double xmin,double xmax,double ymin,double ymax);
};

/*
  Filter block:
  input  : u 
  output : y
  state  : x
      
                 xmax
                ----
               /             ymax
              /             -----
        -------------      /
        |    1      |     /
  u ----| -------   |----------- y
        |  1 + sT   |    /
        -------------   /
             /       ---
            /        ymin
        ----
        xmin

   Differential equation:
       dx_dt = (u - x)/T

   Output:
    y = x,  ymin <= y <= ymax

*/
class Filter: public Cblock
{
 public:
  Filter();
  Filter(double T);
  Filter(double T, double xmin, double xmax,double ymin, double ymax);

  /**
     SETPARAMS - Set the filter time constant

     INPUTS:
       T         Filter time constant
  **/
  void setparams(double T);

  /**
     SETPARAMS - Set the filter time constant and limits

     INPUTS:
       T          Filter time constant
       xmin       Min. limit for state variable
       xmax       Max. limit for state variable
       ymin       Min. limit for output y
       ymax       Max. limit for output y
  **/
  void setparams(double T,double xmin,double xmax,double ymin,double ymax);
};

/*
  Integrator block:
  input  : u 
  output : y
  state  : x
                         
                           
        -------------      
        |    1      |     
  u ----| -------   |----------- y
        |   sT      |    
        -------------   
             
              
   Differential equation:
       dx_dt = u

   Output:
    y = x

*/
class Integrator: public Cblock
{
 public:
  Integrator();
  
  /**
     SETPARAMS - Set the Integrator time constant

     INPUTS:
       T         Integrator time constant
  **/
  void setparams(double T);
};


#endif
