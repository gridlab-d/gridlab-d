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
  INTEGRATIONSTAGE - Stage in integration

  Enum to specify Integration calculation stage.
*/
typedef enum {
  PREDICTOR, // Predictor update
  CORRECTOR  // Corrector update
}IntegrationStage;


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

  A = -a1/a0, B = b1/a0 - a1b0/a0^2, C = 1, D = b0/a0

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
  double p_dxmax,p_dxmin; /* Rate-limiter */

  IntegrationStage p_current_stage; // The current stage

  /**
     UPDATESTATE - Updates the linear control block state variable

     Inputs:
       u               Input to the control block
       dt              Integration time-step
       IntegrationStage  Stage of integration mode calculation, PREDICTOR or CORRECTOR

     Note: State update calculation
       PREDICTOR (Forward Euler):

         \hat{x}_{n+1} = x_{n} + dt*dx_dt(x_{n},u)

       CORRECTOR (Trapezoidal):

         x_{n+1} = x_{n} + 0.5*dt*(dx_dt(x_{n},u_n) + dx_dt(\hat{x}_{n+1},u))

	 The updated state can be retrieved via getstate() method
  **/
  void updatestate(double u, double dt,IntegrationStage stage);

  /**
     UPDATESTATE - Version of UPDATESTATE enforcing limits

     Inputs:
       u               Input to the control block
       dt              Integration time-step
       xmin            Min. limiter for state x
       xmax            Max. limiter for state x
       dxmin           Min. limit for rate of change of x
       dxmax           Max. limit for rate of change of x
       IntegrationStage  Stage of integration mode calculation, PREDICTOR or CORRECTOR

       Notes:
       The updated state can be retrieved via getstate() method
  **/
  void updatestate(double u, double dt,double xmin, double xmax, double dxmin, double dxmax,IntegrationStage stage);

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
     SETYLIMITS - Sets limits for rate of change of x

     Inputs:
       dxmin          Min. limit for rate of change of x (dx_dt)
       dxmax          Max. limit for rate of change of x (dx_dt)
  **/
  void setdxlimits(double dxmin, double dxmax);


  /**
     INIT - Initializes the control block - calculates x[0]

     Inputs:
       u           Control block input u (u[0])
       y           Control block (expected) output y (y[0])
  **/
  void init(double u, double y);

  /**
     INIT_GIVEN_U - Initializes the control block - calculates x[0] given input u

     Inputs:
       u           Control block input u
     Outputs:
       y           Expected output for the control block given the input u
  **/
  double init_given_u(double u);

  /**
     INIT_GIVEN_Y - Initializes the control block - calculates x[0] given output y

     Inputs:
       y           Control block output y
     Outputs:
       u           Expected input for the control block given the output y
  **/
  double init_given_y(double y);

  /**
     GETOUPUT - Returns output y of the control block. Does not do any state update

     Inputs:
       u                 Input to the control block

     Output:
       y                 Control block output
  **/
  double getoutput(double u);

  /**
     GETOUPUT - Returns output y of the control block and updates the state

     Inputs:
       u                 Input to the control block
       dt                Integration time-step
       IntegrationStage  Stage of integration mode calculation, PREDICTOR or CORRECTOR

     Output:
       y                 Control block output

       Output calculation
       PREDICTOR :
	 y_{n+1} = Cx_{n} + Du

       CORRECTOR :
	 y_{n+1} = C\hat{x}_{n+1} + Du

	 Here, x_n is value of the state variable
	 at time instant n, and \hat{x}_{n+1} is the
	 predictor value of state variable at time instant
	 n+1
  **/
  double getoutput(double u,double dt,IntegrationStage stage);

  /**
     GETOUPUT - Returns output y of the control block and (optionally) updates the state

     Inputs:
       u                 Input to the control block
       dt                Integration time-step
       IntegrationStage  Stage of integration mode calculation, PREDICTOR or CORRECTOR
       dostateupdate     Should state variable be updated?

     Output:
       y                 Control block output

     Notes: 
     This method also can optionally update the state variable x of 
     the control block by setting dostateupdate appropriately. The optional
     update of state is helpful in situations where the controller asks to
     freeze the state OR simply get the block output without updating state 
  **/
  double getoutput(double u,double dt,IntegrationStage stage,bool dostateupdate);

  /**
     GETOUTPUT - Version of GETOUTPUT enforcing limits on state and output.
                 State is updated

     Inputs:
       u               Input to the control block
       dt              Integration time-step
       xmin            Min. limit for state variable
       xmax            Max. limit for state variable
       ymin            Min. limit for output y
       ymax            Max. limit for output y
       IntegrationStage  Stage of integration mode calculation, PREDICTOR or CORRECTOR

     Output:
       y               Control block output

  **/
  double getoutput(double u,double dt,double xmin, double xmax, double ymin, double ymax, IntegrationStage stage);

  /**
     GETOUTPUT - Version of GETOUTPUT  enforcing limits on state and output.
                 with optional state update

     Inputs:
       u               Input to the control block
       dt              Integration time-step
       xmin            Min. limit for state variable
       xmax            Max. limit for state variable
       ymin            Min. limit for output y
       ymax            Max. limit for output y
       IntegrationStage  Stage of integration mode calculation, PREDICTOR or CORRECTOR
       dostateupdate   Should state variable be updated?

     Output:
       y               Control block output

  **/
  double getoutput(double u,double dt,double xmin, double xmax, double ymin, double ymax, IntegrationStage stage, bool dostateupdate);

  /**
     GETOUTPUT - Version of GETOUTPUT  enforcing limits on state, derivative, and output
                 State is updated

     Inputs:
       u               Input to the control block
       dt              Integration time-step
       xmin            Min. limit for state variable
       xmax            Max. limit for state variable
       dxmin           Min. limit for rate of change of x (dx_dt)
       dxmax           Max. limit for rate of change of x (dx_dt)
       ymin            Min. limit for output y
       ymax            Max. limit for output y
       IntegrationStage  Stage of integration mode calculation, PREDICTOR or CORRECTOR

     Output:
       y               Control block output

  **/
  double getoutput(double u,double dt,double xmin, double xmax, double dxmin, double dxmax,double ymin, double ymax, IntegrationStage stage);

    /**
     GETOUTPUT - Version of GETOUTPUT  enforcing limits on state, derivative, and output
                 with optional update of state

     Inputs:
       u               Input to the control block
       dt              Integration time-step
       xmin            Min. limit for state variable
       xmax            Max. limit for state variable
       dxmin           Min. limit for rate of change of x (dx_dt)
       dxmax           Max. limit for rate of change of x (dx_dt)
       ymin            Min. limit for output y
       ymax            Max. limit for output y
       IntegrationStage  Stage of integration mode calculation, PREDICTOR or CORRECTOR
       dostateupdate   Should state variable be updated?

     Output:
       y               Control block output

  **/
  double getoutput(double u,double dt,double xmin, double xmax, double dxmin, double dxmax,double ymin, double ymax, IntegrationStage stage, bool dostateupdate);


  /**
     GETSTATE - Returns the internal state variable x for the control block

     Input:
       stage          Stage of integration mode calculation, PREDICTOR or CORRECTOR

     Output:
       x              Control block state variable

     Note:
       This method should be called after the state is updated, either by calling getoutput or updatestate
  **/
  double getstate(IntegrationStage stage);

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

    /**
     SETPARAMS - Set the PI controller gains, state/output limits and rate limiter

     INPUTS:
       Kp         Proportional gain
       Ki         Integral gain
       xmin       Min. limit for state variable
       xmax       Max. limit for state variable
       dxmin      Min. limit on rate of change of x
       dxmax      Max. limit on rate of change of x
       ymin       Min. limit for output y
       ymax       Max. limit for output y
  **/
  void setparams(double Kp, double Ki,double xmin,double xmax,double dxmin, double dxmax,double ymin,double ymax);

};

