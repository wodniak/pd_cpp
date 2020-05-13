Multithreading - Sharing data between threads

Glossary:
- invariants = statements that are always true about a particular data structure (eg. this variable contains number of items in the list). Invariants are often broken during an update (writing).
- race condition = operation which outcome depends on the relative ordering of execution of operations. (threads race to perform their respective operations). 
We can destinguish 
    - benign race conditions - does not cause bugs. Happens when all possible outcomes are acceptable even though they may change with different relative orderings eg. two threads are adding items to a queue for processing.
    - problematic race conditions - concurrent modification to a single object which leads to broken invariants. They are often timing-sensitive eg. they can disappear when the application is run under the debugger / different system.


Avoiding race conditions
- lock-programming - Wrap data structure with a protection mechanism to ensure that only the thread performing a modification can see the intermediate states where the invariants are broken. 
- lock-free programming - design where modifications are done as a series of indivisible changes, each of which preserves the invariants. (nuances of memory model are mind blowing)
- Software transactional memory (STM) - handling updates to the data structure as transactions (similar to databases).


Protecting shared data
- mutex (mutual exclusion) - synchronization primitive used to force ordering. Before accessing a shared data structure, you lock the mutex associated with that data, and when you have finished accessing the data structure, you unlock the mutex. This ensures that all threads see a self-consistent view of the shared data, without any broken invariants.
Biggest problems of mutuxes are:
* Passing out a reference/pointer to protected data (example in mutex_problem_1.cpp)
    Solution : Do not pass pointers and references to protected data outside the scope of the lock, whethehr by returning them from a function, storing them in externally visible memory, or passing them as arguments to user-supplied functions.
* Deadlocks (example in mutex_problem_2.cpp)
    Solution: 
* Protecting either too much or too little data 
* Race conditions inherent to interfaces (example in stack_interface.cpp)
    

