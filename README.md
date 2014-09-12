memory_allocation
=================

## Overview 
I implement malloc, free, and relloc function. It is based on double pointer free list implementation. 

### Data structure
+ Each node in list has block size and two entries to indicate previous block and next block. 
+ Allocated block only has an indicator for memory size , and a footer for pointing to the head of block  

### Malloc 
+ Set size to size of two entries if it is smaller than that
+ Choose block that fit size from free list(First fit)
+ Cut chosen block to smaller blocks if utilization is low  

