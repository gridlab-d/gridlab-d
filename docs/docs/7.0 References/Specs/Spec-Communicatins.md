# Communications

**Source URL:** https://GridLAB-D™.shoutwiki.com/wiki/Communications
## Contents

  * 1 Communications Module
    * 1.1 Network Object
      * 1.1.1 Network Object Inputs
    * 1.2 Network Interface Device
      * 1.2.1 Network Interface Device Inputs
      * 1.2.2 How information will be passed
The Communications Applications Concepts is located at [V3_applications_concepts#Communications]. 

# Communications Module

The communication module will be based upon a simple performance model. Essentially, it will perform as a dispatching “black box”. Messages or commands are created by external objects, delivered into the network model via a network interface device, network delays are applied, and the message is delivered to the appropriate object via another network interface device. The “black box” will initially consist of a number of simplified network properties to demonstrate the anticipated delay times found within the network. 

The following will be broken into three sections, each dealing with different aspects that will need to be addressed to develop the communications module. First, descriptions and requirements of the “black box” network will be provided. Second, parameters and behaviors of the network interface devices will be described. Finally, the general modifications required to existing objects for usage of the communication module will be described. 

## Network Object

The network object will collect all of the applicable signals desired by the overlaying system and then determine the amount of time necessary to deliver the signal to the appropriate object. Actual collection and delivery of the messages will be handled by the network interface device, and will be further described in a later section. As an initial model, the network module will only be concerned with a few parameters; these will include average network latency, bandwidth limitations, and queuing of messages during high levels of congestion. In addition to accepting and delivering messages, the network object will need to communicate to the network interface device any cases where the message was not accepted by the network due to limitations. All of the routing information contained within a standard network will not be explicitly modeled. 

### Network Object Inputs

**Table 1: Network inputs.** _**Property**_ | **Unit** | **Description**  
---|---|---  
_latency_ |  fractions of seconds  | The latency of the network can loosely be described as the average amount of time it takes to transmit information, regardless of the size of the information. This is measured as the amount of time it takes the beginning of a message to be moved from the source to the destination. This value may be specified as a constant value, a time-varying value (players), a dependent variable (via inline coding), or as a random variable which varies over time (see _latency_distribution_).   
_latency_distribution_ | name | This is only needed if the user wishes to create a latency that is somewhat randomized over time. In conjunction with _latency_period_ , _latency_dist_value_1_ , and _latency_dist_value_2_ , a latency value will be randomly selected each period interval from a predefined distribution.   
_latency_period_ |  seconds | Only used in conjunction with _latency_distribution_ to define how often the latency is updated.   
_latency_dist_value_1_ _latency_dist_value_2_ | number | These values will define the distributions selected. Value 1 will define the mean, lower limit, alpha, a, or lambda, depending upon the distribution. Value 2 will determine the standard deviation, upper limit, beta, b, or k (or nothing in some cases). Refer to random.c for clarification on variables.   
_bandwidth_ | Mb/s | Bandwidth determines the maximum data rate that can be achieved by the network. In this system, it will simply represent a cap that cannot be exceeded. If the cap is exceeded, two modes for handling may be engaged; a queued system handled by the network, or a fail and resend method handled by the network interface device. These represent simple models and may be added on to later.   
_congestion_ | ? | Congestion loosely refers to the amount of traffic within the system. As is may have effects on latency, and later, reliability, this will be included as a modifier to these various terms. For example, this may be used: _latency = congestion * bitrate / bandwidth_. This term is only added at this time for future uses and will have no operational value at this time.   
_queue_resolution_ | name | Queue resolution will determine the method in which the network will handle an overloaded system. Two methods will exist: _queued_ and _reject_. _Queued_ will work in conjunction with _buffer_size_ and will store a message in a queue until bandwidth opens up to deliver the message. New messages will be added to the end of the queue and packets will be delivered into the network in the order they were received. In the queue becomes full, information will be lost with no notification. _Reject_ mode will not have a buffer, but will send a rejection signal back to the network interface device to inform it that the packet was rejected due to the bandwidth being exceeded. The network interface device will be required to try again (or not), and will need to store its own information for resubmission.   
_buffer_size_ | Mb | This will define the size of the buffer. See _queue_resolution_ for operation.   
_timeout_ | fraction of seconds | Used in conjunction with the network interface device _duplex_ function. If specified as _half_duplex_ , this determines how long the network will hold onto a message that could not be delivered due to the interface device sending a signal. After the specified time period, the message will be lost with no notification. It will not be limited by a buffer size.   
  
## Network Interface Device

The network interface device will be required to bridge the communications module with any other existing module. This is analogous to triplex meters in power flow being used to attach residential models to the power system; however, due to the constraints of the parent-child relationships, the order will need to be reversed. The network interface device will become the child of the object is controlling. Similar to when using the meter object, any object that interfaces with the network module will need additional logic to detect if a communication device is present. Additionally, each object that interfaces with the device will need logic designed specifically for handling the control signals that will be now sent across the communication module as opposed to directly. For example, the volt var control object will need to deliver the modified set points to the network interface device as opposed to directly to the regulator object. For communication to occur between two objects, both will be required to be attached to a network interface device. 

The network interface device will either deliver messages from the object to the network or from the network to the object. It will also handle any processing delays that may be required, or when the network is in _reject_ mode, it will be required to store and resubmit data as necessary. 

### Network Interface Device Inputs

**Table 2: Network Interface Device inputs.** _**Property**_ | **Unit** | **Description**  
---|---|---  
_duplex_ | name | This determines which direction the interface device is able to send information. _simplex_receiver_ will only allow one way communication in and _simplex_transmitter_ will only allow one way communication outbound. _half_duplex_ will allow bi-directional communications, but in only one direction at a time. In the case of both directions trying at the same time, incoming will always take precedent (outgoing information will need to be queued within the interface device, as will it during the case of a current incoming transmission clashing with a new outgoing). In the case of a new incoming message clashing with a current outgoing message, the message will be stored by the network for a period specified by _timeout_ in the network module. _full_duplex_ will be the default value, and allows for bi-directional communication at all times simultaneously.   
_to_ | object name | This will define where the information should be delivered when the network interface device is required to transmit information. This may be a statically specified object determined by the user, or a time variable name determined by the specific object that is connected to the network. For example, a regulator object may need to only connect to the volt var object, so it would be directly specified. For the volt var object, which will require sending messages to multiple objects, this will need to be handled as an internal logic function within the volt var object, and the name of the object may be passed into the _to_ variable.   
_size_ | bits | This defines the size of the message being sent or received. It may be static or vary over time.   
_transfer_rate_ | bits/second | This defines the maximum rate at which the information may be passed to/from the network. For now, this will apply to both directions. This will determine how long the interface device is busy, and may be limited by the available bandwidth within the network itself.   
  
### How information will be passed

Messages will be passed within the communication module as a structure. Minimally, this will contain the _source_ , _destination_ , _size_ , _bandwidth_ , _delay_ , and _data_ , and may be expanded later to include other information. _bandwidth_ will be assigned by the network object and _delay_ will define the amount of latency determined by the network object. 


