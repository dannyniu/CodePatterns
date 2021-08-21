> Once there's Turing-complete computing, everything else is just
> program and code optimization.
>
> DannyNiu/NJF

Formation - Collecting Puzzle Pieces
====================================

In the beginning, there was 
- bits. 

We needed integers - then we had
- unsigned integers, 
- signed magnitudes,
- one's complement, and
- two's complement.

We needed a character encoding set - then we had 
- ASCII, 
- EBCDIC.

We theorized a model of computing - then we had
- Turing (universal) machine.

We needed a way to specify computing languages - then we had
- Backus-Naur Form based on 
- Chomsky's hierarchy of language grammars.

We needed numbers - we had 
- fixed-point and 
- floating-point representations;

we needed them to be interoperable, we forgot fixed-point and standardized
- IEEE-754-1985.

Development - Consolidation
===========================

We needed a programming environment - then we had
- POSIX (based on Unix),
- etc.

We wanted networking - then we had
- the TCP/IP stack (and its BSD Socket implementation).

We wanted parallel programming - then we had
- threads, 
- locks (mutex), 
- monitors (condition variables).

We wanted computing to be multi-lingual - then we had
- The Unicode Standard, which includes the essential:
- The Universal Character Set,
- The Unicode Transformation Formats, UTF-8, UTF-16, etc.

More Versatile Programming Paradigms on top of Turing Complete Computing
------------------------------------------------------------------------

Improving upon the completeness of Turing (universal) machine, we had:
- Class-Based Object-Oriented Programming
- Template
- Iterator/Generator
- Co-routine
- Lambda Expression
- Functional Programming

Advancement - Active Research
=============================

We want numerical calculations to be reproducible - then we had
- IEEE-754-2008, and later 
- IEEE-754-2019

We wanted faster memory - then we had
- cache, and later multi-level cache.

We wanted multi-processing - then we had
- dual/quad/octa-core processors,
- GPGPU
- NUMA and other cache architecture.

We envisioned general-purpose lock-free programming - then we had
- atomic operations,

And to make atomic operations efficient on cache architectures, we specified
- explicit memory order for atomic operations,
- explicit non-operation fences with memory order semantics.
