# Checkpoints

**TODO**: Add in content from related checkpoint files

A checkpoint is a state save of a long simulation that is only partially completed. Checkpoints can be done periodically either based on the elapsed wall-clock time or elapsed simulation time. When a checkpoint save is performed, a stream dump of the model is output to a checkpoint file. At the time of each checkpoint, the stream file contains the complete internal state of the model, which includes 

  * global variables including module globals
  * internal system states including indexes, multiprocessing states, schedules, loadshapes, end-uses, and transforms
  * objects including all the internal state variables
  * open file handles and flushed positions
  * all other data required to ensure that the simulation can be resumed at the checkpoint

The following global variables are defined for checkpoint saving: 

* **checkpoint_type** -
    This defines the type of checkpoint used. Valid values are {**NONE** , **WALL** , **SIM**}. **WALL** indicates that the **checkpoint_interval** is based on the wall clock time, and **SIM** indicates that the **checkpoint_interval** is based on the simulation clock time. The default value is **NONE**.

* **checkpoint_file** -
    This defines the name of the checkpoint to use. The checkpoint files will be named using a sequence number appended to the checkpoint file name, e.g., _checkpoint_file_._seq_. The default checkpoint file name is the base name of the main GLM file.

* **checkpoint_interval** -
    This defines the interval in seconds between checkpoint saves. The default value is **3600** , or one hour, when **checkpoint_type** is **WALL** and **86400** , or one day, when **checkpoint_type** is **SIM**.

* **checkpoint_keepall** -
    If this value is **TRUE** , all the checkpoint files are kept. If the value is **FALSE** , only the last checkpoint file saved is kept and all previous checkpoint files are delete. The default value is **FALSE** , meaning that only the last checkpoint file is kept.

* **checkpoint_seqnum** -
    This value identifies the sequence number of the next checkpoint file. The default is 0.

Checkpoints are enabled by setting the global variable **checkpoint_type** to either **WALL** or **SIM**. 

A checkpoint file can be loaded and resumed using the stream command line options: 
    
    host% gridlabd --stream _checkpoint_file_._seq_
    

## Examples

Set the checkpoint save to the file **test** every 5 minutes of wall clock time: 
        
    #set checkpoint_type=WALL
    #set checkpoint_file=test
    #set checkpoint_interval=300
    

Set the checkpoint save to the GLM file name every hour of simulation time and keep all checkpoints: 
        
    #set checkpoint_type=SIM
    #set checkpoint_interval=3600
    #set checkpoint_keepall=TRUE