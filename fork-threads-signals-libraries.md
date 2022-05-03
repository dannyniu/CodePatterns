Title: The Unix Complication of Fork, Threads, Signals, and 3rd-party Libraries.

Author: Jianfang "Danny" Niu (dannyniu <at> hotmail <dot> com)

Abstract:

This thesis attempts to discuss the issues with 3 Unix/POSIX features, and
hoping that an understanding of these issues can help programmers structure
their programs to be more efficient, modular, and error-free. The 3 features
discussed are: 

1. process creation through `fork(1)`
2. multi-threading
3. signals

At the end of this thesis, an additional complication factor - 3rd-party 
libraries - are included as part of the discussion.

`fork(2)`
=========

Forking is the established mean of process creation in Unix. It works by
replicating the address space of the calling process, and return an indication
to the program code as to whether it's now executing in the parent or the child
process. Shells use this system call extensively, to preserve script-defined
objects such as functions and non-exported variables. Most other programs
do little after fork and loads a new program very soon.

A problem with fork is that, file dscriptors are shared between parent and
child processes. If the file descriptors are not closed, resource may slowly
run out, and compromised child processes may become a thread to the security of
the application. This is why the `FD_CLOEXEC` flag is defined so that
file descriptors with this flag set can be automatically closed by the system
during an `exec`. In the next revision (Issue 8), it is expected a few more
functions will be added to allow the `FD_CLOEXEC` flag be set atomically
at their creation. These include: `dup3(2)`, `accept4(2)`, `pipe2(2)`.

Threads
=======

Threads are flows of execution that share the same memory address space. With
threads, there is the problem of inter-thread coordination - a large majority
of computation has to be carried out in several steps, which can be
unexpectedly interrupted if threads didn't coordinate with each other.
There exists various synchronization primitives such as mutexes,
condition variables, as well as some defined in newer version of the standard
such as barriers, and reader-writer locks.

A problem with threads with regard to `fork(2)` is that, resources existed
in the parent continue to *appear to* exist in the child but such apparent
existence does not imply its integrity or functionality. For example, the
program is a client to a server, and a state is maintained in the client;
after the fork, the state is duplicated, but if the child update this state
by interacting with the server, the parent process will probably break.
The X11 windowing server is a prominent example of this.

POSIX Threads API came with the `pthread_atfork(3)` function. The original
intent of this interface is that, callbacks supplied to this function are
called based on the order of their registeration, to acquire locks in the
parent before the fork, and to release them after the fork in the parent
and child.

The function was designed to (according to the rationale section):

> provide multi-threaded libraries with a mean to protect themselves from
> innocent application programs that call `fork(2)`, and to provide
> multi-threaded application programs with a standard mechanism for
> protecting themselves from `fork(2)` calls in a library routine or
> the application itself.

But various shortcomings have deminished its usage. The future directions
for this interface in Issue 7 has projected that it be deprecated.

IMHO, such protection, along with several other goals, should be achieved
through the structuring of the application program as well as library codes.

Signals
=======

Usage
-----

For most user programs, there's no need to concern for signals, as the
default actions for these signals can integrate the program well into the
operating environment of the user. Signal handling is mostly tricky for
daemon programs, because they often need to make sure they exit in a way
such that their data files are consistent.

If we categorize the signals based on their reason of cause, we'll have:

- Program Fault
  - SIGBUS(A)
  - SIGFPE(A)
  - SIGILL(A)
  - SIGSEGV(A)
  - SIGSYS(A)
  - SIGTRAP(A)
  - SIGXCPU(A)
  - SIGXFSZ(A)

- User Interaction at Terminal
  - SIGINT(T)
  - SIGQUIT(A)
  - SIGTSTP(S)
  - SIGTTIN(S)
  - SIGTTOU(S)

- Notifications
  - SIGALRM(T)
  - SIGCHLD(I)
  - SIGHUP(T)
  - SIGCONT(C)
  - SIGPIPE(T)
  - SIGTERM(T)
  - SIGURG(I)
  - SIGVTALRM(T)

- Application
  - SIGABRT(A)
  - SIGUSR1(T)
  - SIGUSR2(T)

- Forcible Signals
  - SIGKILL(T)
  - SIGSTOP(S)

where we can see that: a) program faults and forcible signals cannot be handled
portably, or when handled, provide any robustness. b) user interaction at
terminal is best left as is, to not cause too much suprise for the user.
c) the most useful signals that can be handled, are those left in the
notifications and application category.

Generation and Disposition
--------------------------

POSIX specifies that, the signal disposition is the property of the (entire)
process, whereas signal masks are the properties of threads. It's a guess
of mine that, this is the compromise between:

1. existing multi-process semantic, where signals are sent between
   processes, rather than sent specifically to certain threads with
   a process.

2. existing practice with thread implementations, as well as their
   lack of commonality.

3. compatibility with the POSIX Realtime extensions.

In Linux, there's the `clone(2)` system call where the sharing of signal
disposition and related facilities can be customized. But the standard
developers obviously don't yet see a need to include such capability in
the standard, as it doesn't appear to achieve something that cannot be
achieved with existing standard facilities.

The traditional way of catching signals, is through registering a callback
that will be invoked when a signal is received. Most signals can be blocked
with the use of signal masks. These closely resemble the underlying hardware
where an interrupt vector is supplied to the interrupt controller, and some
hardware traps can be masked in hardware-specific ways.

