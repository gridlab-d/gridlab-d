# Multi recorder

Record properties from multiple objects 

# Synopsis
    
    
    module [tape]
    object multi_recorder {
        interval 60;
        property object_name1:property_name1,object_name2:property_name2,...;
        file output_file.csv;
    }
    

# Remarks

The "multi_recorder" object is a variation on the Recorder that can be used to record properties from multiple objects in a model and write the values to one file. The multi_recorder object property syntax includes the use of the objectname and a colon, such as "node1:voltage_A.real", to determine which object to look into. If the multi_recorder object has a parent, it will use its parent by default if there are properties that do not explicitly specify which object to look into. Unlike the recorder, the parent is an optional field, and may be omitted. If there is no parent, the "target" line of the output file will not be printed. 

All properties and other behaviors of the multi-object recorder are equal to those of the normal recorder. 

# Properties

The multi_recorder object has the same properties as recorder. 

# See Also

[player] [tape]


