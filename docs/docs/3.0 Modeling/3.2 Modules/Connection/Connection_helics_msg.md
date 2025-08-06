# helics msg

The helics_msg object is part of the `connection` module. It allows GridLAB-D models to be run as federates in a HELICS co-simulation. In 4.1 the helics_msg object is available by default. 

### Enabling the helics_msg object

When building from source. HELICS is a third party library that can be downloaded and built from [here](https://github.com/GMLC-TDC/HELICS-src//). Once HELICS and its third party libraries, ZeroMQ and boost, have been built, please follow the [cmake build](http://gridlab-d.shoutwiki.com/wiki/CMake_Build//) instructions for enabling HELICS in GridLAB-D. 

### Environment Considerations

The following environment variables must have the location to the HELICS library and its third party libraries, ZeroMQ and boost. 

OS  | Environment Variable   
---|---  
Windows  | PATH   
Mac  | DYLD_LIBRARY_PATH   
Linux  | LD_LIBRARY_PATH   
  
## Helics_msg

The helics_msg object implements the HELICS library such that a single GridLAB-D model acts as a single HELICS federate. There can only be one instance of a helics_msg object per GridLAB-D model. 

### Default Helics_msg

A minimalist helics_msg could be created with 
    
    
    module connection;
    object helics_msg{
         name federate1;
         configure federate1Configuration.txt;
    }
    

### Helics_msg Parameters

Property Name  | Type  | Unit  | Description   
---|---|---|---  
name  | string  | none  | This is the name of the object in the GridLAB-D context. It is also the name of the HELICS federate. This means that all published property topics get prepended with this name like so: name/publish_topic   
configure  | string  | none  | The name of the file used to configure the HELICS federate.   
  
### Configuration File Syntax

#### HELICS 1.X Library

The configuration file defines the publication and subscription topics as well as the broker location helics federate options. The file is a json file with the following schema 
    
    
    {
         “publications” : {
              “globals” : {
                   <global property name> : <subtopic name>,
                   …
              },
              <object name> : {
                   <property name> : <subtopic name>,
                   …
              },
              …
         },
         “subscriptions” : {
              “globals” : {
                   <global property name> : <topic name>,
                   …
              },
              <object name> : {
                   <property name> : <topic name>,
                   …
              },
              …
         },
         “endpoint publications” : {
              “globals” : {
                   <global property name> : <destination endpoint name>,
                   …
              },
              <object name> : {
                   <property name> : < destination endpoint name >,
                   …
              },
              …
         },
         “endpoint_subscriptions” : {
              “globals” : {
                   <global property name> : <endpoint name>,
                   …
              },
              <object name> : {
                   <property name> : <endpoint name>,
                   …
              },
              …
         },
         “core_init_string” : <string>,
         “core_name” : <string>,
         “core_type” : <string>,
         “broker_address” : <string>,
         “time_delta” : <double in seconds>,
         “input_delay” : <double in seconds>,
         “rttolerance” : <double in seconds>,
         “output_delay” : <double in seconds>,
         “period” : <double in seconds>,
         “offset” : <double in seconds>,
         “maximum_iterations” : <integer>,
         “separator” : <single character string>,
         “observer” : <boolean>,
         “rollback” : <boolean>,
         “only_update_on_change” : <boolean>,
         “only_transmit_on_change” : <boolean>,
         “source_only” : <boolean>,
         “uninterruptible” : <boolean>,
         “interruptible” : <boolean>,
         “forward_compute” : <boolean>,
         “real_time” : <boolean>,
         “delayed_update” : <boolean>
    }
    

All of the keys found in the json schema are optional. The example below shows federate configuration that publishes a load, ties a subscription to a substation voltage, and creates an endpoint for the fixed price of a market object for other federates to write to. 
    
    
    {
         "publications" : {
              "network_node" : {
                   "distribution_load" : "total_load"
              }
         },
         "subscriptions" : {
              "network_node" : {
                   "positive_sequence_voltage" : "transmision_sim/b_2_voltage"
              }
         },
         "endpoint_subscriptions" : {
              "market_1" : {
                   "fixed_price" : "fixed_price_endpoint"
              }
         },
         "core_init_string" : "1"
    }
    

#### HELICS 2.X

The configuration file defines the publication and subscription topics as well as the broker location helics federate options. The file is a json file with the following schema 
    
    
    {
         "broker" : string, //The address of the broker to connect to if the core hasn't connected to a broker already
         "core_init_string" : string, //The core_init_string to use if we have to create the core.
         "core_name" : string, //The name of the core to connect to if it exists otherwise create a core with this name.
         "core_type" : string, //The type of core the federate should be connecting to.
         "input_delay" : double, //The time in seconds to delay recieved data.
         "log_level" : int, //The level of verbosity to output to the logs. valid levels are 1-3?
         "max_iterations" : int, //The maximum number of times the federate can reiterate?
         "name" : string, //The federate name.
         "offset" : int, //offset from the period
         "output_delay" double, //The time in seconds to delay sending data.
         "period" : double, //The unit is in seconds. define this if the federate must return only on a multiple of this value.
         "rt_lag" : double, //The amount of time in seconds the federate is allowed to lag real time before corrective action is taken.
         "rt_lead" : double, //The amount of time in seconds the federate is allowed to lead real time before corrective action is taken.
         "rt_tolerance" : double, //The time tolerance in seconds of the real time mode.
         "separator" : string, //The character used separate the federate name from the publication key in the creation of the publication topic.
         "time_delta" : double, //The minimum time delta a federate can return to process an unexpected message.
         "flags" : {
              "delayed_update" : bool, //delay between calling publish and actually publishing the value on the HELICS message bus.
              "forward_compute" : bool, 
              "interruptible" : bool, //Flag that indicates the federate can return earlier than it's requested time to return.
              "observer" : bool, //Flag that indicates that this federate does not publish anything.
              "only_transmit_on_change" : bool, //Flag for publications/endpoints that indicates that the federate should only send if the value is different than the previous sent value.
              "only_update_on_change" : bool, //Flag for subscriptions/endpoints that only update the federate subscription/endpoint if the value is different than the previous value.
              "realtime" : bool, //Flag that indicates that this federate needs to run in real time.
              "source_only": bool, //Flag that indicates that this federate doesn't recieve messages of any type but publishes/sends data only.
              "uninterruptible" : bool, //Flag that indicates that this federate can't return earlier than the it's requested time to return.
              "wait_for_current_tiume" : bool 
         },
         "endpoints" : [
              {
                   "global" : bool, //Flag to indicate this endpoint ss a global enpoint which means the federate name will not be prepended to the endpoint name followed by the separator.
                   "knownDestinations" : string or [strings], //The enpoint name(s) to send messages to from this endpoint.
                   "knownSubscription" : string or [strings], //The publication(s) to subscribe this endpoint to.
                   "name" : string, //The name of the endpoint
                   "type" : string //The data type this endpoint sends and recieves.
                   "info" : "{
                        \"object\" : <object_name> or \"global\", //Can contain a specific GridLAB-D object instance name or global if you want to publish a GridLAB-D global property.
                        \"property\" : <object_property_name> or <global_property_name>, //Contains the name of the GridLAB-D object property or GridLAB-D global property
                   }",
              },
              ...
         ],
         "publications" : [
              {
                   "global" : bool, //Flag to indicate this publication s s a global publication which means the federate name will not be prepended to the publication name followed by the separator.
                   "key" : string, //The name of the publication.
                   "type" : string, //the data type this publication publishes.
                   "unit" : string //The units associated with the data.
                   "info" : "{
                        \"object\" : <object_name> or \"global\", //Can contain a specific GridLAB-D object instance name or global if you want to publish a GridLAB-D global property.
                        \"property\" : <object_property_name> or <global_property_name>, //Contains the name of the GridLAB-D object property or GridLAB-D global property
                   }",
              },
              ...
         ],
         "subscriptions" : [
              {
                   "key" : string, //The name of the publication topic to subscribe to.
                   "required" : bool, //Flag that if set the federate will throw an error is the publication doesn't exist.
                   "type" : string //The data type this subscription holds.
                   "info" : "{
                        \"object\" : <object_name> or \"global\", //Can contain a specific GridLAB-D object instance name or global if you want to publish a GridLAB-D global property.
                        \"property\" : <object_property_name> or <global_property_name>, //Contains the name of the GridLAB-D object property or GridLAB-D global property
                   }",
              },
              ...
         ]
    }
    

