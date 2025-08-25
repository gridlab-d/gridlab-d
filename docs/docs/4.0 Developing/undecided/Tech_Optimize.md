# Tech:Optimize

**Source URL:** https://GridLAB-D™.shoutwiki.com/wiki/Tech:Optimize
# Tech:Optimize

GridLAB-D™ Optimization Module Documentation 

Amelia Musselman 

6/26/2012 

Note: This form of "simple.cpp" has not been added to the baseline code of GridLAB-D™. Contact Jason Fuller (jason.fuller@pnnl.gov) for more information. 

## Contents

  * 1 Document Structure
  * 2 General Variables
  * 3 DISCRETE and DISCRETE_ITERATE
    * 3.1 Variables
    * 3.2 Presync
      * 3.2.1 Case 0:
      * 3.2.2 Case 1 to Case 26:
      * 3.2.3 Case 27:
    * 3.3 Postsync
      * 3.3.1 Case 0:
      * 3.3.2 Case 1 to Case 26:
      * 3.3.3 Case 27:
  * 4 CONTINUOUS
    * 4.1 Variables
    * 4.2 Presync
      * 4.2.1 Case 0 and Case 1:
      * 4.2.2 Cases 2 - 3 and Cases 4 - 5:
      * 4.2.3 Case 6:
    * 4.3 Postsync
      * 4.3.1 Case 0:
      * 4.3.2 Case 1:
      * 4.3.3 Cases 2 - 3 and Cases 4 - 5:
      * 4.3.4 Case 6:
  * 5 CONTINUOUS_LS
    * 5.1 Variables
    * 5.2 Presync
      * 5.2.1 Case 0:
      * 5.2.2 Case 1 - 6:
      * 5.2.3 Case 7:
      * 5.2.4 Case 8 & 9:
    * 5.3 Postsync
      * 5.3.1 Case 0:
      * 5.3.2 Case 1 - 6:
      * 5.3.3 Case 7:
      * 5.3.4 Case 8:
      * 5.3.5 Case 9:
  * 6 CONTINUOUS_NMCD
    * 6.1 Variables
    * 6.2 Presync
      * 6.2.1 Case 0:
      * 6.2.2 Case 1 - 6:
      * 6.2.3 Case 7:
      * 6.2.4 Case 8:
      * 6.2.5 Case 9:
    * 6.3 Postsync
      * 6.3.1 Case 0:
      * 6.3.2 Case 1:
      * 6.3.3 Case 2:
      * 6.3.4 Cases 3 - 4 and Cases 5 - 6:
      * 6.3.5 Case 7:
      * 6.3.6 Case 8:
      * 6.3.7 Case 9:
  * 7 Constraint Checking
  * 8 Notes
## Document Structure

This document describes each of the 5 optimization routines that have been developed for the GridLAB-D™ Optimization Module. The optimizer types are DISCRETE, DISCRETE_ITERATE, CONTINUOUS, CONTINUOUS_LS, and CONTINUOUS_NMCD. The DISCRETE optimizer is a specific case of the DISCRETE_ITERATE optimizer, and so is no longer necessary, but it was initially left in for testing purposes. In fact, the only difference between the DISCRETE and DISCRETE_ITERATE optimizers is that the DISCRETE_ITERATE optimizer stays at the same point until the objective function converges or until some trial limit, so we describe the two together. CONTINUOUS and CONTINUOUS_LS both use a gradient descent method. The difference between the two is that CONTINUOUS uses a simple decreasing step size with user input initial step size and percent to reduce by and CONTINUOUS_LS uses a line search based on Armijo’s rule to determine the step size. CONTINUOUS_NMCD is a coordinate descent that uses Newton’s method on the partial derivative of the objective function to optimize one variable at a time. Thus we have three main optimizer types: a discrete optimizer, a continuous optimizer using gradient descent, and a continuous optimizer using coordinate descent with Newton’s method. 

For each of these optimization routines we will describe variables required and the function of each of the cases for the presync and postsync. In general, the presync is used to set the variables and the postsync is used to check the objective function value given the variables that were set in the presync. The optimizer must go through sync every time a variable is changed to update the objective function value. There is a separate presync and postsync for each optimizer type, and a general presync and postsync that calls the appropriate optimizer specific presync or postsync given the user input optimizer type. This method was easier to organize than checking the optimizer type in each case because the optimizers don’t all have the same number of cases. However, some of the optimizers, in particular the DISCRETE and DISCRETE_ITERATE, and the CONTINUOUS and CONTINUOUS_LS are very similar. 

_simple_ registers simple (the optimizer class) with the core and publishes the user input variables (but not variables that are set within the optimizer). I haven’t changed how the class is registered with the core other than adding variables to gl_publish_variable. 

