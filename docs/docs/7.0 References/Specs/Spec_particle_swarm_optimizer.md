# Spec:particle swarm optimizer

**Source URL:** https://GridLAB-D™.shoutwiki.com/wiki/Spec:particle_swarm_optimizer
**TODO**:  Complete missing sections 

## Contents

  * 1 Overview
    * 1.1 General Description
  * 2 Modeling Assumptions
  * 3 PSO
    * 3.1 Interfacing overview
  * 4 Equations
    * 4.1 PSO published inputs
    * 4.2 PSO published outputs
  * 5 PSO timing
    * 5.1 PSO passes
    * 5.2 Pseudo code for basic PSO
  * 6 Testing and Validation
  * 7 References
  * 8 See also
# Overview

The purpose of developing Particle Swam Optimization (PSO) in GridLAB-D™ is to determine the tuning parameters of Proportional Integral Derivative (PID) controller based price control mechanisms with energy resources on a power system. This model has a lot of flexibility and can easily be applied for single and multi-object optimization problems. 

## General Description

PSO is a population based stochastic optimization technique [1], [2]. It is inspired by social behavior of bird flocking, fish schooling and swarming theory. The implementation of PSO paradigms is not computationally intensive in terms of memory requirements and speed. PSO can be used to solve many of the same kinds of problems as genetic algorithms. In PSO, the potential solutions are called particles and they move through the search-space by following current optimum particles. Each particle keeps track of its coordinates in the search-space. The particle with the best solution (fitness) is stored and this value is called _pbest_. PSO also keeps track of the overall best value obtained so far by any particle in the population. This is called _gbest_. In each step, PSO calculates particle velocities and updates their positions. This process continues until the number of iterations reaches the maximum value or a minimum error criterion is satisfied. The final output indicates the fitness of the given candidate solution. 

# Modeling Assumptions

# PSO

## Interfacing overview

# Equations

Table 1: Equations  Equation | Number   
---|---  
$$\begin{align} Vel_{p,d}^{(j+1)} &= Vel_{p,d}^{(j)}+C_1rand()\left(pBest_{p,d}^{(j)}-present_{p,d}^{(j)}\right)+C_2rand()\left(gBest_{p,d}^{(j)}-present_{p,d}^{(j)}\right)\end{align}$ | 1.1   
$$\begin{align} present_{p,d}^{(j+1)} &= present_{p,d}^{(j)}+Vel_{p,d}^{(j)}\end{align}$ | 1.2   
  
  


  


Table 2: Equation Notation  Variable | Definition   
---|---  
$$p$ | Population size   
$$d$ | Dimension   
$$j$ | Iteration count   
$$Vel$ | Particle velocity   
$$pBest$ | Personal best   
$$gBest$ | Global best   
$$present$ | Current value   
$$rand()$ | Standard uniform distribution (uniformly distributed random numbers) on the open interval (0,1)   
$$C_1, C_2$ | Positive constants   
  
  


## PSO published inputs

Table 3 : PSO inputs  Variable | Type | Units | Value (default) | Allowable values | Definition   
---|---|---|---|---|---  
number_agents | double | NA | 10 | Value > 1 | The number of agents (particles) in a swarm.   
number_unknowns | double | NA | 3 | Value >= 1 | This is the dimension of solution space. It is equal to the number of unknowns in a variable to be optimized.   
maximum_iterations | double | NA | 250 | Value > 1 | number of iteration to execute the optimization.   
maximum_weight | double | NA | 0.9 | 0< Value < 1 | The upper limit of the time varying inertia weight.   
minimum_weight | double | NA | 0.4 | 0< Value < 1 | The lower limit of the time varying inertia weight.   
maximum_velocity | double | NA | 20 | 0< Value < 100 | It is the maximum change that particle can take during one iteration.   
learning_factor_C1 | double | NA | 2 | 0< Value < 2 | Hookes's coefficient.   
learning_factor_C2 | double | NA | 2 | 0< Value < 2 | Hookes's coefficient.   
minimum_error | double | NA | 0.001 | 0< Value < 0.1 | This is used as one of the stop conditions.   
update_period | double | sec | 900 | Value > 1 | How often should PSO run to obtain fitness of the given candidate solution.   
cost_function | TBD   
  