All of the keys found in the json schema are optional. The example below shows federate configuration that publishes a load, ties a subscription to a substation voltage, and creates an endpoint for the fixed price of a market object for other federates to write to. 
    
    
    {
         "name" : "fed1",
         "period" : 1.0,
         "publications" : [
              {
                   "global" : false,
                   "key" : "total_load",
                   "type" : "complex",
                   "unit" : "VA",
                   "info" : "{
                        \"object\" : \"network_node\",
                        \"property\" : \"distribution_load\"
                   }"
              }
         ],
         "subscriptions" : [
              {
                   "key" : "transmission_sim/b_2_voltage",
                   "type" : "complex",
                   "unit" : "V",
                   "info" : "{
                        \"object\" : \"network_node\",
                        \"property\" : \"positive_sequence_voltage\"
                   }"
              }
         ],
         "endpoints" : [
              {
                   "global" : false,
                   "name" : "fixed_price_point",
                   "type" : "double",
                   "info" : "{
                        \"object\" : \"market_1\",
                        \"property\" : \"fixed_price\"
                   }"
              }
         ]
    }
    

#### HELICS 3.X and GridLAB-D 5.X

The configuration file defines the publication and subscription topics as well as the broker location helics federate options. The file is a json file with the following schema 
    
    
    {
         "broker"": string, //The address of the broker to connect to if the core hasn't connected to a broker already
         "core_init_string"g: string, //The core_init_string to use if we have to create the core.
         "core_name"m: string, //The name of the core to connect to if it exists otherwise create a core with this name.
         "core_type"y: string, //The type of core the federate should be connecting to.
         "input_delay"e: double, //The time in seconds to delay recieved data.
         "log_level"l: int, //The level of verbosity to output to the logs. valid levels are 1-3?
         "max_iterations"a: int, //The maximum number of times the federate can reiterate?
         "name" : string, //The federate name.
         "offset" : int, //offset from the period
         "output_delay" double, //The time in seconds to delay sending data.
         "period" : double, //The unit is in seconds. define this if the federate must return only on a multiple of this value.
         "rt_lag" : double, //The amount of time in seconds the federate is allowed to lag real time before corrective action is taken.
         "rt_lead" : double, //The amount of time in seconds the federate is allowed to lead real time before corrective action is taken.
         "rt_tolerance"": double, //The time tolerance in seconds of the real time mode.
         "separator" : string, //The character used separate the federate name from the publication key in the creation of the publication topic.
         "time_delta" : double, //The minimum time delta a federate can return to process an unexpected message.
         "flags"g: {
              "delayed_update" : bool, //delay between calling publish and actually publishing the value on the HELICS message bus.
              "forward_compute" : bool, 
              "interruptible" : bool, //Flag that indicates the federate can return earlier than it's requested time to return.
              "observer" : bool, //Flag that indicates that this federate does not publish anything.
              "only_transmit_on_change"l: bool, //Flag for publications/endpoints that indicates that the federate should only send if the value is different than the previous sent value.
              "only_update_on_change"": bool, //Flag for subscriptions/endpoints that only update the federate subscription/endpoint if the value is different than the previous value.
              "realtime"e: bool, //Flag that indicates that this federate needs to run in real time.
              "source_only": bool, //Flag that indicates that this federate doesn't recieve messages of any type but publishes/sends data only.
              "uninterruptible" : bool, //Flag that indicates that this federate can't return earlier than the it's requested time to return.
              "wait_for_current_tiume" : bool 
         },
         "endpoints"l: [
              {
                   "global" : bool, //Flag to indicate this endpoint ss a global enpoint which means the federate name will not be prepended to the endpoint name followed by the separator.
                   "destination"r: string or [strings], //The enpoint name(s) to send messages to from this endpoint.
                   "name" : string, //The name of the endpoint
                   "type"h: string //The data type this endpoint sends and recieves.
                   "info"n: {
                        "object"o: <object_name> or "global", //Can contain a specific GridLAB-D object instance name or global if you want to send/recieve a GridLAB-D global property.
                        "property" : <object_property_name> or <global_property_name>, //Contains the name of the GridLAB-D object property or GridLAB-D global property
                   },
              },
              ...
         ],
         "publications" : [
              {
                   "global" : bool, //Flag to indicate this publication s s a global publication which means the federate name will not be prepended to the publication name followed by the separator.
                   "name" : string, //The name of the publication.
                   "type" : string, //the data type this publication publishes.
                   "unit" : string //The units associated with the data.
                   "info" : {
                        "object" : <object_name> or "global", //Can contain a specific GridLAB-D object instance name or global if you want to publish a GridLAB-D global property.
                        "property"D: <object_property_name> or <global_property_name>, //Contains the name of the GridLAB-D object property or GridLAB-D global property
                   },
              },
              ...
         ],
         "inputs"}: [
              {
                   "target"
    : string, //The name of the publication topic to subscribe to.
                   "required"l: bool, //Flag that if set the federate will throw an error is the publication doesn't exist.
                   "type"o: string //The data type this subscription holds.
                   "info"h: {
                        "object"l: <object_name> or "global", //Can contain a specific GridLAB-D object instance name or global if you want to write to a GridLAB-D global property.
                        "property"h: <object_property_name> or <global_property_name>, //Contains the name of the GridLAB-D object property or GridLAB-D global property
                   },
              },
              ...
         ]
    }
    

