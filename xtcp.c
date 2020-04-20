/* DannyNiu/NJF, 2020-02-24. All rights reserved. */

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <signal.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>

//
// Global states.

static char *bndaddr, *bndserv, *connaddr, *connserv;
static struct addrinfo *addrs_bnd, *addrs_conn;

//
// Handles error logging and addrinfo freeing.

int xtcp_getaddrinfo(
    char const  *restrict addr, char const *restrict serv,
    struct addrinfo const *restrict hints,
    struct addrinfo **res)
{
    *res = NULL;
    int ret = getaddrinfo(addr, serv, hints, res);

    if( ret )
    {
        fprintf(
            stderr, "getaddrinfo %s %s: %s\n",
            addr, serv,
            gai_strerror(ret));

        if( ret == EAI_SYSTEM )
            perror("getaddrinfo errno");

        freeaddrinfo(*res);
        *res = NULL;
        return -1;
    }
    else return 0;
}

//
// For convenience.

static struct {
    char host[256];
    char serv[256];
    struct sockaddr_storage addr;
    socklen_t len;
} addr_res;

void *xtcp_get_addr_res()
{
    memset(addr_res.host, 0, sizeof addr_res.host);
    memset(addr_res.serv, 0, sizeof addr_res.serv);
    memset(&addr_res.addr, 0, addr_res.len = sizeof addr_res.addr);
    return &addr_res.addr;
}

void xtcp_sockprint(const char *restrict msg, int fd)
{
    int ret;

    ret = getnameinfo(
        (const void *)&addr_res.addr, addr_res.len,
        addr_res.host, sizeof addr_res.host,
        addr_res.serv, sizeof addr_res.serv,
        NI_NUMERICHOST | NI_NUMERICSERV);

    if( ret )
    {
        fprintf(
            stderr, "getnameinfo: %s\n",
            gai_strerror(ret));

        if( ret == EAI_SYSTEM )
            perror("getnameinfo errno");
    }
    else
    {
        printf(
            "%s %11s %7s fd<%d>\n",
            msg, addr_res.host, addr_res.serv, fd);
    }
}

//
// The IO Loop.

#define CAPACITY 512
typedef struct {
    size_t len;
    uint8_t dat[CAPACITY];
} xtcp_buffer_t;

// #define POLLRECV (POLLIN | POLLRDNORM | POLLRDBAND | POLLPRI)
// #define POLLSEND (POLLOUT | POLLWRNORM | POLLWRBAND)

#define POLLRECV POLLIN
#define POLLSEND POLLOUT

ssize_t xtcp_recv(
    struct pollfd *restrict pfd, int pflags,
    xtcp_buffer_t *restrict buf, int ioflags)
{
    ssize_t ret = 1; // reserve 0<EOF> and -1<error> as special.
    size_t filled, remain;

    filled = buf->len;
    remain = CAPACITY - filled;

    if( pfd->revents & pflags && remain )
    {
        errno = 0;
        ret = recv(pfd->fd, buf->dat+filled, remain, ioflags);

        if( ret > 0 )
            buf->len += ret;

        else if( errno )
            fprintf(
                stderr, "pid<%ld> recv: %s\n",
                (long)getpid(), strerror(errno));
    }

    return ret;
}

ssize_t xtcp_send(
    struct pollfd *restrict pfd, int pflags,
    xtcp_buffer_t *restrict buf, int ioflags)
{
    ssize_t ret = 0;

    if( (pfd->revents & pflags || ioflags & MSG_OOB) && buf->len )
    {
        errno = 0;
        ret = send(pfd->fd, buf->dat, buf->len, ioflags);

        if( ret > 0 )
            memmove(buf->dat, buf->dat+ret, buf->len -= ret);

        else if( errno )
            fprintf(
                stderr, "pid<%ld> send: %s\n",
                (long)getpid(), strerror(errno));
    }

    return ret;
}

int xtcp_passover(int inbnd, int upstr)
{
    // Assume 2 ends so as to guarantee
    // xor-1 works as a simplified
    // binary modular operation.

    xtcp_buffer_t buffer[2] = { {0}, {0}, };
    struct pollfd fds[2] = {
        { .events = POLLRECV | POLLSEND, .fd = inbnd, },
        { .events = POLLRECV | POLLSEND, .fd = upstr, },
    };
    int i, j;

    // Per RFC 6093.
    i = 1;

    if( setsockopt(inbnd, SOL_SOCKET, SO_OOBINLINE, &i, sizeof i) == -1 )
        fprintf(
            stderr, "pid<%ld> setsockopt inbnd: %s\n",
            (long)getpid(), strerror(errno));

    if( setsockopt(upstr, SOL_SOCKET, SO_OOBINLINE, &i, sizeof i) == -1 )
        fprintf(
            stderr, "pid<%ld> setsockopt upstr: %s\n",
            (long)getpid(), strerror(errno));

    while( poll(fds, 2, -1) >= 0 )
    {
        for(i=0; i<2; i++)
        {
            if( fds[i].revents & (POLLERR | POLLHUP | POLLNVAL) )
            {
                // exception at at least 1 end,
                // propagate it to both ends.
                close(fds[0].fd);
                close(fds[1].fd);
                return 0;
            }

            j = i^1; // (i + 1) % 2;

            xtcp_recv(fds+i, POLLRECV, buffer+i, 0);
            xtcp_send(fds+j, POLLSEND, buffer+i, 0);

            if( buffer[i].len == CAPACITY )
                fds[i].events &= ~POLLRECV;
            else fds[i].events |= POLLRECV;

            if( !buffer[i].len )
                fds[j].events &= ~POLLSEND;
            else fds[j].events |= POLLSEND;
        }
    }

    return -1; // poll returned with failure.
}

