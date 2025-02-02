# Multithreading
## Glossary:
- `process vs thread` = https://www.geeksforgeeks.org/difference-between-process-and-thread/
- `hardware concurrency` = having multiple processors or processors with multiple cores on which tasks can be truly run in the same time.
- `task switching` = doing a bit of one task and then a bit of another and so on so that it appears that the tasks are happening concurrently.
- `context switching` = In order to do task switching, OS needs to perform a context switch every time it changes from one task to another. OS needs to cache CPU state and instruction pointer for the currently running task, and reload another task. CPU then have to load the memory for the instructions and data for the new task into the cache.
- `invariants` = statements that are always true about a particular data structure (eg. this variable contains number of items in the list). Invariants are often broken during an update (writing).
- `race condition` = operation which outcome depends on the relative ordering of execution of operations. (threads race to perform their respective operations).
We can destinguish
    - benign race conditions - does not cause bugs. Happens when all possible outcomes are acceptable even though they may change with different relative orderings eg. two threads are adding items to a queue for processing.
    - problematic race conditions - concurrent modification to a single object which leads to broken invariants. They are often timing-sensitive eg. they can disappear when the application is run under the debugger / different system.

## Sharing data between threads
### Avoiding race conditions
- `lock-programming` - Wrap data structure with a protection mechanism to ensure that only the thread performing a modification can see the intermediate states where the invariants are broken.
- `lock-free programming` - design where modifications are done as a series of indivisible changes, each of which preserves the invariants. (nuances of memory model are mind blowing)
- Software transactional memory (STM) - handling updates to the data structure as transactions (similar to databases).

### Protecting shared data
1. `mutex` (mutual exclusion) - synchronization primitive used to force ordering. Before accessing a shared data structure, you lock the mutex associated with that data, and when you have finished accessing the data structure, you unlock the mutex. This ensures that all threads see a self-consistent view of the shared data, without any broken invariants.

Biggest problems of mutuxes are:
* `Passing out a reference/pointer to protected data` (**example in mutex_problem_1.cpp**)
   >Solution : Do not pass pointers and references to protected data outside the scope of the lock, whethehr by returning them from a function, storing them in externally visible memory, or passing them as arguments to user-supplied functions.

* `Deadlocks` (example in mutex_problem_2.cpp)
Usual situation is when threads are arguing over locks on mutexes - each of a pair of threads needs to lock both of a pair of mutexes to perform some operation, but each thread has only one locked mutex and is waiting for the other.
Doesnt only occur with locks. You can create deadlock with two threads and no locks by having each thread call join() on the std::thead object for the other. In this case neither thread can make progress because its waiting for the other.
    Solution:
    - `Avoid nested locks` - dont acquire a lock if you already hold one. If you really need to than use std::lock to lock multiple mutexes at once without risk of a deadlock.
    - `Avoid calling user-supplied code while holding a lock` - it might want to acquire other lock. Sometimes it is unavoidable when writing generic code. (eg. stack - every operation on the parameter type is user-supplied code)
    - `Acquire locks in fixed order` - define consistent order of locking mutexes
    - `Use a lock hierarchy` - simple wrapper around mutex to assign to it some value.
    - `Exception safe locking` - make sure that mutex will get unlocked even when some operation throws (Use RAII-style lock management such as std::lock_guard, std::unique_lock etc.)

* `Protecting either too much or too little data`
The granularity of the lock is a term to describe the amount of data protected by a single mutex. A fine-grained lock protects small amount of data and a coarse-grained lock protects large amount of data.
It is important to choose a sufficiently coarse lock granularity for 2 reasons: ensure that the required data is protected and that a lock is held only for operations that require it.
Where possible acquire a lock only when accessing the shared data and do processing of data outside the lock. Dont do any time-consuming activities eg. I/O while holding a lock.

* `Race conditions inherent to interfaces` (**example in stack_interface.cpp**)

2. `Alternative methods`
* `Protecting shared data during initialization`.
Extreme but common case. Shared data needs protection only from concurrent access while its being initialized, but after that no explicit synchronization is required (only for read operations, data is read-only). Using a mutex in this case in unnecessary hit to performance.

Used when a shared resource is very expensive to create so we want to do it only when required eg. opening database connection, allocation of a lot of memory.
This behaviour is called lazy-initialization. Each operation that requires a resource first checks to see if it has been initialized.

* `Protecting rarely updated data structures with std::shared_mutex` (shared_lock while reading, lock_guard while writing to get exclusive access to the data)


## Synchronizing concurrent operations
### Waiting for an event
a) `Keep checking a flag in shared data` (protected with a mutex) and have the second thread set the flag when it completes a task. Continuosly checking that flag and acquiring mutex is a performance hit.

