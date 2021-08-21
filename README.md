# CodePatterns
Exemplified Programming Interface Usages.

This project holds complete code samples for demonstrating
archetypal usage of various application programming interfaces
and common code patterns in those languages.

# Files and Demonstrations

1. xtcp.c 

1.1. host and service name resolution using `getaddrinfo`, 

1.2. syscall usages for `socket`, `bind`, `listen`, `connect`, etc.

1.3. IO multiplexing with SysV `poll`. 
(See [this](https://beesbuzz.biz/code/5739-The-problem-with-select-vs-poll) for an argument against `select`)

- Build Command: `make CFLAGS="-Wall -Wextra" xtcp`

- Usage: `xtcp [[bind-address] bind-port] connect-address connect-port` (addresses and ports can be host and service names)

2. intconv.md

- A discussion of C language integer types of my own.

3. programming-foundations.md

- Making sense of how the current forms of programming languages came about.