//
// Child process main subroutine.

noreturn void xtcp_connect(int fd_inbnd)
{
    int ret;
    struct addrinfo *addr;

    for(addr = addrs_conn; addr; addr = addr->ai_next)
    {
        int fd = -1;

        if( addr->ai_family != AF_INET &&
            addr->ai_family != AF_INET6 )
            continue;

        fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if( fd == -1 )
        {
            fprintf(
                stderr, "pid<%ld> xtcp_connect socket: %s",
                (long)getpid(), strerror(errno));
            continue;
        }

        ret = connect(fd, addr->ai_addr, addr->ai_addrlen);
        if( ret == -1 )
        {
            // silent.
            close(fd);
            fd = -1;
            continue;
        }

        ret = xtcp_passover(fd_inbnd, fd); // break;
        printf(
            "pid<%ld> xtcp_passover returned %d\n",
            (long)getpid(), ret);

        if( ret == -1 )
            exit(EXIT_FAILURE);
        else exit(EXIT_SUCCESS);
    }

    printf("no connectable peer\n");
    exit(EXIT_FAILURE);
}

//
// Main process event loop.

int xtcp_listen()
{
    int
        fd_listen = -1,
        fd_listen6 = -1,
        ret;

    struct addrinfo *addr;

    struct pollfd fds[2];
    int nfds = 0;

    for(addr = addrs_bnd; addr; addr = addr->ai_next)
    {
        int fd = -1;

        if( addr->ai_family != AF_INET &&
            addr->ai_family != AF_INET6 )
            continue; // skipping non-Internet sockets.

        if( (addr->ai_family == AF_INET ? fd_listen : fd_listen6) >= 0 )
            continue; // avoid opening redundent listening sockets.

        fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if( fd == -1 )
        {
            perror("xtcp_listen: socket");
            continue;
        }

        ret = bind(fd, addr->ai_addr, addr->ai_addrlen);
        if( ret == -1 )
        {
            perror("bind");
            close(fd);
            fd = -1;
            continue;
        }

        ret = getsockname(fd, xtcp_get_addr_res(), &addr_res.len);
        if( ret == -1 )
        {
            perror("xtcp_listen: getsockname");
            // pass;
        }
        else xtcp_sockprint("bound to", fd);

        ret = listen(fd, 0);
        if( ret == -1 )
        {
            perror("listen");
            close(fd);
            fd = -1;
            continue;
        }

        *( addr->ai_family == AF_INET ? &fd_listen : &fd_listen6) = fd;
    }

    if( fd_listen >= 0 )
    {
        fds[nfds].fd = fd_listen;
        fds[nfds].events = POLLIN;
        nfds++;
    }

    if( fd_listen6 >= 0 )
    {
        fds[nfds].fd = fd_listen6;
        fds[nfds].events = POLLIN;
        nfds++;
    }

    while( poll(fds, nfds, -1) >= 0 )
    {
        int i, j, fd;
        int x = 0;
        pid_t pid;

        for(i=0; i<nfds; i++)
        {
            if( !(fds[i].revents & POLLIN) ) continue;
            x++;

            fd = accept(fds[i].fd, xtcp_get_addr_res(), &addr_res.len);
            if( fd == -1 )
            {
                perror("accept");
                continue;
            }
            else xtcp_sockprint("accepted", fd);

            pid = fork();
            if( pid == -1 )
            {
                perror("fork");
                return -1;
            }
            else if( pid == 0 )
            {
                for(j=0; j<nfds; j++) close(fds[j].fd);

                (void)xtcp_connect(fd); // has _Noreturn specifier.
                exit(EXIT_SUCCESS); // break;
            }
            else
            {
                printf("forked %ld\n", (long)pid);
                close(fd);
                fd = -1;
                break;
            }
        }

        if( !x ) break;
    }

    return -1;
}

//
// Entry point of the program,
// sets up signal handling and resolves network addresses.

int main(int argc, char *argv[])
{
    struct addrinfo hints;
    int ret;

    struct sigaction sigact = { .sa_handler = SIG_IGN, };
    sigaction(SIGCHLD, &sigact, NULL);

    printf("main pid: %ld\n", (long)getpid());

    if( argc > 5 )
    {
        printf("Ignoring unneeded argument(s).\n");
        argc = 5;
    }
    
    if( argc == 5 )
    {
        bndaddr = argv[1];
        argc--;
        argv++;
    }

    if( argc == 4 )
    {
        bndserv = argv[1];
        argc--;
        argv++;
    }

    if( argc == 3 )
    {
        connaddr = argv[1];
        connserv = argv[2];
    }

    if( argc < 3 )
    {
        printf("Not enough arguments, exiting.\n");
        return EXIT_FAILURE;
    }

    hints = (struct addrinfo){0};
    hints.ai_flags = AI_ALL;
    hints.ai_socktype = SOCK_STREAM;

    ret = xtcp_getaddrinfo(connaddr, connserv, &hints, &addrs_conn);
    if( ret == -1 )
        return EXIT_FAILURE;

    hints = (struct addrinfo){0};
    hints.ai_flags = AI_PASSIVE | AI_ALL;
    hints.ai_socktype = SOCK_STREAM;

    ret = xtcp_getaddrinfo(bndaddr, bndserv, &hints, &addrs_bnd);
    if( ret == -1 )
    {
        freeaddrinfo(addrs_conn);
        addrs_conn = NULL;
        return EXIT_FAILURE;
    }

    ret = xtcp_listen();
    printf("main xtcp_listen returned %d\n", ret);

    if( ret == -1 )
        return EXIT_FAILURE;
    else return EXIT_SUCCESS;
}
