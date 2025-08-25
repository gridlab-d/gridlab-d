# Spec:Optimize

**Source URL:** https://GridLAB-D™.shoutwiki.com/wiki/Spec:Optimize
## Contents

  * 1 S1
  * 2 S1.1
  * 3 S1.2
  * 4 S1.3
  * 5 S2
    * 5.1 S2.1
    * 5.2 S2.2
    * 5.3 S2.3
    * 5.4 S2.4
    * 5.5 S2.5
    * 5.6 S2.6
    * 5.7 S2.7
  * 6 Prototype
  * 7 See also
### S1

Module scope
    ([R1])

### S1.1

Name
    ([R1.1]) The module implementation folder shall be named **optimize** and the function shall be named according to the [GridLAB-D™ module implementation standards].

### S1.2

Core scope
    ([R1.2]) The module core linkage shall conform to the [GridLAB-D™ module implementation standards].

### S1.3

Class scope
    ([R1.3]) The class **objective** shall conform to the [GridLAB-D™ class implementation standards].

### S2

([R2]) The **objective** class properties shall conform to the [GridLAB-D™ property naming standards]

#### S2.1

Target property
    ([R2.1]) The general form of the objective shall be either **[max|min](_expression_)** or **[_object_ :]_property_ =_expression_** , where _expression_ is a mathematical expression conforming to the [GLM syntax].

#### S2.2

Changing properties
    ([R2.2]) The general form of the decision variables list is **[_object1_ :]_property1_[,[_object2_ :]_property2_[,...,[_objectN_ :]_propertyN_]]**.

#### S2.3

Constraints
    ([R2.3]) Move this to specs: Constraints are described as comma-separated a list of inequalities of the form **[_object_ :]_property_ [<|<=|=|>=|>|<>] (_expression_)**.

#### S2.4

Maximum trials
    ([R2.4]) The default number of trials shall be 100.

#### S2.5

Precision
    ([R2.5]) The default precision shall be 3 significant digits.

#### S2.6

Models
    ([R2.6]) The enumeration allows the specification of modeling assumptions such as linear, non-negative, and automatic scaling.

#### S2.7

Methods
    ([R2.7]) The Specific method options such as the use of tangent or quadratic estimates, forward or central derivatives, and Newton or conjugate searching.

## Prototype

A prototype module **optimize** was created in the trunk at R2104 (22 Dec 2010) with a simple single variable optimizer using Newton's method. The optimizer object's class is **simple**. The following variables are provided 

goal (enumeration)
    The goal for the target variable, i.e., EXTREMUM, MINIMUM, or MAXIMUM. By default the goal is EXTREMUM, meaning that the solver will find the extreme that the second derivative leads to from the initial value. If MINIMUM or MAXIMUM is given, the solver will check that the second derivative leads to the desired goal and stop if it does not.

objective (char1024)
    The target variable that is to be minimized (or maximized). This is specified in the form _objectname_._propertyname_ and must refer to a double.

variable (char1024)
    The decision variable that is to be modified to minimize (or maximize) the target variable. This is specified in the form _objectname_._propertyname_ and must refer to a double.

constraints (char1024)
    A constraint (optional) given as variable comparisons. The allowed comparisons are <, <=, ==, >=, >, and !=, e.g., `_objectname_._propertyname_ < 12.2`.

delta (double)
    The value by which the decision variable is altered to estimate the first and second derivatives.

epsilon (double)
    The value to within which the first derivative must be reduced before stopping the optimization process.

trials (int32)
    The maximum number of trials allowed before stopping the optimization process.

Two test models are provided to verify the behavior of the **simple** class. 

simple.glm
    This creates a trivial quadratic function which the solver should be able to solve in a single iteration.

cubic.glm
    This creates two simultaneous cubic problems which the solver should be able to solve concurrently in no more than 5 iterations.

constrained_simple.glm
    This creates a trivial quadratic function with a constraint on the goal.

## See also

  * [User's Manual]
  * [Requirements]
  * [Developer notes]
  * [Technical support document]
  * [Validation]
  * [Modules]

