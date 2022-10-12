#include "cblock.h"

Cblock::Cblock()
{
  p_order = 1; // For future use
  p_xmin  = p_ymin = -1000.0;
  p_xmax  = p_ymax =  1000.0;
}

Cblock::~Cblock(void)
{
}

/* Set the coefficients for the transfer function

  Y(s)    b0s + b1
 ----- = --------------
  U(s)    a0s + a1

  a = [a0,a1], b = [b0,b1]
 */
void Cblock::setcoeffs(double *a,double *b)
{
  p_A[0] = -a[1]/a[0];
  p_B[0] = (b[1] - a[1]*b[0])/a[0];
  p_C[0] = 1.0;
  p_D[0] = b[0]/a[0];
}

void Cblock::setxlimits(double xmin,double xmax)
{
  p_xmin = xmin;
  p_xmax = xmax;
}

void Cblock::setylimits(double ymin,double ymax)
{
  p_ymin = ymin;
  p_ymax = ymax;

}

double Cblock::getderivative(double x, double u)
{
  return p_A[0]*x + p_B[0]*u;
}

double Cblock::updatestate(double u,double dt,double xmin,double xmax,DeltaModeStage stage)
{
  double xout,dx_dt;

  if(stage == PREDICTOR) {
    p_dxdt[0] = getderivative(x[0],u);
    xout = x[0] + dt*p_dxdt[0];
    xout = std::max(xmin,std::min(xout,xmax));
    p_xhat[0] = xout;
  }

  if(stage == CORRECTOR) {
    dx_dt = getderivative(p_xhat[0],u);
    xout = x[0] + 0.5*dt*(p_dxdt[0] + dx_dt);
    xout = std::max(xmin,std::min(xout,xmax));
    x[0] = xout;
  }

  return xout;
}

double Cblock::updatestate(double u,double dt,DeltaModeStage stage)
{
  double xout;

  xout = updatestate(u,dt,p_xmin,p_xmax,stage);

  return xout;
}

double Cblock::getoutput(double u,double dt,double xmin,double xmax,double ymin,double ymax,DeltaModeStage stage)
{
  double x,y;

  x = updatestate(u,dt,xmin,xmax,stage);
  
  y = p_C[0]*x + p_D[0]*u;

  y = std::max(ymin,std::min(y,ymax));

  return y;
}

double Cblock::getoutput(double u,double dt,DeltaModeStage stage)
{
  double y;

  y = getoutput(u,dt,p_xmin,p_xmax,p_ymin,p_ymax,stage);

  return y;
}

void Cblock::init(double u, double y)
{
  double uout;
  if(fabs(p_A[0]) < 1e-10) x[0] = y;
  else {
    uout = y/(p_D[0] - p_C[0]*p_B[0]/p_A[0]);
    x[0] = -p_B[0]/p_A[0]*uout;
  }
}

double Cblock::getstate(DeltaModeStage stage)
{
  double xout;
  if(stage == PREDICTOR) xout = p_xhat[0];
  if(stage == CORRECTOR) xout = x[0];
  return xout;
}

// ------------------------------------
// PI Controller
// ------------------------------------

PIControl::PIControl(void): Cblock()
{
}

void PIControl::setparams(double Kp, double Ki)
{
  double a[2],b[2];

  // Parameters for state-space representation
  // Transfer funtion is
  // Kp + Ki/s => (sKp + Ki)/s
  // In standard form, this will be
  // (sKp + Ki)/(s + 0)

  b[0] = Kp;
  b[1] = Ki;
  a[0] = 1.0;
  a[1] = 0.0;

  setcoeffs(a,b);
  setxlimits(-1000.0,1000.0);
  setylimits(-1000.0,1000.0);

}
void PIControl::setparams(double Kp, double Ki,double xmin,double xmax,double ymin,double ymax)
{
  setparams(Kp,Ki);
  setxlimits(xmin,xmax);
  setylimits(ymin,ymax);
}

// ------------------------------------
// Filter
// ------------------------------------

Filter::Filter(void)
{
  setxlimits(-1000.0,1000.0);
  setylimits(-1000.0,1000.0);
}

void Filter::setparams(double T)
{
  double a[2],b[2];

  // Parameters for state-space representation
  // Transfer funtion is
  // 1/(1 + sT)
  // In standard form, this will be
  // (s*0 + 1)/(sT + 1)

  b[0] = 0;
  b[1] = 1;
  a[0] = T;
  a[1] = 1;

  setcoeffs(a,b);
  setxlimits(-1000.0,1000.0);
  setylimits(-1000.0,1000.0);
}

void Filter::setparams(double T,double xmin,double xmax,double ymin,double ymax)
{
  setparams(T);
  setxlimits(xmin,xmax);
  setylimits(ymin,ymax);
}

// ------------------------------------
// Integrator
// ------------------------------------

Integrator::Integrator(void)
{
}

void Integrator::setparams(double T)
{
  double a[2],b[2];

  // Parameters for state-space representation
  // Transfer funtion is
  // 1/(sT)
  // In standard form, this will be
  // (s*0 + 1)/(sT + 0)

  b[0] = 0;
  b[1] = 1;
  a[0] = T;
  a[1] = 0;

  setcoeffs(a,b);
  setxlimits(-1000.0,1000.0);
  setylimits(-1000.0,1000.0);
}