/*
  Lead lag control block:
  input  : u 
  output : y
  state  : x (integrator)
      
                 xmax
                ----
               /             ymax
              /             -----
        -------------      /
        | 1 + sTA   |     /
  u ----| --------  |----------- y
        | 1 + sTB   |    /
        -------------   /
             /       ---
            /        ymin
        ----
        xmin

   Differential equation:
       dx_dt = (-1/TB)*x + (1 - TA)*u/TB

   Output:
    y = x + TA/TB*u,  ymin <= y <= ymax

*/
class LeadLag: public Cblock
{
 public:
  LeadLag();

  /**
     SETPARAMS - Set the lead lag time constants

     INPUTS:
       TA         Denominator time constant
       TB         Numerator time constant
  **/
  void setparams(double TA, double TB);

  /**
     SETPARAMS - Set the Lead lag controller gains and limits

     INPUTS:
       TA         Denominator time constant
       TB         Numerator time constant
       xmin       Min. limit for state variable
       xmax       Max. limit for state variable
       ymin       Min. limit for output y
       ymax       Max. limit for output y
  **/
  void setparams(double TA, double TB,double xmin,double xmax,double ymin,double ymax);

      /**
     SETPARAMS - Set the PI controller gains, state/output limits and rate limiter

     INPUTS:
       TA         Denominator time constant
       TB         Numerator time constant
       xmin       Min. limit for state variable
       xmax       Max. limit for state variable
       dxmin      Min. limit on rate of change of x
       dxmax      Max. limit on rate of change of x
       ymin       Min. limit for output y
       ymax       Max. limit for output y
  **/
  void setparams(double TA, double TB,double xmin,double xmax,double dxmin, double dxmax,double ymin,double ymax);

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
        |    K      |     /
  u ----| -------   |----------- y
        |  1 + sT   |    /
        -------------   /
             /       ---
            /        ymin
        ----
        xmin

