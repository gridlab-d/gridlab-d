# Spec:Initialization

**Source URL:** https://GridLAB-D™.shoutwiki.com/wiki/Spec:Initialization
# Spec:Initialization

Approval item:  SPECIFICATION REVIEW NEEDED 

## S1

Creation order
    Prior to Hassayampa (Version 3.0) objects shall be initialized in the order in which they are created. As of Hassayampa (Version 3.0) objects shall be initialized in the order in which they are created only when the init_sequence global variable is set to CREATION. If any object init() returns 0, the entire model initialization sequence shall be regarding as having failed and the simulation shall not start. If all object init() functions return 1, the initialization shall be regarded as successful and the simulation shall proceed with the main event loop. Any other return value is considered an error. (R1).

## S2

Deferred initialization
    As of Hassayampa (Version 3.0), when init_sequence is set to DEFERRED objects shall be initialized in the order in which they are created, provided that when init() call returns the value 2 indicating that deferred initialization is desired, the initialization sequence shall call init() again after all other object init() calls are done. This process shall be repeated for all deferred objects until all object init() calls return the value 1. This process shall be repeated no more than init_max_defer times for any given object (R2).

## S3

Rank initialization
    As of Hassayampa (Version 3.0), when init_sequence is set to BOTTOMUP or TOPDOWN, objects shall be initialized in the bottom-up or top-down rank order, respectively, with consideration given to deferred return values from init() as described in S2 (R3)

## S4

Object locking
    As of Hassayampa (Version 3.0), while init() is running, the object shall be locked and the OF_LOCKED flags shall be set (R4).

## S5

Deferred flag
    As of Hassayampa (Version 3.0), when the object initialization is deferred, the object flag OF_DEFERRED shall be set (R5).

## S6

Automatic deferral
    As of Hassayampa (Version 3.0), when init_sequence is set to AUTO automatic object deferral shall be enabled (R6).

## S7

Automatic deferral class override
    As of Hassayampa (Version 3.0), the flag PC_NODEFERRAL shall be used in the _class_register_() call to indicate that objects of the class should not be included in any automatic deferral determination (R7).

Automatic deferral object override
    As of Hassayampa (Version 3.0), the flag PF_DEFER shall be used in the _property_register_() call to indicate that objects of the class should be included in any automatic deferral determination when PC_NODEFERRAL is specified, or should not be included when PC_NODEFERRAL is not specified (R7).

## S8

Initialization time
    As of Hassayampa (Version 3.0), the global clock and all object clocks shall be set to the starttime immediately after the objects are created and before the GLM values are set (R8).

## S9

Initialization state
    As of Hassayampa (Version 3.0), all objects will have the OF_INIT flag set until after initialization is successfully completed (R9).

## Related Concepts:

  * Initialization
    * Requirements
    * Specification
    * Technical manuals