All of the keys found in the json schema are optional. The example below shows federate configuration that creates a HELICS publication for a substation object's distribution_load property, ties a HELICS input to a substation object's positive_sequence_voltage property, and creates a HELICS endpoint for the fixed_price property of a market object for other federates to write to. 
    
    
    {
         "name" : "fed1",
         "period" : 1.0,
         "publications" : [
              {
                   "global" : false,
                   "name" : "total_load",
                   "type" : "complex",
                   "unit" : "VA",
                   "info" : {
                        "object" : "network_node",
                        "property" : "distribution_load"
                   }
              }
         ],
         "inputs" : [
              {
                   "target" : "transmission_sim/b_2_voltage",
                   "type" : "complex",
                   "unit" : "V",
                   "info" : {
                        "object" : "network_node",
                        "property" : "positive_sequence_voltage"
                   }
              }
         ],
         "endpoints" : [
              {
                   "global" : false,
                   "name" : "fixed_price_point",
                   "type" : "double",
                   "info" : {
                        "object" : "market_1",
                        "property" : "fixed_price"
                   }
              }
         ]
    }
    

All configuration examples thus far have created HELICS publications, inputs, and endpoints that are tied to a single internal GridLAB-D object instance's property. It is now possible to configure a HELICS publication such that it publishes multiple objects' properties. The HELICS publication type must be a string and GridLAB-D will publish a JSON serialized string of specifed objects' properties. for example, let's create a single HELICS publication that will publish a triplex_meter's measured_power and measured_real_power properties as well as a house's heating_setpoint, cooling_setpoint, thermostat_cycle_time, and heating_system_type properties. The HELICS configuration file's publication instance would look something like this: 
    
    
    {
        "global": false,
        "name": "json_pub",
        "type": "string",
        "info": {
            "message_type": "JSON",
            "publication_info": {
                "tpxm": [
                    "measured_power",
                    "measured_real_power"
                ],
                "house1": [
                    "heating_setpoint",
                    "cooling_setpoint",
                    "thermostat_cycle_time",
                    "heating_system_type"
                ]
            }
        }
    }
    

