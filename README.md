# MyThreadLibrary

#Thread routines.

1. MyThread MyThreadCreate (void(*start_funct)(void *), void *args)
This routine creates a new MyThread. The parameter start_func is the function
in which the new thread starts executing. The parameter args is passed to the
start function. This routine does not pre‐empt the invoking thread. In others
words the parent (invoking) thread will continue to run; the child thread will sit
in the ready queue.

2. void MyThreadYield(void)
Suspends execution of invoking thread and yield to another thread. The invoking
thread remains ready to execute—it is not blocked. Thus, if there is no other
ready thread, the invoking thread will continue to execute.

3. int MyThreadJoin(MyThread thread)
Joins the invoking function with the specified child thread. If the child has
already terminated, do not block. Note: A child may have terminated without
the parent having joined with it. Returns 0 on success (after any necessary
blocking). It returns ‐1 on failure. Failure occurs if specified thread is not an
immediate child of invoking thread.

4. void MyThreadJoinAll(void)
Waits until all children have terminated. Returns immediately if there are no
active children.

5. void MyThreadExit(void)
Terminates the invoking thread. Note: all MyThreads are required to invoke this
function. Do not allow functions to “fall out” of the start function.

#Semaphore routines.

1. MySemaphore MySemaphoreInit(int initialValue)
Create a semaphore. Set the initial value to initialValue, which must be non‐
negative. A positive initial value has the same effect as invoking
MySemaphoreSignal the same number of times. On error it returns NULL.

2. void MySemaphoreSignal(MySemaphore sem)
Signal semaphore sem. The invoking thread is not pre‐empted.
void MySemaphoreWait(MySemaphore sem)
Wait on semaphore sem.

3. int MySemaphoreDestroy(MySemaphore sem)
Destroy semaphore sem. Do not destroy semaphore if any threads are blocked
on the queue. Return 0 on success, ‐1 on failure.

