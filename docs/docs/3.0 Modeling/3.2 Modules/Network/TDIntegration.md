# Transmission

The transmission and distribution integration framework provides a means for GridLAB-D's distribution-level powerflow results to interact with a transmission-level model. This allows modeling the impact of aggregating control effects and demand response on the transmission grid, as well as the transmission-level impact of distribution-side resources. GridLAB-D interfaces with the commercially available [PowerWorld Corporation's Simulator](http://www.powerworld.com/products/simulator.asp) program, which handles the model and the transmission solver, while providing both transmission-level powerflow and dynamics simulation. 

# Classes

All T&D  classes will reside inside a network module. This module will provide classes that connect to PowerWorld, that pass values from powerflow  objects to a PowerWorld model, and record the behavior of the module and its instantiated objects. 

Currently, all the GridLAB-D classes are loaded with 
    
    module network;
    

## pw_model

The pw_model class represents an individual PowerWorld model that GridLAB-D is communicating with via a [SimAuto](http://www.powerworld.com/products/simulator/add-ons-2/simauto) link. It will identify, load, and recalculate the model based on GLM inputs and GridLAB-D simulation updates. 

### Prerequisites

The pw_model must be initialized before any pw_load or pw_recorder objects. As a rule, the directive 
    
      #set init_sequence=DEFERRED
    

will correctly alter the initialization behavior. 

PowerWorld must be installed, along with a licensed SimAuto extension. 

### Usage

An example pw_model definition: 
    
    
      module network;
      
      object pw_model{
        name MyPowerworldModel;
        model_name "C:\\Program Files(x86)\\PowerWorld\\mySamples\\SampleOne.PWB";
      }
    

The only required field is the model_name, which must point to a file. This file is loaded by SimAuto, so is not validated until the COM calls are made. Note that any backslashes in the path (\\) must be duplicated. This ensures GridLAB-D passes the proper path to the PowerWorld Simulator. 

The properties "update_flag", "exchange_count", and "valid_flag" are recordable reference properties that may be of interest during troubleshooting. 

### Published Inputs

Input Name  | Quantity type  | Description   
---|---|---  
model_name  | string  | The file path for the PowerWorld model to run. If the path is blank, or the file cannot be opened by PowerWorld, the run will stop.   
update_flag  | bool  | A flag set by the pw_load objects that are attached to the specific pw_model, and reset by the model when it calls for a PowerWorld update.   
valid_flag  | bool  | A flag set by the pw_model if the underlying PowerWorld model is in a valid state. Should this flag be set to 'false', the GridLAB-D powerflow solution will use the last good voltage value and will abort should GridLAB-D converge while PowerWorld remains divergent.   
  
### Published Outputs

Output Name  | Quantity type  | Description   
---|---|---  
exchange_count  | int32  | The number of times PowerWorld and GridLAB-D have exchanged voltage and load information.   
  
## pw_load

pw_load objects correspond with loads in PowerWorld. When PowerWorld updates, the positive sequence voltage is read in and applied to the GridLAB-D object, which is later converted to balanced three-phase by a substation object. When the GridLAB-D pw_load's present load changes by more than a certain threshold, the pw_load will set a flag in its corresponding pw_model indicating that the PowerWorld loads need to be updated, that PowerWorld needs to recalculate at the end of the iteration, and that GridLAB-D will need to reiterate with new values from PowerWorld. 

### Usage

The pw_load object must define a parent model, a bus number, and a load id. There must be a bus with the given number in the specified model, and a load with the given number attached to that bus. The existence of the PowerWorld objects can only be checked once the SimAuto link is established. The 'parent' object must initialize before the pw_load; the load will defer initialization as required, which requires the global init_sequence to be properly set. 

A typical pw_load looks like: 
    
    
      #set init_sequence=DEFERRED
      module network;
      
      object pw_model {
        name MyModel;
        file my_model.PWB;
      }
      
      object pw_load {
        name MyLoad;
        parent MyModel;
        powerworld_bus_num 4;
        powerworld_load_id 2;
      }
    

A Substation object must be used to connect a pw_load to the GridLAB-D Powerflow solver. 

### Published Inputs

Input Name  | Quantity type  | Description   
---|---|---  
powerworld_bus_num  | int  | The bus number within PowerWorld associated with the Bus object to read voltages from in PowerWorld. A bus must be associated with that number, or this object cannot run.   
powerworld_load_id  | string  | The ID within PowerWorld of the load to post current loads to. A load must be associated with the ID, and be attached to the bus numbered in powerworld_bus_num, or this object cannot run.   
power_threshold  | double MVA  | The magnitude of power change that the pw_load object will tolerate before signaling an intent to post its new current load to the PowerWorld model. An aggregation of the magnitude difference for each ZIP faction will be compared against this value to determine if a reiteration is necessary.   
load_power  | complex MegaVolt-Amperes  | The power load as dictated by the substation underneath the pw_load.   
load_impedance  | complex MegaVolt-Amperes  | The impedance load as dictated by the substation underneath the pw_load.   
load_current  | complex MegaVolt-Amperes  | The constant current load as dictated by the substation underneath the pw_load.   
  
The published voltage outputs are read by the substation object that uses the pw_load as a parent. 

### Published Outputs

Output Name  | Quantity type  | Description   
---|---|---  
load_voltage  | complex Volts  | The voltage as dictated by the PowerWorld bus in the model.   
  
## pw_recorder

The pw_recorder is a special-purpose recording device that directly accesses PowerWorld objects through the API, which cannot be done using a traditional recorder. This gives visibility into objects on the 'other side of the fence' from GridLAB-D. 

The pw_recorder's interface is a cross between the recorder's and a pw_load object. The outfile, properties, interval, and limit parameters mirror the recorder. The obj_class, key_strings, and key_value describe what object is to be recorded from PowerWorld. The model property, or the parent object, must be the pw_model that the desired object can be found in. 

Key_strings and key_values are iterated as a pair: the strings must be the PowerWorld key fields, and the values provide the uniquely identifying information for which PowerWorld object is to be read from the model. 

Once the target object is found, the pw_recorder will write the specified PowerWorld object properties to the outfile on the specified interval, until the write limit has been reached. 

### Usage

A pw_recorder that measures the voltage and power from a load: 
    
    
    object pw_recorder{
       parent myModel;
       obj_class load;
       key_strings BusNum,LoadID;
       key_values 5,1;
       properties BusKVVolt,LoadMVA,LoadMW;
       interval 60;
       limit 100;
       outfile pw_rec_out.csv;
    }
    

### Published Inputs

Input Name  | Quantity type  | Description   
---|---|---  
model  | pw_model object  | The PowerWorld model object to monitor. If no model is specified, the pw_recorder will check if there is a parent object that is a pw_model, and will fill 'model' with the parent. If a model has not been selected by sync time, the pw_recorder will remain inactive.   
outfile_name  | string  | The file path to use for the output file. If blank, a name will be automatically generated from the pw_recorder's model and GridLAB-D ID number.   
obj_class  | string  | The name of the class of the PowerWorld object that will be recorded.   
key_strings  | string  | A comma-delimited list of key fields required to identify the PowerWorld target object.   
key_values  | string  | A comma-delimited list of key field values that uniquely identify the PowerWorld target object.   
properties  | string  | A comma-delimited list of fields to record from the PowerWorld target object.   
interval  | integer  | The number of seconds to wait between writing lines to the recorder. Interval must be positive.   
limit  | integer  | The maximum number of lines to write to a file.   
  
## Substation Property Hooks

The substation object in the GridLAB-D powerflow module performs two objectives. The substation reads the load_voltage property from the pw_load parent, if present, and converts this positive sequence value to the equivalent balanced three-phase voltages to act as the swing bus voltages for the powerflow solution. The substation takes the three phase unbalanced power solution seen at the substation node, calculates the average power on the phases, and writes this average to the load_power property in the pw_load parent, if present. The substation node also passes positive sequence ZIP components, explicitly set at the substation, to the pw_load parent. In addition, there is a property that allows the user to specify which phase at the substation is the reference phase for the GridLAB-D powerflow solution. The substation object is updated to keep track of the three phase power solution. 

Substation continues to be a child class of the node object inside the powerflow module. A typical substation implementation is 
    
    
    object substation {
    	name SubS;
    	bustype SWING;
            parent network_node;
            reference_PHASE_A;
            phase ABCN;
    	nominal_voltage 7199.558;
    }
    

Listed below are the additional properties that interact with the pw_load object. 

### Published Inputs

Input Name  | Quantity type  | Description   
---|---|---  
positive_sequence_voltage  | complex Volts  | The positive sequence voltage given from the PowerWorld bus model.   
reference_phase  | enumeration  | The phase that will be used as the reference angle for the powerflow solution. <br/> - PHASE_A(Default) <br/> -PHASE_B <br/> -PHASE_C  
transmission_level_constant_power_load  | complex Volt-Amperes  | the positive-sequence constant power load to be posted directly to the pw_load object (powerflow solver does not handle this, it is explicitly converted and posted to PowerWorld's solver).   
transmission_level_constant_impedance_load  | complex Ohms  | the positive-sequence constant impedance load to be posted directly to the pw_load object (powerflow solver does not handle this, it is explicitly converted and posted to PowerWorld's solver).   
transmission_level_constant_current_load  | complex Amperes  | the positive-sequence constant current load to be posted directly to the pw_load object (powerflow solver does not handle this, it is explicitly converted and posted to PowerWorld's solver).   
  
### Published Outputs

Output Name  | Quantity type  | Description   
---|---|---  
average_distribution_load  | complex Volt-Amperes  | The average of the distribution system loads on all three phases at the substation object.   
distribution_power_A  | complex Volt-Amperes  | The measured power of the attached powerflow on phase A.   
distribution_power_B  | complex Volt-Amperes  | The measured power of the attached powerflow on phase B.   
distribution_power_C  | complex Volt-Amperes  | The measured power of the attached powerflow on phase C.   
  
Please note that the transmission current and impedance loads are converted to complex power values first and then posted to the proper properties(load_current and load_impedance) in the pw_load object. The average_transmission_power_load value must be added to the average_distribution_load before posting to pw_load(load_power). 

# See also

  * Powerflow module
  * Requirements
  * Specifications
  * Implementation
  * User's manual
  * Navajo (trunk)

