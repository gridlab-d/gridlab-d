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
    