init parses all of the variables, objective function, and constraints. It reads in parameters that the user sets and has default values if the user decides not to set the parameters. It also sets constants necessary for specific optimizer types that the user is not expected (an in most cases will not be able) to set. 

Note that currently constraints can only be of the form variable (<, <=, >, >=, ==, or !=) constant because of the way the program is initialized and registered with the core. Some modifications would need to be made to the way the constraints are parsed and checked to allow constraints that are non-linear and/or include more than one variable, but there is nothing about the method for checking constraints or the optimization algorithms that should disallow constraints of this type. 

Currently we allow up to three variables and eight constraints, an upper and lower bound for each of the variables and the objective function. Usually in optimization there are not constraints on the objective function because constraints on the objective function can be written in terms of the variables. However, in our case, since we don’t have an analytical expression for the objective function it could be beneficial to allow it to be constrained directly. Once the initialization is modified to allow constraints that involve more than one variable we could have any number of constraints, so the constraints may need to be saved as a vector with one function to check all constraints because we currently check each constraint individually. 

In the following we describe each case of the pre-sync and post-sync and define all variables involved. 

## General Variables

User input: 

  * goal: what is optimal? MAXIMUM or MINIMUM. Must be specified by user, no default.
  * optimizer: optimizer type. DISCRETE, DISCRETE_ITERATE, CONTINUOUS, CONTINUOUS_LS, or CONTINUOUS_NMCD. Must be specified by user currently (although the default could be set to DISCRETE or DISCRETE_ITERATE because those will work with any type of objective function).
  * objective: objective function name. I think this is only used in warnings or other print statements when we need something need a name for the objective.
  * variable1, variable2, variable3: first, second, and third variable names. Same as objective name above for variables.
  * constraint1, constraint2, … constraint8: constraint names. Same as objective name above for constraints. We currently allow for up to 8 constraints: an upper and lower bound for each of the variables and the objective.
  * constrain1, constrain2, … constrain8: comparison operator and value to compare variable to in constraint. Value is the constant on the right hand side of the constraint of the form above. Use constrain.op to get operator (<, <=, >, >=, ==, or !=) and constrain.value to get the constant to be compared to the current variable value in checking constraints.
Internally set: 

  * *pObjective: pointer to objective function value
  * *pVariable1, *pVariable2, *pVariable3: pointers to decision variable values
  * *pConstraint1, *pConstraint2, … *pConstraint8: pointer to constraint variable value. Since currently constraints are all of the form variable (<, <=, >, >=, ==, or !=) constant, *pConstraint will point to the same place as *pVariable for the variable on the left hand side of the equation. To check constraints, the current value of the variable is compared to the constant value using the comparison operator, which are both stored as part of constrain described above.
## DISCRETE and DISCRETE_ITERATE

### Variables

User input: 

  * delta1, delta2, delta3: amount to change x1, x2, and x3 by, respectively, when checking around the current point for a better solution. Default 0 (returns a warning and won’t optimize if all deltas are 0 because then it can’t move anywhere).
  * op_interval: number of timesteps to wait before trying to optimize again once the optimizer has been run once. Default 1.
  * trials: not used in DISCRETE. Max number of trials that optimizer will stay at the same point to wait for the objective function to converge. The DISCRETE_ITERATE optimizer should obtain the same results at the DISCRETE optimizer by settings trials to 1. Default 1.
  * epsilon: not used in DISCRETE. convergence tolerance for iterator. If objective function value at the current timestep is within epsilon of the previous objective function value stop iterating and move to the next point. Default 0.1
Internally set: 

  * pass: which case to go to.
  * t_next: don’t optimize again until you’ve reached this timestep.
  * feas_check: boolean indicating whether or not a feasible solution has been found for this optimization yet. If a feasible solution can’t be found from the starting point the optimizer will stop and return an error.
  * violation: boolean indicating whether or not constraints are broken at the current point.
  * next_x1, next_x2, next_x3: used for setting variable values. Starting point at beginning of optimization, set to the next point to go to at the end of each step of the optimization.
  * best_x1, best_x2, best_x3: x values corresponding to best objective function seen so far. Discrete value always goes to the best surrounding value, or stays at the current point if no surrounding value is better.
  * best_y: best objective function value seen so far. Used in comparing current solution to best seen.
  * trial: not used in DISCRETE. Current number of trials that the optimizer has been on the same point. Moves on at the end of the iteration if trial >= trials. Starts at 1.
  * last_y: not used in DISCRETE. y value from previous iteration. Used to check convergence.
### Presync

#### Case 0:

  * Set starting point. For each possible variable check if that variable exists and if it does set it to next_x.
  * Check if any constraints on the variables are violated. If they are, return an error and stop because the starting point must be feasible (this also assumes that the point the optimizer ended on at the last iteration, which was feasible then must still be feasible).
  * If DISCRETE_ITERATE only set variables on the first trial because once we come back to the trial a second time the variables will be the same, we are just waiting for the objective function value to converge.
