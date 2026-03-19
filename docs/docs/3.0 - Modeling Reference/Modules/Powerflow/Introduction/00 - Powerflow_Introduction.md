# Power Flow User Guide

The **powerflow** module performs distribution level solver methods to primarily obtain the voltage and current values in a system. Details on the different solver methods and how each object are handled are in the powerflow guide. 

# Synopsis
    
    
    module powerflow;

    module powerflow {
      acceleration_factor 1.4;
      default_maximum_voltage_error 1e-6 V;
      fault_impedance 1e-6+0d Ohm;
      geographic_degree 0.0;
      line_capacitance FALSE;
      lu_solver "";
      maximum_voltage_error 1e-6 V;
      nominal_frequency 60.0 Hz;
      NR_iteration_limit 500;
      NR_superLU_procs 1;
      primary_voltage_ratio 60.0 pu;
      require_voltage_control FALSE;
      show_matrix_values FALSE;
      solver_method FBS;
      warning_underfrequency 55.0 Hz;
      warning_overfrequency 65.0 Hz;
      warning_undervoltage 0.8 pu;
      warning_overvoltage 1.2 pu;
      warning_voltageangle 2.0 deg;
    }
    

# Classes

  * **billdump** – Billing data dump on meter objects at specified times.
  * **currdump** – Current data dump on link object at specified times.
  * **powerflow_library** – Abstract class for objects the only contain data but don't synchronize. 
    * **emissions** – Emissions library object
    * **line_configuration** – Line configuration library object
    * **line_spacing** – Link spacing library object
    * **overhead_line_conductor** – Overhead conductor library object
    * **power_metrics** – Reliability metrics container
    * **regulator_configuration** – Regulator configuration library object
    * **restoration** – Restoration control library object
    * **transformer_configuration** – Transformer configuration library object
    * **triplex_line_configuration** – Triplex line configuration library object
    * **underground_line_conductor** – Underground line conductor configuration library object
  * **powerflow_object** – Abstract class for object the are included in the flow solution 
    * **fault_check** – Fault identification object for reliability analysis
    * **frequency_gen** – Frequency generation object
    * **link** – Abstract link (branch) object. 
      * **fuse** – Fusable link object.
      * **line** – Generic line object 
        * **overhead_line** – Overhead line object
        * **triplex_line** – Triplex line object
        * **underground_line** – Underground line object
      * **regulator** – Voltage regulator object
      * **relay** – Relay object
      * **series_reactor** – Series reactor object
      * **switch_object** – Generic switch object 
        * **recloser** – Recloser object
        * **sectionalizer** – Sectionalizer object
      * **transformer** – Transformer object
    * **node** – Generic node (bus) object. 
      * **capacitor** – Capacity object
      * **load** – Generic load object 
        * **pqload** – PQ load object
      * **meter** – Meter object
      * **motor** – Motor object
      * **substation** – Substation object
      * **triplex_node** – Triplex node object 
        * **triplex_meter** – Triplex meter object
    * **volt_var_control** – Volt-var controller object
  * **voltdump** – Volt data dump on node objects at specified times
  
# Globals

Parameter | Type | Description
-- | -- | --
**acceleration_factor** | double | specifies the GS method acceleration factor (default is 1.4).
**default_maximum_voltage_error** | double | specifies the default voltage convergence limit (default is 10-6 puV).
**fault_impedance** | complex | specifies the fault impedance (default is 10-6<0).
**geographic_degree** | double | specifies the topological degree factor (default is 0.0).
**line_capacitance** | bool | specifies whether to use line capacitance quantities (default is FALSE).
**lu_solver** | char256 | specifies the filename for external LU solver (default is "").
**maximum_voltage_error** | double | specifies the default voltage convergence limit for synchronization events (default is 10-6 pu).
**nominal_frequency** | double | is the nominal AC frequency (default is 60.0 Hz).
**NR_iteration_limit** | int64 | specifies the maximum number of iteration during a single NR solution (default is 500).
**NR_superLU_procs** | int32 | specifies the number of processors to use for multithreaded NR solutions (default is 1).
**primary_voltage_ratio** | double | is the primary voltage ratio for link and node voltage calcs (default is 60.0 pu).
**require_voltage_control** | bool | enable voltage control source requirement (default is FALSE).
**show_matrix_values** | bool | enables dumping of matrix calculations as they occur (default is FALSE).
**solver_method** | enumeration {FBS,GS,NR} | specifies the solver method to use (default is FBS).
**warning_underfrequency** | double | specifies the frequency below which a warning is posted (default is 55.0 Hz)
**warning_overfrequency** | double | specifies the frequency above which a warning is posted (default is 65.0 Hz).
**warning_undervoltage** | double | specifies the voltage below which a warning is posted (default is 0.8 pu).
**warning_overvoltage** | double | specifies the voltage above which a warning is posted (default is 1.2 pu).
**warning_voltageangle** | double | specifies the angle difference (over a single link) above which a warning is posted (default is 2.0 deg).

## Inherited Classes

Nearly all objects within the **powerflow** module are derived from two primary objects: **node** and **link**. Therefore, any properties defined for these two objects are also available to any derived object. For example, a **node** has voltage properties, so a **load** automatically has these properties available as well. Any **powerflow** objects that inherit properties from **node** or **link** will be labeled as such. Furthermore, **node** and **link** contain most relevant default quantities. Derived objects often assume zero value or throw an error if an explicit property is not indicated. Any exceptions to this rule will be indicated in the parameter list of the particular object. 

## Powerflow Objects

Along with all of the properties inherited from either **node** or **link**, all objects within the **powerflow** module inherit two basic properties. These two properties are the phases of the object and the nominal voltage for that area of the system. These are expressed in the `phases` and `nominal_voltage` parameters of **powerflow** objects. 

The `phases` property has a variety of valid inputs. These are: 

  * `A` \- Phase A of a three phase connection
  * `B` \- Phase B of a three phase connection
  * `C` \- Phase C of a three phase connection
  * `D` \- Delta connected phases - this implies ABC, but explicitly specifying them is recommended
  * `N` \- Neutral phase
  * `G` \- Ground phase
  * `S` \- Split phase - this represents residential level wires (2 "hot" and 1 neutral wire)

These different phases can be specified in a variety of ways. Below are some identical examples with a simple **node** object (which is covered in more later in this page). 
    
    
    object node {
    	phases ABC;
    	}
    
    
    
    object node {
    	phases "ABC";
    	}
    
    
    
    object node {
    	phases A|B|C;
    	}
    
    
    
    object node {
    	phases "A|B|C";
    	}
    

The other common property is nominal voltage, which is passed into the objects using the `nominal_voltage` parameter. This parameter is used to ensure connected objects are in the proper region (have the same nominal voltage) and also to specify an initial value for the convergence criteria of the different solver methods. Using the same node example, a 7200 Volt nominal voltage would be expressed as: 
    
    
    object node {
    	nominal_voltage 7200.0;
    	}
    

