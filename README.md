# CodePatterns
Exemplified Programming Interface Usages.

This project holds complete code samples for demonstrating
archetypal usage of various application programming interfaces
and common code patterns in those languages.

# Files and Demonstrations

[xtcp.c](xtcp.c)
----

1. host and service name resolution using `getaddrinfo`, 

2. syscall usages for `socket`, `bind`, `listen`, `connect`, etc.

3. IO multiplexing with SysV `poll`. 
(See [this](https://beesbuzz.biz/code/5739-The-problem-with-select-vs-poll) for an argument against `select`)

- Build Command: `make CFLAGS="-Wall -Wextra" xtcp`

- Usage: `xtcp [[bind-address] bind-port] connect-address connect-port` (addresses and ports can be host and service names)

[IEEE-754-2019 Summary in Spreadsheet Format](2024-04-08%20IEEE-754%20Summary.xlsx)
----

- The newer IEEE-754 standards are more than just FP data formats. Here I've made a outline summary of the 2019 edition of the standard as a publicly available reference.

[intconv.md](intconv.md)
----

- A discussion of C language integer types of my own.

[AsyncAwaitSince1970.md](AsyncAwaitSince1970.md)
----

- Concurrent programming with async/await is nothing new under the sun.

[programming-foundations.md](programming-foundations.md)
----

- Making sense of how the current forms of programming languages came about.

[fork-threads-signals-libraries.md](fork-threads-signals-libraries.md)
----

A discussion about some of the most complicated concepts with Unix/POSIX.