Something similar can be done for a HELICS input. It is possible to create a HELICS input that could write to multiple objects' properties in the GridLAB-D model. The message it expects is a JSON serialized string containing the object names and properties as well as the values of those properties. First let's look at how to set up the HELICS input instance in the configuration file. 
    
    
    {
        "global": false,
        "target": "python_federate/json_pub",
        "type": "string",
        "info": {
            "message_type": "JSON"
        }
    }
    

In another federate named "python_federate" it's now possible to set the constant_power_12 property of a triplex_load named "tpxl" and the heating_setpoint, thermostat_cycle_time, and heating_system_type properties of a house named house1 publishing the following string in a publication named "json_pub": 
    
    
    "{
         \"tpxl\": {
             \"constant_power_12\": \"50.56-25.362j W\"
         },
         \"house1\": {
             \"heating_setpoint\": 55.6,
             \"thermostat_cycle_time\": 150,
             \"heating_system_type\": \"HEAT_PUMP\"
         }
    }"
    

Also, with endpoints it possible to create an endpoint that will send a message containing a single GridLAB-D object instance's property and to write received messages to a different object instance's property. The HELICS endpoint configuration instance would look something like this: 
    
    
    {
        "global": false,
        "name": "string_ep",
        "type": "string",
        "destination": "python_federate/string_ep",
        "info": {
            "publication_info":{
                "object": "tpxm",
                "property": "measured_real_power"
            },
            "subscription_info":{
                "object":"house1",
                "property":"cooling_setpoint"
            }
        }
    }
    

It's also possible to create an endpoint that will send a message containing multiple objects' properties and receive a message to write to any existing objects' properties. The example below shows a endpoint configuration instance that sends the measured_power and measured_real_power properties of a triplex_meter named tpxm, the heating_setpoint, cooling_setpoint, thermostat_cycle_time, and heating_system_type properties of a house named house1, and the global property, clock. 
    
    
    {
        "global": false,
        "name": "json_ep",
        "type": "string",
        "destination": "python_federate/json_ep",
        "info": {
            "message_type": "JSON",
            "receives_messages": true,
            "publication_info": {
                "tpxm": [
                    "measured_power",
                    "measured_real_power"
                ],
                "house1": [
                    "heating_setpoint",
                    "cooling_setpoint",
                    "thermostat_cycle_time",
                    "heating_system_type"
                ],
                "globals": [
                    "clock"
                ],
            }
        }
    }
    