b) `Have the waiting thread sleep` for short periods between the checks eg. by using this_thread::sleep_for()

c) `Use condition variables`. Condition variable is associated with an event and one or more threads can wait for that condition to happen. When a thread determines that the condition is satisfied, it notifies the rest of the threads to wake them up and allow them to continue processing.
std::condition_variable::wait is an optimization over a busy wait (described in b)).
During a call to wait() a condition variable can check the supplied condition any number of times. It does it always with a locked mutex. When the waiting thread reacquires a mutex and checks the condition without direct notification from another thread than we call is spurious wake. Number of these wakes is indeterminate so dont use a fuction with side effects for the condition check.

### Waiting for one-off event
If a thread needs to wait for a specific one-off event, it obtains a `future` representing that event. Thread can periodically wait on the future for short periods of time to see if the event has occurred while performing some other tasks. Once the event has occurred and future task is 'ready', future cannot be reset.
Futures are used to communicate between threads but future object itself do not provide synchronized access so you need to use mutexes for serialization.
std::experimental::future (proposals for the new cpp standard versions) changes a little bit that behaviour but its a topic for other discussion.

a) `Returning values from background tasks` (**example in parallel_sum.cpp**)
You can use `std::async` to start an 'asynchronous task' if you dont need the result of it right away. Async returns future object which eventually will hold the return value of the function. Calling get() on the future blocks the thread untill the future is ready and returns a value.

note : Just as with std::thread, if the arguments are rvalues, the copies are created by moving the originals.
: This allows the use of move-only types (eg. unique_ptr) as both the function object and the arguments.
       Also use std::ref() if you want to pass something by reference. Even if function takes an argument as a reference it will have a reference to the  copied object to new thread stack.

Exceptions: If the function call invoked as part of std::async throws an exception, it is stored in the future in place of a stored  value, future becomes 'ready' and call to get() rethrows it.

>`std::future` handles all the synchronization necessary to transfer data from one thread to another, calls to the member functions of a particular std::future instance are not synchronized with each other. If you access a single future object from multiple threads without additional synchronization, you get undifined behaviour. Future models unique ownership of the asynchronous result, and the one-shot nature of get() makes such concurrent access paintless. (After you call get() there is no value left to retrieve from future)

b) `Passing tasks between threads` (**example in futures.cpp**)
You can wrap any callable into a std::packaged_task to keep clean interface when passing them around.
Eg. GUI frameworks require that updated to the GUI are done from specific threads. So if another thread needs to update it, it must send a message to the right updater thread. With std::packaged_task executing thread doesnt require a custom message for each GUI-related activity.

c) `promises`
std::promise<T> provides a means of setting a value that can later be read through an associated std::future. Waiting thread can block on the future, while thread providing the data can use the promise to set the associated data and make the future "ready".

d) `Waiting from multiple threads`
Limitation of std::future is that only one thread can wait for the result. If you need to wait for the same event from more than one thread, you need to use `std::shared_future` instead. Pass shared_future as a copy to each thread, so that later each thread can access its own local shared_future object. Accessing shared asynchronous state from multiple thread is safety only if each thread does it through its own shared_future.


e) TODO: Describe std::experimental (chains, barriers, latches, spinlocks)


## Concurrent code design
### Techniques for dividing work
1. `Sharing data before processing` (**example in parallel_sum.cpp**)
Allocate first N elements to one thread, then next N elements to next thread etc. No matter how data is divided, each thread process assigned elements seperately, without any communication with other threads until it has completed. Useful when data can be easily divided before processing.
Similar to `MPI - Message Passing Interface`. Frameworks where each task is split into a set of parallel tasks, worker threads run these tasks independently and the results are combined in a final "reduction" step.
<br/>

2. `Dividing data recursively` (**example in parallel_quick_sort.cpp**)
Sometimes you cannot know how to divide data. For example quick_sort algorithm runs recursively, because only after processing an item you know to which half they go in. With each level of recursion there are more calls to the quick_sort, because you have to sort both elements that belong before the pivot and after. These recursive calls are independent (they access seperate sets of elements).
<br/>

3. `Dividing work by task type`
Make threads specialist in certain type of task. Threads might work on the same data, but for different purposes.
This technique results from separating concerns with concurrency - each thread execute different task independently. Other threads can give it data or trigger events that it needs to handle. With that each piece of code has a single responsability.
<br/>

4. `Dividing sequence of tasks`
If your task consist of applying the same sequence of operations to many independent data items, use `pipeline`. Create a seperate thread for each stage of algorithm. When the operaton is completed by first stage thread, data is put in a queue to be picked by the next thread. This allows first thread to start processing next element while second thread works on first element. (video streaming - you wait at the beggining of video to fill all stage threads with data. After that each new element is processed smoothly, equally spaced in time)

### Performance hits

