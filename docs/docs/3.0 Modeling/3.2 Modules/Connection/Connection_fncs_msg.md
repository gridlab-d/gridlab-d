# fncs msg

The fncs_msg object is part of the `connection` module. It allows GridLAB-D models to be run as federates in a FNCS co-simulation. Starting in 4.0 this object is not available in the default 4.0 distribution of GridLAB-D but can be compiled with FNCS from the source code. In 4.1 the fncs_msg object is available by default. 

### Enabling the fncs_msg object

Prior to the 4.1 release of GridLAB-D the fncs_msg object must be custom compiled from source code by dynamically linking with the FNCS library. FNCS is a third party library that can be downloaded and built from [here](https://github.com/FNCS//). Once FNCS and its third party libraries, ZeroMQ and CZMQ, have been built, the following option needs to be added to the configure step for compiling GridLAB-D. 
    
    
    --with-fncs=<location to where FNCS and its third party libraries were installed>
    

### Environment Considerations

Prior to the 4.1 release of GridLAB-D the OS environment must be properly set up to run the FNCS library from GridLAB-D if FNCS was installed to a custom location and not the system default. The following environment variables must have the location to the FNCS library and it's third party libraries, ZeroMQ and CZMQ. 

OS  | Environment Variable   
---|---  
Windows  | PATH   
Mac  | DYLD_LIBRARY_PATH   
Linux  | LD_LIBRARY_PATH   
  
## Fncs_msg

The fncs_msg object implements the FNCS library such that a single GridLAB-D model acts as a single FNCS federate. There can only be one instance of a fncs_msg object per GridLAB-D model. 

### Default Fncs_msg

A minimalist fncs_msg could be created with 
    
    
    object fncs_msg{
         name federate1;
         option "transport:hostname localhost, port 5570";
         configure federate1Configuration.txt;
    }
    

Which is the same a specifying 
    
    
    object fncs_msg{
         name federate1;
         option "transport:hostname localhost, port 5570";
         message_type GENERAL;
         configure federate1Configuration.txt;
    }
    

### Fncs_msg Parameters

Property Name  | Type  | Unit  | Description   
---|---|---|---  
name  | string  | none  | This is the name of the object in the GridLAB-D context. It is also the name of the FNCS federate. This means that all published property topics get prepended with this name like so: name/publish_topic   
option  | string  | none  | This parameter is used by the fncs_msg object to define the FNCS broker object location. (ie, IP address and port number)   
message_type  | enumeration  | none  | An enumeration setting to define the structure of outgoing and incoming messages. The valid choices are <br/> - `GENERAL` all messages contain the value for a single object's property. <br/> - `JSON` messages can contain values for multiple object properties. 
configure  | string  | none  | The name of the file used to configure the FNCS federate. This format varies depending on the message_type being used.   
  
### Configuration File Syntax

The configuration file defines the publication and subscription topics as well as the broker location options. The syntax differs based on `message_type` used. 

#### GENERAL Configuration

the `GENERAL` messaging structure assumes that each message published and recieved contains a value for a single object's property. So every topic that the FNCS federate publishes or subscribes to corresponds to a specific object's property. The syntax for subscribing to a topic is shown below. 
    
    
    subscribe "precommit|presync|sync|postsync|commit:object_name.property_name <- topic";
    

An example subscription that write the data from a topic to the cooling_setpoint of a house would look like 
    
    
    subscribe "precommit:house1.cooling_setpoint <- federate2/house_cooling_setpoint";
    

The syntax for publishing to a topic is shown below. 
    
    
    publish "precommit|presync|sync|postsync|commit:object_name.property_name -> topic";
    

Below is an example for publishing a house object's indoor air temperature to a specific subtopic 
    
    
    publish "commit:house1.air_temperature -> house1_indoor_air_temperature";
    

Remember that the actual topic that gets published is prepended with the fncs_msg objects name. so If the name of the fncs_msg object is federate1 then the topic that house1's indoor air temperature its published on is federate1/house1_indoor_air_temperature. The precommit|presync|sync|postsync|commit choice determines when GridLAB-D will publish or update the properties during each exec cycle. If you are not sure where in the exec cycle to publish or update a property then, in general, precommit is best for subscriptions and commit is best for publications. a tolerance can be specified for which a property is published. an example is shown below 
    
    
    publish "commit:house1.air_temperature -> house1_indoor_air_temperature; 5";
    

Now the GridLAB-D federate will only publish house1's air_temperature property only if the property has changed by 5 degrees. **Please Note!!!** : Object names cannot contain "<", ">", or "-" as the line parser will fail to parse the publish ad subscribe lines correctly if they do contain them. 

#### JSON Configuration

the `JSON` messaging structure was designed specifically for the GridAPPS-D Platform. It hardcodes the FNCS federate to publish all variables listed in configuration file on one topic in a json structured string and to subscribe to specific topic. It also hard codes the fncs_msg object to always request 1 second time steps. The hardcoded publication topic is <federate name>/fncs_output and the hardcoded subscription topic is <federate name>/fncs_input. The configuration file is expected to be a json formated file with the following schema 
    
    
    {
         "globals" : [<global property>, ...],
         <object name> : [<property name>, ...],
         ...
    }
    

The example shown below shows how to publish the simulation global clock, the tap positions on a regulator and a house's air temperature. 
    
    
    {
         "globals" : ["clock"],
         "regulator12" : ["tap_A", "tap_B", "tap_C"],
         "house23" : ["air_temperature"],
    }
    


  