Another way of receiving signals is `sigwait(2)` functions from the
POSIX Realtime extensions. The rationale from the standard says:

> Existing programming practice on realtime systems uses the ability to
> pause waiting for a selected set of events and handle the first event that
> occurs in-line instead of in a signal-handling function. This allows
> applications to be written in an event-directed style similar to a
> state machine. This style of programming is useful for largescale
> transaction processing in which the overall throughput of an application
> and the ability to clearly track states are more important than
> the ability to minimize the response time of individual event handling.

The traditional `kill(2)` and `killpg(2)` system calls are used for
generate signals for processes and group of processes. With threads, a new
`pthread_kill(3)` is introduced. Other means of generating signals include
asynchronous I/O notifications, `sigqueue(2)`. Many are too from the
POSIX Realtime extensions.

Async-Signal Safety
-------------------

Async-signal safety is a confusing concept used both within the context of
signal handling function, as well as `fork(2)` in multi-threaded applications.
But it's nontheless easy to comprehend.

Async-signal safe functions is a set of functions prescribed by the standard
as being able to be called without restriction from signal-catching functions.

The words "can be called from signal-catching function without restriction"
does not explain how this list is prescribed, nor does it describe why they're
safe to call after `fork(2)` and before `exec`, or in at-fork handlers.

In <https://man7.org/linux/man-pages/man7/signal-safety.7.html> an example
based on the `stdio` library is given to illustrate the problem with unsafe
functions. In essence, an `stdio` function that's updating a `FILE*` stream
may be interrupted in such way that, leaves the state of the stream
inconsistent and unusable, in such way that the behavior of a subsequent call
on this stream will be undefined.

To see how this differs from thread safety:

1. the "unsafe factors" within thread safety a) exists due to concurrent access
   to shared resources, and b) can be resolved through the use of
   synchronization objects such as mutexes.

2. the "unsafe factors" within async-signal safety a) exists due to abrupt
   interruption during certain operations that alters some shared resource
   and b) cannot be resolved cleanly because the active codes (whether it's
   the signal-catching function, or post-fork setup routines, etc.) cannot
   reliably recover the inconsistent resource states.

Finally, the standard goes on to say this in the rationales volume:

> The behavior of unsafe functions, as defined by this section, is undefined
> when they are called from (or after a longjmp() or siglongjmp() out of)
> signal-catching functions in certain circumstances. The behavior of
> async-signal-safe functions, as defined by this section, is as specified
> by POSIX.1, regardless of invocation from a signal-catching function.
> This is the only intended meaning of the statement that async-signal-safe
> functions may be used in signal-catching functions without restriction.

Exception Models in Higher-Level Languages
------------------------------------------

Many higher-level languages support object-oriented style exceptions. C++
is one of the few that compiles directly to machine code and executes off
the CPU. Java, C#, and many others compile to intermediate codes are run by
a virtual machine. Perl, PHP, Python, are yet another group that're executed
by an interpreter. For these kind of languages, interaction between signal
and their run-time are more involved and complicated than that with C, and
require foundational support from their run-time, VM, and interpreter.

3rd-party Libraries
===================

Third-party libraries are the straw that breaks the camel's back in this 
situation, because they all have valid reasons to create processes, threads,
and alter (or on behalf of itself, request the application to alter) the
disposition and masking of signals. It's best if the libraries are structured
in such way that they can flexibly adapt to applications of varying scale.

For example, a library that has library-wide states may use either use
internal mutexes to protect those states, or accept a mutex as argument
in any API routines that accesses the said states. Such mutex may be optional
so that the library can be used in both single-threaded and multi-threaded
applications. For example, if the mutex parameter is passed `NULL`, then
the locking operation is skipped by the library routine with the assumption 
that the caller is a single-threaded program.

A library may create threads, processes. For

- Libraries that Create Threads

In the most likely cases, the library may be managing threads transparently
to the application. The library interfaces may dispatch work autonomously.
Input to and output from the library will most likely be dispatched in an
asynchronously-in-synchronously-out fashion, so as to maximize the utility
of multiple threads. In this case APIs that receives application input return
immediately and without blocking; APIs that provides output back to the
application may block - until ready or until a timeout; there may also be
some APIs for polling the status of the work done within the library.

It is important, that data-consistency-critical applications (such as 
databases) can utilize some kind of finalization function to ensure 
the said consistency. Calls to these finalization APIs may be made in
`atexit` handlers, signal handlers for some termination signals, etc.

- Libraries that Create Processes

Creating processes is a natural way of workload outsourcing and
compartmentalization. Process management is more involved and complex than
threads management. Process creation in a library is very likely transparent
to the application, unless the PID of the child processes are made available
to the application.

The main program communicates with the satellite programs using some kind of
protocol (e.g. RPC - remote procedure call).

Because the application (or a different library used by the library) may have
configured different disposition for SIGCHLD, it is recommended that the
exit status not be used for communicating failures between the library and the
satellite process, and that such indication be communicated in-band within
the protocol.

Finally, on:

- Libraries and Signal Disposition and Masking

It's perfectly normal for library routines to temporarily alter the disposition
and masking of certain signals, and then restore them upon returning. Such 
change may also happen between some initialization and finalization functions.
The usual async-signal safety 'heads-ups' apply here.

While researching for this subsection, there doesn't seem to be a lot of
libraries that have specific requirements for signal setups, most of the
results turned up are about handling signals in application. 
