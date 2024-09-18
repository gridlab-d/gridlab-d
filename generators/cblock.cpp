#include "cblock.h"
#include <stdexcept>

Cblock::Cblock()
{
  p_order = 1; // For future use
  p_xmin  = p_ymin = p_dxmin = -1000.0;
  p_xmax  = p_ymax = p_dxmax =  1000.0;
  p_current_stage = PREDICTOR;
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
  if(a[0] == 0) {
    printf("a[0] = 0.0\n");
    throw std::runtime_error("Cblock is not a transfer function\nCheck the value of the parameters\n");
  }
  p_A[0] = -a[1]/a[0];
  p_B[0] = b[1]/a[0] - a[1]*b[0]/(a[0]*a[0]);
  p_C[0] = 1.0;
  p_D[0] = b[0]/a[0];
}

void Cblock::setxlimits(double xmin,double xmax)
{
  p_xmin = xmin;
  p_xmax = xmax;
}

void Cblock::setdxlimits(double dxmin,double dxmax)
{
  p_dxmin = dxmin;
  p_dxmax = dxmax;
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

void Cblock::updatestate(double u,double dt,double xmin,double xmax,double dxmin, double dxmax,IntegrationStage stage)
{
  double xout,dx_dt;

  if(stage == PREDICTOR) {
    dx_dt = getderivative(x[0],u);
    p_dxdt[0] = std::max(p_dxmin,std::min(dx_dt,p_dxmax));
    xout = x[0] + dt*p_dxdt[0];
    xout = std::max(xmin,std::min(xout,xmax));
    p_xhat[0] = xout;
    p_current_stage = CORRECTOR; // Update stage
  }

  if(stage == CORRECTOR) {
    dx_dt = getderivative(p_xhat[0],u);
    dx_dt = std::max(p_dxmin,std::min(0.5*(p_dxdt[0] + dx_dt),p_dxmax));
    xout = x[0] + dt*dx_dt;
    xout = std::max(xmin,std::min(xout,xmax));
    x[0] = xout;
    p_current_stage = PREDICTOR; // Update stage
  }
}

void Cblock::updatestate(double u,double dt,IntegrationStage stage)
{
  updatestate(u,dt,p_xmin,p_xmax,p_dxmin,p_dxmax,stage);
}

double Cblock::getoutput(double u)
{
  double x_n,y_n;
  if(p_current_stage == PREDICTOR) x_n = x[0];
  else x_n = p_xhat[0];
  
  y_n = p_C[0]*x_n + p_D[0]*u;

  y_n = std::max(p_ymin,std::min(y_n,p_ymax));

  return y_n;
}
  
double Cblock::getoutput(double u,double dt,double xmin,double xmax,double ymin,double ymax,IntegrationStage stage, bool dostateupdate)
{
  double x_n,y_n,x_n1;

  if(dostateupdate) {
    updatestate(u,dt,xmin,xmax,p_dxmin,p_dxmax,stage);
    if(stage == PREDICTOR) x_n = p_xhat[0];
    else x_n = x[0];
  } else {
    if(stage == PREDICTOR) x_n = p_xhat[0] = x[0];
    else x_n = x[0] = p_xhat[0];
  }
  
  y_n = p_C[0]*x_n + p_D[0]*u;

  y_n = std::max(ymin,std::min(y_n,ymax));

  return y_n;
}

double Cblock::getoutput(double u,double dt,double xmin,double xmax,double ymin,double ymax,IntegrationStage stage)
{
  double y;
  
  y = getoutput(u,dt,xmin,xmax,ymin,ymax,stage,true);

  return y;
}


double Cblock::getoutput(double u,double dt,double xmin,double xmax,double dxmin, double dxmax,double ymin,double ymax,IntegrationStage stage, bool dostateupdate)
{
  double y;
  
  p_dxmin = dxmin;
  p_dxmax = dxmax;

  y = getoutput(u,dt,xmin,xmax,ymin,ymax,stage,dostateupdate);

  return y;
}

double Cblock::getoutput(double u,double dt,double xmin,double xmax,double dxmin, double dxmax,double ymin,double ymax,IntegrationStage stage)
{
  double y;
  
  y = getoutput(u,dt,xmin,xmax,dxmin,dxmax,ymin,ymax,stage,true);

  return y;
}



double Cblock::getoutput(double u,double dt,IntegrationStage stage,bool dostateupdate)
{
  double y;

  y = getoutput(u,dt,p_xmin,p_xmax,p_ymin,p_ymax,stage,dostateupdate);

  return y;
}

double Cblock::getoutput(double u,double dt,IntegrationStage stage)
{
  double y;

  y = getoutput(u,dt,p_xmin,p_xmax,p_ymin,p_ymax,stage,true);

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

double Cblock::init_given_u(double u)
{
  double y;
  if(fabs(p_B[0]) < 1e-10) {
    x[0] = 0;
    y = p_D[0]*u;
  } else if(fabs(p_A[0]) < 1e-10) {
    x[0] = 0;
    y = p_D[0]*u;
  } else {
    x[0] = -p_B[0]/p_A[0]*u;
    y    = p_C[0]*x[0] + p_D[0]*u;
  }
  return y;
}

double Cblock::init_given_y(double y)
{
  double u;
  if(fabs(p_A[0]) < 1e-10) {
    u = 0;
    x[0] = y;
  } else {
    u = y/(p_D[0] - p_C[0]*p_B[0]/p_A[0]);
    x[0] = -p_B[0]/p_A[0]*u;
  }
  return u;
}


double Cblock::getstate(IntegrationStage stage)
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
  setxlimits(-1000.0,1000.0);
  setdxlimits(-1000.0,1000.0);
  setylimits(-1000.0,1000.0);
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
  setdxlimits(-1000.0,1000.0);
  setylimits(-1000.0,1000.0);

}

void PIControl::setparams(double Kp, double Ki,double xmin,double xmax,double ymin,double ymax)
{
  setparams(Kp,Ki);
  setxlimits(xmin,xmax);
  setylimits(ymin,ymax);
}

void PIControl::setparams(double Kp, double Ki,double xmin,double xmax,double dxmin, double dxmax,double ymin,double ymax)
{
  setparams(Kp,Ki);
  setxlimits(xmin,xmax);
  setdxlimits(dxmin,dxmax);
  setylimits(ymin,ymax);
}


// ------------------------------------
// Filter
// ------------------------------------

Filter::Filter(void)
{
  setxlimits(-1000.0,1000.0);
  setdxlimits(-1000.0,1000.0);
  setylimits(-1000.0,1000.0);
}

void Filter::setparams(double K,double T)
{
  double a[2],b[2];

  // Parameters for state-space representation
  // Transfer funtion is
  // K/(1 + sT)
  // In standard form, this will be
  // (s*0 + K)/(sT + 1)

  b[0] = 0;
  b[1] = K;
  a[0] = T;
  a[1] = 1;

  setcoeffs(a,b);
  setxlimits(-1000.0,1000.0);
  setdxlimits(-1000.0,1000.0);
  setylimits(-1000.0,1000.0);
}

void Filter::setparams(double T)
{
  setparams(1.0,T);
}

void Filter::setparams(double K,double T,double xmin,double xmax,double ymin,double ymax)
{
  setparams(K,T);
  setxlimits(xmin,xmax);
  setylimits(ymin,ymax);
}

void Filter::setparams(double T,double xmin,double xmax,double ymin,double ymax)
{
  setparams(1.0,T,xmin,xmax,ymin,ymax);
}

void Filter::setparams(double K,double T,double xmin,double xmax,double dxmin, double dxmax,double ymin,double ymax)
{
  setparams(K,T);
  setxlimits(xmin,xmax);
  setdxlimits(dxmin,dxmax);
  setylimits(ymin,ymax);
}

void Filter::setparams(double T,double xmin,double xmax,double dxmin, double dxmax,double ymin,double ymax)
{
  setparams(1.0,T,xmin,xmax,dxmin,dxmax,ymin,ymax);
}

// ------------------------------------
// Lead lag
// ------------------------------------

LeadLag::LeadLag(void)
{
  setxlimits(-1000.0,1000.0);
  setdxlimits(-1000.0,1000.0);
  setylimits(-1000.0,1000.0);  
}

void LeadLag::setparams(double TA,double TB)
{
  double a[2],b[2];

  // Parameters for state-space representation
  // Transfer funtion is
  // (1 + sTA)/(1 + sTB)

  b[0] = TA;
  b[1] = 1;
  a[0] = TB;
  a[1] = 1;

  setcoeffs(a,b);
  setxlimits(-1000.0,1000.0);
  setdxlimits(-1000.0,1000.0);
  setylimits(-1000.0,1000.0);
}

void LeadLag::setparams(double TA,double TB,double xmin,double xmax,double ymin,double ymax)
{
  setparams(TA,TB);
  setxlimits(xmin,xmax);
  setylimits(ymin,ymax);
}

void LeadLag::setparams(double TA,double TB,double xmin,double xmax,double dxmin, double dxmax,double ymin,double ymax)
{
  setparams(TA,TB);
  setxlimits(xmin,xmax);
  setdxlimits(dxmin,dxmax);
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
  setdxlimits(-1000.0,1000.0);
}

void Integrator::setparams(double T,double xmin,double xmax,double ymin,double ymax)
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
  setxlimits(xmin,xmax);
  setylimits(ymin,ymax);
  setdxlimits(-1000.0,1000.0);
}
