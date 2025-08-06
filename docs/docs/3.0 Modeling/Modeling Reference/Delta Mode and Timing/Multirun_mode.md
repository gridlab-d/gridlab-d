# Multirun mode

Approval item:  \--Dchassin 16:48, 9 October 2011 (UTC) 

Multirun mode is used to run multiple models simultaneously with linked objects. Multirun mode is automatically enabled when an instance directive is used. One additional slave instance of GridLAB-D is started for each block defined in the master instance. Using an instance directive in a slave instance will result in unspecified and undefined behavior. 

# Overview

Multirun mode allows a GridLAB-D user to partition a large model as they see fit, splitting dependent groups of objects into smaller groups. This allows the individual models to be simpler, and for the individual runs to operate on smaller data sets. Instances can communicate over shared memory or though network APIs. There is the limitation that the slave instances cannot communicate with each other, and that function calls cannot be directly made between objects in different instances. 

# Behavior

In Multirun mode, GridLAB-D is called normally with a GLM file. If the slaves are to be run on foreign nodes, instances of GridLAB-D must be running on those systems with the "--slavenode" parameter, which will leave them running as servers for the instancing model. 

## Master Load-time

The 'master' GLM file is informally denoted as such by the presence of an instance block. This block is read in at load-time, but not acted on until after the random numbers, process scheduling, and main loop handler have all been initialized. 

## Initialization

When the instance block is initialized, the core will flag itself as being in 'multirun master' mode and initialize an output prefix, typically "M00". The master will check that the instance block will not try to recursively open the master file, validate the connection mode type, then initialize the linkage directives. This allows accurate buffer calculations to be made, leading to memory allocation, writing headers, and copying in linkage names. 

### Establishing a Connection

Once the data initialization is complete, the master will construct the communication medium and populate it with the data required to recognize that a link has been established. If the connection is via shared memory (on Windows), the header data will be copied to the shared block, and system event signals will be created for synchronization. If the connection is done with a socket, the master will send a message that contains the string "GLDMTR", and will expect "GLDSND" in response from the slavenode. A 'callback' socket will be constructed for the slave instance to connect to, and a message will be sent to the slavenode that contains the target directory and the name of the model file to run, the port to connect to, and some system information. 

### Handler Thread

Having set the ground work for the slavenode to run, the master starts a thread that will ensure that the slave instance remains running. If shared memory is used, then the thread does a system() call, and reports when the call has completed with the termination of the slave instance process. If a socket is used, the thread shall use a blocking recv() to wait for incoming data messages, and will signal that the slave instance has stopped when the socket closes. 

## Slave Node

GridLAB-D is able to start runs on remote hosts, should an instance be running with the "--slavenode" parameter. By default, this will use port 6267, but can be changed by setting the "slave_port" value. A copy of GridLAB-D running as a slave node will not shut down until it fails to spawn a slave instance, or until forcefully stopped. 

One slave node is able to handle any number of GridLAB-D node requests. 

## Slave Load-Time

The slave instances are started by another instance of GridLAB-D and include the "--slave" parameter. The GLM and input files must be located locally to the GridLAB-D instance that will run the model, and any output files will be written to that system. 

### Parameters

The --slave parameter's input token is a string with a "host:port" syntax. If this string is "localhost:resource name", GridLAB-D will start in shared memory mode and look for the specified shared memory resource. Otherwise, it will attempt to open a TCP socket to the specified host. By default, the 'resource name' is internally generated, and the socket port randomly selected by the OS. 

# See Also

  * Multirun mode
    * Directives
    * instance
    * linkage
    * master
    * slave
  * Technical documents 
    * Requirements
    * Specifications
    * Technical manuals
    * Validation
