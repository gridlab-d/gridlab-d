# Model Initialization

Models are initialized after the GLM file is loaded but before the clock start and the first pre-commit or sync calls. 

## Before [Hassayampa (Version 3.0)]

Objects are initialized in the order in which they are defined in the GLM file. Class implementations can perform initialization during the first pre-commit or first sync events if necessary. 

## [Hassayampa (Version 3.0)] and later

The order in which objects are initialized depends on the value of the [init_sequence] [global variable]: 

`#set [init_sequence]=[CREATION]`
    Objects are initialized in the order in which they are defined in the GLM file. This is the backward compatibility initialization sequence and is the default of all versions before [Hassayampa (Version 3.0)]

`#set [init_sequence]=[DEFERRED]`
    Objects are initialized in the order in which they are created. However objects are allowed to request deferred initialization if they need to wait for another object to be initialized first. Objects that defer initialization will be called again until they no longer defer init. The maximum number of deferrals is specified by the [init_max_defer] [global variable]. This method is the default for [Hassayampa (Version 3.0)] and later.

`#set [init_sequence]=[BOTTOMUP]`
    Objects are initialized in the bottom-up rank order. Within each rank objects are initialized in the order in which they are created using deferred behavior.

`#set [init_sequence]=[TOPDOWN]`
    Objects are initialized in the top-down rank order. Within each rank objects are initialized in the order in which they are created using deferred behavior.

`#set [init_sequence]=[AUTO]`
    Objects are initialized in the order in which they are created, provided that all the objects on which they depend (parent and object properties) are initialized already. Otherwise, the dependent objects' initializations are deferred.

## Version

The deferred, bottom-up, and top-down initialization methods were introduced in [Hassayampa (Version 3.0)]. 

## See also

  * Initialization
    * [Requirements]
    * [Specification]
    * [Technical manuals]