#### Case 1 to Case 26:

  * Set the variable values for each point surrounding the starting point. Variables values are set in the following order:

Case  | x1  | x2  | x3   
---|---|---|---  
0  | next_x1  | next_x2  | next_x3   
1  | next_x1 - delta  | next_x2 - delta  | next_x3 - delta   
2  | next_x1 - delta  | next_x2 - delta  | next_x3   
3  | next_x1 - delta  | next_x2 - delta  | next_x3 + delta   
4  | next_x1 - delta  | next_x2  | next_x3 - delta   
5  | next_x1 - delta  | next_x2  | next_x3   
6  | next_x1 - delta  | next_x2  | next_x3 + delta   
7  | next_x1 - delta  | next_x2 + delta  | next_x3 - delta   
8  | next_x1 - delta  | next_x2 + delta  | next_x3   
9  | next_x1 - delta  | next_x2 + delta  | next_x3 + delta   
  
... the remainder of the chart has x2 and x3 repeated for x1 = next_x1 and x1 = next_x1+delta, except the case where no variables are shifted by delta is skipped since that case is first. 

  * As in case 0, we only set the variables on the first trial of each case because they will remain the same while we iterate for the objective function to converge.
  * Check constraints on variables. If any of the variable constraints are violated, reset variables to starting position to avoid getting an objective function that can’t be calculated and because we don’t need to know the objective function value at infeasible points anyway.
#### Case 27:

  * Return an error if no feasible solution can be found.
  * Otherwise, on the first trial set the variables and next_x to the value checked that gave the best objective function value.
### Postsync

#### Case 0:

  * Check if you have either reached the convergence criteria for the objective function or the trial limit for waiting for convergence. If you have, do the following. Otherwise set last_y to compare the objective to on the next iteration and return to the same pass.
  * Check constraints on objective function (it won’t get to this step if constraints on the variables were violated).
  * If constraints are broken: 
    * move to next pass without doing anything. Note: we don’t return an error if the objective function constraints are violated at the starting point. If the variables are feasible we only return an error if we can’t get to any feasible point in one step.
  * Otherwise: 
    * Set feas_check = 1 to indicate that a feasible solution has been found. (feas_check set to 0 in init).
    * Set best value to starting point because we don’t have anything else to compare it to yet.
    * Move to next pass
#### Case 1 to Case 26:

  * Iterate, same as in Case 0.
  * If the variable constraints were violated move to the next pass without doing anything. Note that this is done in both the if and else statements, so we will immediately move to the next pass before iterating if the variable constraints are violated.
  * At last step of iteration: check constraints on objective function.
  * If the current solution is feasible, and either you haven’t found a feasible solution yet or the current solution is better than the best feasible solution thus far: 
    * Set feas_check = 1 to indicate that a feasible solution has been found.
    * Set best value to current point.
    * Move to next pass
  * Otherwise move to next pass without doing anything.
#### Case 27:

  * Iterate until obj. has converged or trial limit reached.
  * Return an error if no feasible solution was found.
  * Otherwise move forward in time, and don’t optimize again until t1+op_interval.
## CONTINUOUS

### Variables

User input: 

  * op_interval: number of timesteps to wait before trying to optimize again once the optimizer has been run once. Default 1.
  * trials: max number of trials that optimizer will stay at the same point to wait for the objective function to converge. Defalt 1.
  * epsilon: convergence tolerance for iterator. If objective function value at the current timestep is within epsilon of the previous objective function value stop iterating and move to the next point. Default 0.1
  * stepsize: initial stepsize for first step of gradient descent. Default 1, but should be specified by user. Gives a warning if not.
  * reduce: fraction to reduce stepsize by each step of the gradient descent. Default is 0.1 (10%).
  * grad_epsilon: epsilon for gradient descent stopping criteria. Gradient descent will stop when the norm of the gradient is less than grad_epsilon (||grad f||<= grad_epsilon). Default 0.1.
  * grad_trials: maximum number of trials for gradient descent if stopping criteria is not met. Default 100.