## PSO published outputs

Table 4: PSO outputs  Variable | Type | Units | Definition   
---|---|---|---  
gbest | double | NA | Best value obtained so far   
  
# PSO timing

PSO finds the fitness of the given candidate solution for every user defined time period. The values of the decision variables are set in presync (). The objective function value is calculated in sync () and it is compared with the best solution so far in postsync(). If the current solution is better than the best solution so far, then keep the current solution as the best else do not update the best. Update particle velocity and position with the current best for each particle and update the fitness of the function for each iteration. This process repeats until it reaches the maximum iteration limit. 

## PSO passes

For obtaining the fitness of the given candidate solution, PSO proceeds as: 
    
    
     1.	Solve the fitness of the objective function for each particle
     2.	Find the _best_ fitness for each particle (_pBest_)
     3.	Find the _best_ fitness among all the particles (_gBest_)
     4.	Solve for new velocity and position for each particle using 1.1 and 1.2
     5.	Update the fitness of each particle with the new particle position. This procedure continues until it reaches the maximum iteration limit.
    

## Pseudo code for basic PSO
    
    
    * **Step1: Initialize the particles and their velocity components (init ())**
     For each particle:
        For all dimensions:
            particle position (particle, dimension) = rand
            particle velocity (particle, dimension) = rand
            pbest (particle, dimension) = particle position (particle, dimension)
        End
     End
    
    
    
    * **Step 2: Initialize the pbest fitness array (init ())**
     For each particle
         pbest fitness (particle) = -1000
     End
    
    
    
    * **Step 3: Main PSO routine (presync ())**
      For each iteration(as long as number of iterations are less than maximum iterations)
          * **Step 3A: Find the fitness of each particle**
                     For each particle
                         value = particle position (particle)
                         solution = f (value)(declaration of an objective function) 
                        (the calculation of the objective function takes place in sync (inrinsic sync()) using runtime classes)
                         If Solution is NOT zero
                            current fitness = 1/absolute (solution)
                         Else
                            current fitness = 1000
                         End
                     End
           * **Step 3B: Decide pbest among all the particles**
                       For each particle
                           If current fitness (particle) is greater than pbest fitness (particle)
                  		   pbest fitness (particle) = current fitness (particle)
                              For all dimensions
                                  pbest (particle, dimension) = particle position (particle, dimension)
                              End
                           End
                       End        
           * **Step 3C: Decide gbest among all the particles**
                       [gbest, gbest index] = maximum of current fitness
                           For all dimensions
                               gbest (dimension) = particle position (gbest index, dimension)
                           End
           * **Step 3D: Update the position and velocity components**
                          For each particle
                              For all dimensions
                                  present (dimension) = particle position (particle, dimension)
                             End
                             For all dimensions
                                 calculate particle velocity using 1.1
                                 calculate particle position using 1.2
                             End
                          End
     End 
      * **Step 4: Publish outputs**
                gbest
    

# Testing and Validation

……Testing and validation is still being finalized and will be updated shortly. Please try back later ……. 

# References
    
    
    1.	Eberhart, R. C. and Kennedy, J. A new optimizer using particle swarm theory. “ _Proceedings of the Sixth International Symposium on Micromachine and Human_
            Science _”, Nagoya, Japan. pp. 39-43, 1995_
    2.	Kennedy, J. and Eberhart, R. C. Particle swarm optimization. “ _Proceedings of IEEE International Conference on Neural Networks_ ”, Piscataway, NJ. pp.1942-
            1948,1995
    

# See also

  * [particle_swarm_optimizer]
  * [Requirements]
  * [Implementation]
  * [Grizzly (Version 2.3)]

