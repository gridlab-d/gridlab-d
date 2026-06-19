# Power Flow User Guide

The **powerflow** module performs distribution level solver methods to primarily obtain the voltage and current values in a system. A power flow calculation determines the steady-state node voltages and line currents given the system model, electrical loads at each node, and substation voltage. The power flow problem is solved using a three-phase unbalanced flow solver. When the topology is strictly radial, the forward-back sweep (FBS) method is used by default. Non-radial topologies use the Gauss-Seidel (GS) or Newton-Raphson (NR) methods. See [Algorithm Selection](Theory/Algorithm_Selection.md) for guidance on choosing a solver method.

Details of the different objects and their properties are in the [Powerflow Classes](Classes/00-index.md) page. Technical background on modeling theory is in the [Theory](#theory) section below.

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

# Classes

Nearly all objects within the **powerflow** module are derived from two primary objects: **node** and **link**. Therefore, any properties defined for these two objects are also available to any derived object. For example, a **node** has voltage properties, so a **load** automatically has these properties available as well. Any **powerflow** objects that inherit properties from **node** or **link** will be labeled as such. Furthermore, **node** and **link** contain most relevant default quantities. Derived objects often assume zero value or throw an error if an explicit property is not indicated. Any exceptions to this rule will be indicated in the parameter list of the particular object. 

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
  

# Theory

The following pages provide technical details on the algorithms and component models used in the powerflow module. The specific methodology and equations of the forward-back sweep method are described in Kersting (2007). The Gauss-Seidel implementation is described in Grainger and Stevenson (1994). The Newton-Raphson method is described in Garcia, et al. (2000).

- [Algorithm Selection](Theory/Algorithm_Selection.md)
- [Component Modeling: Node, Load, and Meter](Theory/Node_Load_Meter_Theory.md)
- [Lines and Transformers Theory](Theory/Lines_and_Transformers_Theory.md)
- [Controls and Devices Theory](Theory/Controls_and_Devices_Theory.md)
- [Newton-Raphson Distribution Powerflow Solver](Theory/Newton-Raphson_Distribution_Powerflow_Solver.md)
- [Motor Theory: Single Phase Induction Motor](Theory/Tech_DeltaSPIM.md)
- [Motor Theory: Three Phase Induction Motor](Theory/Tech_DeltaTPIM.md)
- [Motor Theory: Composition Motor Models](Theory/Tech_CompositionMotor.md)
- [Series Compensator Theory](Theory/Series_compensator.md)
- [References](Theory/References.md)

# Visualization Tools

## Visualization Tools

**Google Earth support**: Currently, GridLAB-d supportx KML output for Google Earth. However, only the **powerflow** module supports the export of KML when the `--kmldump` command line argument is given. There is an effort under way at PNNL to extend and generalize this capability so that all objects in the model are output to KML and users can navigate the objects using Google Earth.

In the Version 1 **kmldump** routine, only objects that have geocoordinates defined are output to KML. In Version 2, an additional option is desired whereby an object that does not have geocoordinates is output as a part its parent object. Accessing the parent object give the user access to an object tree of all those that have been attached at that location.
    