Internally set: 

  * pass: which case to go to.
  * t_next: don’t optimize again until you’ve reached this timestep.
  * violation: boolean indicating whether or not constraints are broken at the current point.
  * next_x1, next_x2, next_x3: used for setting variable values. Starting point at beginning of optimization, set to the next point to go to at the end of each step of the optimization.
  * trial: Current number of trials that the optimizer has been on the same point. Moves on at the end of the iteration if trial >= trials. Starts at 1.
  * init_stepsize: used to save the initial stepsize that the user set because once the gradient descent is completed either by meeting the stopping criteria or reaching grad_trials (trial limit) we will reset the stepsize to the initial user input value for the next time the optimizer is called (after op_interval).
  * last_y: y value from previous iteration. Used to check convergence.
  * lower_y: objective funciton value at next_x-delta. Used for estimating gradient (the partial derivative will be (upper_y-lower_y)/(2*delta), but we don’t need to save upper_y because it will equal *pObjective at the time the gradient is computed).
  * grad_trial: trial counter for gradient descent method. Used to determine when to stop if convergence criteria is not met.
  * delta: change in x used to estimate gradient. Same for all variables. Default 0.01
  * dx1, dx2, dx3: estimated partial derivatives for x1, x2, and x3, respectively.
  * grad_mag: magnitude of the gradient. Used for checking stopping criteria.
### Presync

#### Case 0 and Case 1:

  * On first trial shift x1 to next_x1 - delta (Case 0) or next_x1 - delta (Case 1). Delta is a small amount we shift by to approximate gradient.
#### Cases 2 - 3 and Cases 4 - 5:

Same as Case 0 and Case 1 for variables 2 and 3, respectively. 

#### Case 6:

On first trial do the following. Otherwise just go to postsync. 

  * Check if stopping criteria (||grad f||<= grad_epsilon) has been met or gradient descent trial limit reached (note this is different from the iteration trial limit). If it has, stop at current point before moving in direction of gradient.
  * Otherwise, move stepsize in the normalized direction of the gradient for maximum or negative gradient for minimum.
### Postsync

#### Case 0:

  * Check if you have either reached the convergence criteria for the objective function or the trial limit for waiting for convergence. If you have, do the following. Otherwise set last_y to compare the objective to on the next iteration and return to the same pass.
  * Set lower_y to the objective value for next_x1 - delta to use in calculating the gradient.
  * Move to next pass.
#### Case 1:

  * Iterate, same as in Case 0.
  * On last iteration calculate an approximation for dx1. Our estimate is the average of the upper and lower slopes: ((upper_y-mid_y)/delta - (mid_y-lower_y)/delta)/2 = (upper_y-lower_y)/(2*delta), but upper_y is the current objective function value.
#### Cases 2 - 3 and Cases 4 - 5:

Same as Cases 0 and 1 for variables 2 and 3, respectively. 

#### Case 6:

  * Iterate if you’ve reached the stopping criteria for the gradient descent. Otherwise don’t bother to iterate because the objective function value isn’t actually used for anything in Case 6.
  * If gradient descent stopping criteria has been met: 
    * Reset all of the parameters. Reset stepsize to initial user input step size.
    * Don’t optimize again until t1+op_interval.
  * Otherwise, reduce stepsize by user input parameter reduce (or default 0.1).
## CONTINUOUS_LS

Reference for Armijo’s Rule: 

Bazar, Mokhtar, et al. Nonlinear Programming: Theory and Algorithms, Third Edition. Wiley: Hoboken, 2006. p. 362-363 

### Variables

The variables for calculating the gradient are all the same as for CONTINUOUS above. The only variables that are different are those involving the step size. For CONTINUOUS_LS we no longer use the variable reduce and stepsize is defined in a different way. We also have the following additional variables instead to determine the stepsize through an inexact line search using Armijo’s rule. 

User input: 

  * stepsize: initial stepsize to try for the line search using Armijo’s Rule. Note that although this still can be input by the user it is not as necessary as for the first continuous method. Default 1.
Internally set: 

  * alpha: amount to multiply stepsize by if too small to satisfy Armijo’s Rule. Default 2.
  * epsilon_ls: scalar multiple for ?'(?) in Armijo's rule. Default 0.2.
  * init_y: Objective function value at starting point (before x is shifted by delta to approximate derivative). Used for first order approximation of function at current point for Armijo’s Rule.
  * ls_trial: current trial for finding line search step size.
  * ls_trials: trial limit for line search (max number of times to double or half step size before stopping). Currently it was set to 1 or 2 for testing to avoid hitting convergence iteration limit. I do not know what a good number of trials would be.
### Presync

#### Case 0:

Set variables to initial value. 

#### Case 1 - 6:

Calculate gradient, same as for CONTINUOUS (Cases shifted one up since we start on 1 instead of 0). 

#### Case 7:

Same as for CONTINUOUS presync Case 6. 

#### Case 8 & 9:

On first trial move stepsize in the normalized direction of the gradient for maximum or negative gradient for minimum. Otherwise just go to postsync. 

### Postsync

#### Case 0:

  * Iterate until objective converges, same as in CONTINUOUS.
  * Set init_y to the objective value for next_x1 to use in checking Armijo’s Rule.
  * Move to next pass.
#### Case 1 - 6:

Calculate gradient, same as for CONTINUOUS (Cases shifted one up since we start on 1 instead of 0). 

