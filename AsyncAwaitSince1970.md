Async/Await Are There All The Way Since The *Epoch*
===================================================

Since roughly around 2000~2010, modern high-level programming languages are
beginning to embrace the idea of what can be summarized as "concurrent 
programming". This include 

- a revisit of coroutine techniques use to be found prodominently in assembly 
  languages,
  
- wider adoption of the iterator/generator concept,

- and more recently, async/await concepts and the said 2 keywords in
  programming languages.

In this piece, I voice my opinion to argue that these things, although novel,
are nonetheless re-iteration of conventional wisdom found in older synchronous
"blocking" programming.

The Unix Philosophy
===================

The Unix philosophy tell us that - write small programs work well together,
rather than monoliths that are inefficient and difficult to maintain. 
This is examplified in the "pipeline" feature where the output of 1 program
is fed to the input of another to form a chain that achieve complex results.

The `pipe(2)` and `mkfifo(2)` system calls creates this output-input bridge;
the programs use `read(2)` and `write(2)` system calls to pass data between
them, either directly, or indirectly through the C library. Here, `read(2)`
and `write(2)` are the **yield** points in the "coroutine" that is the entire
system:

- The entire operating system can be seen as a big application program,
- The kernel is the manager of the coroutines that are the processes,
- The `read(2)` and `write(2)` calls yield and resume the control flow
  of the processes.

Feature Points of Iterators/Generators and Coroutines
=====================================================

Iterators and generators may have started as helping components of "loops",
yet, they've certainly grown beyond that.

By their very nature, iterators/generators require separate stacks to function.
Acquiring a separate stack is a implementation-dependent task. POSIX provided
ways to create "contexts", which allow for several subroutines running in the
same thread to use separate stack, but that feature has been considered
obsolete and was removed in the 2008 revision of the standard.

A quick note on coroutine: people don't quite agree on what it is and what
qualifies as one. Certainly the coroutine techniques in assembly programming
is wildly different from one that would be found in a high-level programming
language; likewise, one library's coroutine implementation is likely 
incompatible with another's.

Another thing that needed separate stacks are *threads*. Multi-threading is
typically used in high-performance applications, to allow for parallel flows
of control. However, multi-thread programming is a delicate task, and one
need years of experience before they can call themselves experts. But you know
what doesn't require that level of profficiency:

Concurrent Programming With and Without "Async/Await"
=====================================================

Let's say we want to download and locally save some web pages, this is probably
what you'd do in modern JavaScript:

```javascript
async function getpages()
{
  let req1 = fetch("example.com");
  let req2 = fetch("example.org");
  let req3 = fetch("example.net");
  return [ await req1, await req2, await req3 ];
}
```

But you know how to do that without "async/await"?

```bash
getpages()
{
  curl example.com & pid1=$!
  curl example.org & pid2=$!
  curl example.net & pid3=$!
  wait $pid1 $pid2 $pid3
}
```

Conclusion
==========

So did we just re-invent the wheel? My comment on this is:

> 1. Async/Await features of a programming language lets programmer partition
>    their program more logically, and most imprtantly - more safely, in order
>.   to achieve better performance.
>
> 2. Multi-processing is still a good way to minimize the complexity of code
>    that handles the logic of separate tasks, and also to isolate and 
>    compartmentalize errors.