   Differential equation:
       dx_dt = (Ku - x)/T

   Output:
    y = x,  ymin <= y <= ymax

*/
class Filter: public Cblock
{
 public:
  Filter();
  Filter(double K,double T);
  Filter(double K,double T, double xmin, double xmax,double ymin, double ymax);

  /**
     SETPARAMS - Set the filter time constant. Gain = 1.0
     INPUTS:
       T         Filter time constant
  **/
  void setparams(double T);

  /**
     SETPARAMS - Set the filter gain and time constant

     INPUTS:
       K         Filter time constant
       T         Filter time constant
  **/
  void setparams(double K, double T);

    /**
     SETPARAMS - Set the filter time constant and limits on state and output. Gain = 1.0

     INPUTS:
       T          Filter time constant
       xmin       Min. limit for state variable
       xmax       Max. limit for state variable
       ymin       Min. limit for output y
       ymax       Max. limit for output y
  **/
  void setparams(double T,double xmin,double xmax,double ymin,double ymax);

  /**
     SETPARAMS - Set the filter gain, time constant and limits on state and output

     INPUTS:
       K          Filter gain
       T          Filter time constant
       xmin       Min. limit for state variable
       xmax       Max. limit for state variable
       ymin       Min. limit for output y
       ymax       Max. limit for output y
  **/
  void setparams(double K,double T,double xmin,double xmax,double ymin,double ymax);

  /**
     SETPARAMS - Set the filter gain, time constant and limits for state, derivative,
     and output
     
     INPUTS:
       K          Filter gain
       T          Filter time constant
       xmin       Min. limit for state variable
       xmax       Max. limit for state variable
       dxmin      Min. limit for rate of change of x
       dxmax      Max. limit for rate of change of x
       ymin       Min. limit for output y
       ymax       Max. limit for output y
  **/
  void setparams(double K,double T,double xmin,double xmax,double dxmin, double dxmax,double ymin,double ymax);
  
  /**
     SETPARAMS - Set the filter time constant and limits for state, derivative,
     and output
     
     INPUTS:
       T          Filter time constant
       xmin       Min. limit for state variable
       xmax       Max. limit for state variable
       dxmin      Min. limit for rate of change of x
       dxmax      Max. limit for rate of change of x
       ymin       Min. limit for output y
       ymax       Max. limit for output y
  **/
  void setparams(double T,double xmin,double xmax,double dxmin, double dxmax,double ymin,double ymax);

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