#### Case 7:

  * Iterate, same as Case 0.
  * On last iteration check if you’re reached the stopping criteria for the gradient descent.
  * If gradient descent stopping criteria has been met: 
    * Reset all of the parameters. Reset stepsize to initial user input step size.
    * Don’t optimize again until t1+op_interval.
  * Otherwise: Check if Armijo’s Rule is satisfied. 
    * If it is, increase stepsize (multiply by alpha, default 2) and go to case 8 to continue to increase it until the greatest stepsize for which Armijo’s rule is satisfied.
    * If it’s not, decrease stepsize (divide by alpha), and go to case 9 to continue to decrease stepsize until Armijo’s Rule is satisfied.
#### Case 8:

  * Iterate, same as Case 0.
  * On last iteration check if Armijo’s Rule is still satisfied. 
    * If it is not (which is the first if statement within each of the if statements for the goal type) then the previous stepsize will be the greatest stepsize for which Armijo’s Rule is satisfied. In this case we do the following: 
      * Divide by alpha to set the stepsize to the last step for which it was satisfied
      * Set next_x to next_x shifted stepsize in the normalized direction of the gradient for maximum or negative gradient for minimum.
      * Go back to case 0. Variables will be set to next_x and we will take another step of the gradient descent from Case 0.
    * If it is satisfied: 
      * If we have reached the trial limit for the line search do the second two steps above (for if it were not satisfied) with the current stepsize.
      * If we have not reached the trial limit, continue to multiply by alpha to increase stepsize until Armijo’s rule is no longer satisfied or line search trial limit is met. Return to Case 8.
#### Case 9:

  * Iterate, same as Case 0.
  * On last iteration check if Armijo’s Rule is satisfied. 
    * If it is (which is the first if statement within the each of the if statements for the goal type) then we have reached the greatest stepsize for which Armijo’s Rule is satisfied. In this case we do the following: 
      * Set next_x to next_x shifted stepsize in the normalized direction of the gradient for maximum or negative gradient for minimum.
      * Go back to case 0. Variables will be set to next_x and we will take another step of the gradient descent from Case 0.
    * If it is not satisfied: 
      * If we have reached the trial limit for the line search do the steps above (for if it were satisfied) with the current stepsize.
      * If we have not reached the trial limit, continue to divide by alpha to decrease stepsize until Armijo’s rule is satisfied or line search trial limit is met. Return to Case 9.
## CONTINUOUS_NMCD

### Variables

