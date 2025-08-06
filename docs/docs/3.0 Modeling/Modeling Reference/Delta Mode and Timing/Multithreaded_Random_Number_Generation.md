# Multithreaded Random Number Generation

With the introduction of full multithreaded synchronization Hassayampa (Version 3.0), the random number generator used in Four Corners (Version 2.2) (RNG2) has become unusable. This page discusses the various reasons for upgrading the random number generator, the requirements for a new random number generation (RNG3) and design considerations for the upgrade. (Some of the reasons for upgrading the random number generator are not related specifically to the introduction of multithreading—they have been known for some time in RNG2 and the problems they cause have not been shown to dramatically affect current users in a way that can't be worked around relatively easily.) 

1) RNG2 has a relatively short period which is easily cycled during some simulations. Long simulations are relatively rare and the looping pattern has yet to be observed in existing models, so this has not been a priority fix. However, if each object has it's own state, then the emergence of the loop pattern could easily become obvious. 

2) RNG2 has some strong internal correlations that can affect output noticeably. This is similar to cycling behavior in that it could cause short term strongly correlated sequences to emerge for a single object if each object has its own state. 

3) RNG2 does not create the same sequence from a given seed on different platforms. This has been a source of frustration for autotest, which depends on repeatable across platforms. The current fix is to have separate test assert objects for each platform. 

Multithreading has created a new problem that is a serious setback for the existing implementation: 

4) When multiple threads are being used for synchronization events, a given seed does not create the same sequence for two consecutive runs even on the same platform. This happens because the order in which the random numbers are generated cannot be controlled for objects that are processed in parallel. Most users consider this behavior unacceptable because it prevents repeatability when desired and cannot mitigated without forcing single-threaded operation, which can significantly increase time-to-solution. In effect fast and deterministic are mutually exclusive with RNG2. 

Overall, we conclude that the current implementation of RNG2 in random.c is obsolete and needs to be deprecated in favor of a new implementation that overcomes these problems. 

# Requirements

This section identifies the performance requirements for the random number generator. 

## Periodicity

**(R1)RNG3 shall have a period not less than 2^128.**

The requirement stems from the fact that a single object could use the RNG3 up to 10 times per simulation second. If a simulation of 100,000 objects lasts 10 years, this would correspond to about 2^48 random numbers. Rounding up to the next power of power of 2 is 2^64, and allowing random initial start points with plenty of margin gives 2^128. 

## Determinism

**(R2)RNG3 shall have separate determinism for each object or pseudo-object that is parallelized.**

This is necessary to ensure that sequence determinism is preserved regardless of the sequence in which objects call the RNG3 functions. 

## Repeatability

**(R3)RNG3 shall generate the same sequence for a given seed on all platforms**. 

This must be true regardless of the native integer size and the compiler family being used. 

## Entropy

**(R4)RNG3 shall satisfy the source purity, information entropy, monobit frequency, block frequency, and run statistical tests.**

The specific metrics for passing these tests have yet to be determined but they are necessary to ensure that the sequences do indeed represent random sequences. 

## Compatibility

**(R5) A global flag to restore RNG2 operation in version three shall be provided.**

This will allow models to behave like they do in version 2 when operating single threaded. The mode cannot work when operating multithreaded because of the limitation of RNG2 and users should be warned when RNG2 is used simultaneously with multithreading. 

# Design Considerations

The implementation of RNG3 will require that some new design issues be considered. 

## State

Each object or pseudo object that uses RNG3 in parallel must keep it's own state variable so that when it requests a random number, the generator can produce the next number in the sequence for that object. 

## Locking

This also means that calls to request a random number must always provide the state context, because the random number generator no longer has a global internal state. Consequently, the random number generator no longer requires locking (which is a performance issue when it is used alot). 

## Initialization

The initial state of each sequence must itself be randomized using a global seed. The initialization process for anything that uses random number must not be itself parallel because the sequence of initial states must be deterministic to preserve the overall determinism of the simulation. 

# Implementation

Depending on the version of GridLAB-D, the following limitation exist. 

## R2678 (3 July 2011 - Navajo (trunk))

RNG3 has a theoretical period of 2^31, or about 2.15 billion interations. Based on this, the worst case scenario above (10 random numbers per simulation second per object) would result is cycling of each object's state about every 8 months of simulation time. This isn't bad, but the search is on for an implementation of RNG3 with a much longer period. 

## R2681 (4 July 2011 - Navajo (trunk))

A direct test of the period of R2678 shows the implementation using the Park constants has a period of about 2^5, which is way too low. The constants for the Cray RANF LCG were used instead and the period is shown to be about 2^30. Further improvements can only be made if the state is 64 bits or more. Such a change is probably unnecessary at this point. 

It should be noted that the seed must be coprime to N (which is in this case 44485709377909=97*1091*420362567). This means multiples of any of the three factors should not be permitted as seeds for RNG3. Accordingly, there should be a test that prevents the use of such seeds. A randseed() function is needed to implement proper seeding of RNG3. 


  