User input: 

  * op_interval: number of timesteps to wait before trying to optimize again once the optimizer has been run once. Default 1.
  * trials: max number of trials that optimizer will stay at the same point to wait for the objective function to converge. Defalt 1.
  * epsilon: convergence tolerance for iterator. If objective function value at the current timestep is within epsilon of the previous objective function value stop iterating and move to the next point. Default 0.1
  * grad_epsilon: epsilon for gradient stopping criteria. Optimizer will run Newton’s method on one variable at every timestep. However, if the norm of the gradient is less than grad_epsilon (||grad f||<= grad_epsilon) the optimizer will stay at the current point and not continue trying to optimize. Default 0.1.
  * NM_trials: max number of trials for Newton's Method used in single timestep. Default 10 (for testing purposes. It may be good to make it bigger).
  * NM_epsilon: epsilon for Newton’s Method stopping criteria, which is similar to gradient descent stopping criteria, but for a single variable (|f'(x)|<= NM_epsilon).
Internally set: 

  * pass: which case to go to.
  * t_next: don’t optimize again until you’ve reached this timestep.
  * violation: boolean indicating whether or not constraints are broken at the current point.
  * next_x1, next_x2, next_x3: used for setting variable values. Starting point at beginning of optimization, set to the next point to go to at the end of each step of the optimization.
  * trial: Current number of trials that the optimizer has been on the same point. Moves on at the end of the iteration if trial >= trials. Starts at 1.
  * last_y: y value from previous iteration. Used to check convergence.
  * lower_y: objective funciton value at next_x-delta. Used for approximating gradient (the partial derivative will be (upper_y-lower_y)/(2*delta), but we don’t need to save upper_y because it will equal *pObjective at the time the gradient is computed). Also used in approximating second derivative for Newton step.
  * delta: change in x used to estimate gradient. Same for all variables. Default 0.01
  * dx1, dx2, dx3: estimated partial derivatives for x1, x2, and x3, respectively.
  * grad_mag: magnitude of the gradient. Used for checking stopping criteria.
  * stepsize: used for backtracking if optimal solution is infeasible. Currently, stepsize starts at (next_x1 - init_x1)/2 and is halved every time the optimizer takes a step back to get into a feasible region.
  * best_x1, best_x2, best_x3: x values corresponding to best objective function seen so far. Saves the best point seen during the optimization, and will always return that point.
  * best_y: best objective function value seen so far. Used in comparing current solution to best seen.
  * NM_trial: current trial of Newton's Method. Stop when stopping criteria is met or when NM_trial>= NM_trials.
  * init_x1, init_x2, init_x3: x value at the start of the optimization. Used to back track if the optimum is infeasible. Note: currently we backtrack between the optimal unconstrained solution and the starting point, but it would probably be better to backtrack between the optimum and best feasible point found.
  * mid_y: objective funciton value at next_x. Used for calculating second derivative. Used in the same way as lower_y, but for Newton’s method we need three points to calculated the second derivative.
  * ddx1, ddx2, ddx3: approximated second partial derivatives.
  * x1_wait, x2_wait, x3_wait: number of timesteps to wait before trying to optimize specified x again. This variable is to keep the optimizer from getting stuck on trying to optimize the same variable repeatedly because the optimal solution is infeasible. Since we use derivative information to decide which variable to optimize if a variable is stuck at a constraint it may still have the greatest magnitude derivative, but we can’t move to a better solution, so we should try to optimize another variable instead. For this reason, x_wait will be set to 2 when a variable has to backtrack or can’t reach an optimum for some other reason. If the wait for one variable is greater than 0 it will decrease one each time another variable is optimized until it reached zero. Only variables with x_wait == 0 may be optimized. This way if a variable is unable to reach an optimum we will try to optimize the other two variables before optimizing the initial variable again.
  * op_var: which variable to run Newton's Method on, determined by which variable has the greatest magnitude for the partial derivative. Will be 0 before a variable is selected (when the gradient is being calculated).
### Presync

#### Case 0:

On first trial, set variables to initial value. Check variable constraints. If infeasible, return an error because starting point must be feasible. (This also assumes that the point the optimizer ended on at the last iteration, which was feasible then must still be feasible). 

#### Case 1 - 6:

Move plus or minus delta around current point, same as in CONTINUOUS_LS (and CONTINUOUS shifted one case up). 

#### Case 7:

On the first trial: 

  * If you haven’t already selected a variable to optimize (i.e. op_var == 0): 
    * If the gradient stopping criteria (||grad f||<= grad_epsilon ) is met, go back to initial point (it was shifted to calculate the gradient) and skip to post-sync. Don’t try to optimize any more.
    * Otherwise: 
      * Select the variable with the greatest partial derivative at the current point that has x_wait>0 to optimize. Set op_var equal to the corresponding variable number. (x_wait indicates that the optimization was stopped by a constraint or the variable couldn’t be optimized for some other reason the last time we tried to optimize x. If this happens we set x_wait so we will try to optimize the other variables first before we try to optimize that x again).
      * Decrease x_wait values that are greater than 0 by one, so we can try to optimize those variables again after all the other variables have been optimized.
  * Once we have selected a variable to optimize either by going through the process above (we will go through one of the variable if statements directly after setting op_var in the if(op_var == 0) statement) or because we had previously selected op_var, for the variable that we want to optimize: 
    * Check if we’ve reached the stopping criteria for Newton’s Method (|f’(x)|<= NM_epsilon ) or the Newton’s Method trial limit. If we have, set the variables to next_x and check the constraints (we won’t do anything if there is a violation until the postsync though).
    * Otherwise, check other ways in which we won’t be able to reach an optimum, and set variables back to next_x because we will stop trying to optimize in the post-sync.
    * Otherwise, take a step according to Newton’s Method for the derivative of the variable that we’re optimizing on (since Newton’s Method is an algorithm for finding the roots of a function we use it to find the roots of the derivative because the local max or min will occur where f’(x) = 0), so our Newton step will be x1=x0-f’(x)/f’’(x).
#### Case 8:

On first trial set variables to next_x and check if constraints are violated (we won’t actually check the result of violation until the post-sync though). 

#### Case 9:

Set variables to next_x. We don’t iterate on case 9 because it’s not necessary to know the objective function value immediately, and we will move forward in time after Case 9, so it will iterate naturally. 

### Postsync

#### Case 0:

  * Iterate until objective converges, same as in CONTINUOUS.
  * Check objective constraints. We won’t get to this step unless the variable constraints were satisfied. If infeasible return an error because starting point must be feasible.
  * Otherwise, if feasible, set init_x’s to be used in backtracking if the optimum is infeasible. Set initial best_x’s and best_y’s to compare the final solution to, so we will always return the best value seen at the end of the optimization. Also set mid_y to be used in calculating the second derivatives.
  * Move to next pass.
#### Case 1:

Iterate and calculate dx1. Same as Case 0 for CONTINUOUS. Go to Case 2. 

#### Case 2:

  * Iterate same as for CONTINUOUS.
  * Calculate dx1 same as for CONTINUOUS.
  * Calculate ddx1 (the second partial derivative for x1).


    
    
    ddx = (dx_upper - dx_lower)/delta 
        = ((upper_y-mid_y)/delta-(mid_y-lower_y)/delta)/delta 
        = (upper_y-2*mid_y+lower_y)/delta^2
        = (*pObjective-2*mid_y+lower_y)/(delta*delta),
        since upper_y = *pObjective since the current value of x is next_x+delta.
    

  * If op_var == 0 optimizer is currently in the process of calculating the gradient and op_var has not yet been selected go to the next pass to finish calculating the gradient.
  * Otherwise, optimizer has already selected a variable to optimize on, go to case 7 for that variable to take a Newton step.
#### Cases 3 - 4 and Cases 5 - 6:

Same as Cases 1 - 2 for variables 2 and 3, respectively. 

#### Case 7:

  * If op_var is still equal to 0 you must have met the gradient stopping criteria. If so, reset all of the iteration parameters, go back to case 0, and don’t try to optimize again until t1+op_interval. No need to iterate if you have already reached the stopping criteria.
  * Otherwise, for the appropriate op_var: 
    * If you have reached the stopping criteria or trial limit for Newton’s Method: 
      * Check before iterating if there was a constraint on the variables. If there was, set stepsize = (next_x - init_x)/2, the initial amount to backtrack by, and go to Case 8 to start backtracking.
      * If there were no violations on the variable constraints iterate as done previously until objective function converges. Then check objective constraints. If there is a violation do the same as for a violation on the variable constraints.
      * If there are no violations on either the variable or objective constraints, if you’re current objective value is worse than the best feasible objective value seen previously set the variables to the values that gave the best objective value. Otherwise, stay at your current point because it is the best seen thus far. Go to case 9 to finalize (we have completed Newton’s method for the specific variable).
    * If you have not yet met the stopping criteria see if it is possible to reach an optimum from the current point: 
      * Check if ddx == 0, which means the objective function is linear with respect to the current variable, and Newton’s method cannot be used. If so, set x_wait = 2 (we will wait until the other two variables have been optimized to try to optimize this one again). Throw a warning, reset all or iteration parameters, and go back to case 0. Don’t try to optimize again until op_interval.
      * Check if ddx<0 with a goal of MINIMUM. If so, throw a warning, reset all or iteration parameters, and go back to case 0 because you won’t be able to get to a local min when the second derivative is negative. Don’t try to optimize again until op_interval.
      * Check if ddx>0 with a goal of MAXIMUM. If so, same as above, can’t get to a local max when the second derivative is positive.
    * Otherwise, you just took a Newton step in the presync. 
      * Check if there was a violation of the variable constraints. If there wasn’t a violation of the variable constraints, iterate until the objective function converges, then check constraints on objective function. If violated, go back to case 1, same as above. Otherwise compare current objective value to best feasible value seen so far. If current value is better, set best_x to current x value.
      * Whether there is a violation or not, go back to case 1 for another iteration of Newton’s method on the same variable.
#### Case 8:

Optimizer will only come to this case if the optimum or place that Newton’s Method ended because of a trial limit is infeasible. Currently this case will backtrack until it gets into a feasible region and then stop, compare that solution to the best solution seen, and return whichever is better. Better options for backtracking are discussed in notes below. 

Initial stepsize was set to (next_x - init_x)/2 in case 7 and we have already backtracked once by that stepsize in going through the presync (since we set next_x to next_x2 - stepsize). We will do the following to backtrack for the variable currently being optimized: 

  * Check the variable constraints. If violated, halve stepsize, backtrack stepsize further, and stay on case 8 (but go back to presync).
  * If variable constraints aren’t violated iterate until objective converges then check objective constraints. If there is a violation, do the same as for variable violation.
  * If neither the variable nor objective constraints are violated compare current objective value to best feasible value seen so far. If current value is better, set best_x to current x value. Go to case 9 to finalize (regardless of whether best_x was reset or not).
#### Case 9:

Reset all or iteration parameters. Set pass to 0 for next time optimization is called. Don’t try to optimize again until op_interval. 

## Constraint Checking

The constraints are checked in two helper functions at the bottom of the file. There is one function to check variable constraints and a separate to check constraints on the objective because sometimes it is beneficial to check variable constraints in the presync or before we’ve iterated to get an accurate estimate of the objective function value. Currently, the constraints must be of the form variable (<, <=, >, >=, ==, or !=) constant. Some modifications will need to be made to check constraints not of this form. 

## Notes

I noticed when I was debugging that the variable dx, which was left over from when I was watching it before but is no longer in any of the optimization routines and is not defined in the initialization either, is still defined and changes whenever I step through the debugger. I am not sure if this is a variable that is defined elsewhere in GridLAB-D™ and is somehow changing with the optimizer or if something else. It is not affecting anything as far as I can tell, but I don’t know where it’s coming from. 

Constraint backtracking: When backtracking in the NMCD optimizer t would be better if once we got into a feasible region we could go back and forth until we got within a certain distance of the constraint because we might overstep the constraint by a lot on the first backtracking step we take (or a latter step). Also, currently backtracking will go from the x values at the optimal solution back to the initial x values. However, it might be better to backtrack between the optimal solution and best feasible solution instead because that would be a smaller region to backtrack over, and I’m pretty sure that the x-value at the optimal feasible solution must be between the optimal solution and the best solution seen so far (at least I can’t think of a counter example). Note also that there is not currently a trial limit on the backtracking. It might be good to add a trial limit on the backtracking, especially if we allow the backtracking to go back and forth to get within a certain tolerance of the constraint because otherwise we could get stuck backtracking indefinitely. If a trial limit was added, once the trial limit was reached we would just return the best feasible value so far. 

NMCD optimizer and infeasibility: grad epsilon will tell the optimizer not to try to continue to optimize anymore once we have reached a local max/min, but if we’re not moving because all of the variables are stuck on a constraint we would also want it to stop. Currently we have no way of doing that (the variables will continue to go to the infeasible optimum and then return to the constraint). x_wait forces the optimizer to rotate through all the variables, but that won’t help if all are stuck at a bound. 

NMCD optimizer testing: I have only tested the CONTINUOUS_NMCD optimizer on an unconstrained runtime function so far. It needs a lot more testing. In particular, the constraints need to be tested. 

Unconstrained Gradient Descent: I did not initially set up the gradient descent to save the best value seen so far because theoretically the gradient descent should be able to reach an optimum without it. However, if the gradient descent stops because it reached the grad_trial limit it could potentially be at a worse value than it had been at previously because the stepsize could take it into a worse region. In this case it would be better to go to the best value seen so far. 

Constrained Gradient Descent: The method of coordinate descent should work for constraining the gradient descent. We could run gradient descent until a constraint was violated, then return to the last feasible point and run coordinate descent using the derivative (essentially a single variable gradient descent) rather than Newton’s method. 

Other things to add: 

  * Currently we have the option to set how many seconds to wait before trying to optimize again. However, the only way that can be set to never is if the number of seconds to wait is set to the difference between the start time and the end time. It would be beneficial to add an option to optimize once and then never again. Also, for optimizers like CONTINUOUS_NMCD in particular it would be good if we could optimize once (or a few times) for each variable and then never optimize again. This could be accomplished by adding a counter for the total number of times to optimize and having the user input the max number of times to optimize. (Note that unlike the trial limits we used this one would not reset at each timestep).
  * Currently, at the beginning of the postsync we say, if ( isnan(*pObjective) || !isfinite(*pObjective) ) return an error, but there are times when it is okay if the objective function is undefined as long as that’s not the point we end on. For this reason, it would be better if instead of giving an error in the case above we treated such points as infeasible. To do this we would have to put the check inside of the post-sync for each individual optimizer instead of inside the general postsync because it would need to go where we check the objective constraints.
  * There are also cases where it would be good to change the errors to warnings and exit the optimizer so the rest of GridLAB-D™ can continue to run.
Linearity Check: Currently, all of the optimizers will still be able to run if the objective function is linear, but it would be good to put an explicit check for linearity within each optimizer because, the continuous optimizers in particular expect the objective function to be non-linear. If the objective function is linear and unconstrained we should stop at the current point because otherwise the solution will just continue to improve infinitely. If the objective function is linear and constrained, we know that the optimal solution will be on a constraint (unless the constraints are integer. Also, I’m not sure if this holds if the constraints are continuous, but non-linear). We could just follow the improving direction until we hit a constraint, but there might be a way to get there more quickly. Here is what would happen with the current versions of the optimizers if the objective function were linear: 

  * DISCRETE: will continue to improve on every step, but if we knew the objective were continuous we might be able to take bigger steps in the improving direction or stop if it were unconstrained (The discrete optimizer will work on a continuous as a well as a discrete objective function).
  * CONTINUOUS: Will improve on every step. Will eventually stop by hitting grad trial limit.
  * CONTINUOUS_LS: Same as CONTINUOUS. Also, Armijo’s rule will never be met. The line search will be stopped by the trial limit.
  * CONTINUOUS_NMCD: If the variable currently being optimized is linear when the others are held constant the CONTINUOUS_NMCD optimizer will stop at the current point and move to the next variable to try to optimize. If all variables are linear it will be stuck and never move anywhere because the second derivative will be 0 for a linear objective and we tell it to stop if the second derivative is 0, so we won’t divide by 0.
