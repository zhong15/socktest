/*
 * cc --std=c89 -o epollserver.o socktest.o epollserver.c && ./epollserver.o
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if HAVE_EPOLL
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
/* #include <sys/types.h> */
#include <unistd.h>
#include "socktest.h"
#include <fcntl.h>

#define MAX_EVENTS 10
#define MAXLENGTH 1024

struct epoll_event *malloc_events(int);
struct epoll_event *realloc_events(struct epoll_event *, int, int);
void handle_echo(int, int, struct epoll_event *);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;
    char *host = "127.0.0.1";
    int port = 8081;
    int LISTENQ = 10;
    int error;

    int i;
    int nready;

    int epfd;
    struct epoll_event ev, events[MAX_EVENTS];

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(host);
    servaddr.sin_port = htons(port);

    error = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (error)
    {
        close(listenfd);
        printf("bind error: %d\n", error);
        exit(EXIT_FAILURE);
    }

    error = listen(listenfd, LISTENQ);
    if (error)
    {
        close(listenfd);
        printf("listen error: %d\n", error);
        exit(EXIT_FAILURE);
    }

    epfd = epoll_create(1);
    if (epfd == -1)
    {
        perror("epoll_create error\n");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = listenfd;
    error = epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
    if (error == -1)
    {
        perror("epoll_ctl error\n");
        exit(EXIT_FAILURE);
    }

    for (;;)
    {
        nready = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nready < 0) /* nready= -1 */
        {
            perror("epoll_wait error\n");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < nready; i++)
        {
            if (events[i].data.fd == listenfd)
            {
                len = sizeof(cliaddr);
                connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);

                /* LT */
                /* ev.events = EPOLLIN | EPOLLRDHUP; */
                /* ET */
                error = fcntl(connfd, F_GETFL, 0);
                if (error == -1)
                {
                    perror("fcntl error\n");
                    goto accept_error;
                }
                if (fcntl(connfd, F_SETFL, error | O_NONBLOCK) == -1)
                {
                    perror("connfd fcntl error\n");
                    goto accept_error;
                }
                ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                ev.data.fd = connfd;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev) < 0)
                {
                    perror("connfd epoll_ctl error\n");
                    goto accept_error;
                }

                continue;

            accept_error:
                close(connfd);
            }
            else if ((events[i].events & EPOLLIN) || (events[i].events & EPOLLRDHUP))
            {
                handle_echo(events[i].data.fd, epfd, &events[i]);
            }
            /*
            else if (events[i].events & EPOLLOUT)
            {
            }
            */
        }
    }

    close(epfd);
    close(listenfd);

    return 0;
}

struct epoll_event *malloc_events(int n)
{
    struct epoll_event *events;
    int i;
    events = (struct epoll_event *)malloc(sizeof(struct epoll_event) * n);
    for (i = 0; i < n; i++)
        (events + i)->data.fd = -1;
    return events;
}

struct epoll_event *realloc_events(struct epoll_event *events, int n, int m)
{
    events = (struct epoll_event *)realloc(events, (sizeof(struct epoll_event) * m));
    if (n < m)
    {
        for (; n < m; n++)
            (events + n)->data.fd = -1;
    }
    return events;
}

void handle_echo(int connfd, int epfd, struct epoll_event *client)
{
    ssize_t n;
    char readbuf[MAXLENGTH];
    char buff[MAXLENGTH];
    char *s;

    /* disconnect */
    if (client->events & EPOLLRDHUP)
        goto connfd_close;

    if ((n = read(connfd, readbuf, sizeof(readbuf))) < 0)
    {
        if (errno == ECONNRESET)
            goto connfd_close;
        else
        {
            printf("read error: %d\n", errno);
            exit(EXIT_FAILURE);
        }
    }
    else if (n == 0)
        goto connfd_close;
    else
    {
        readbuf[n] = '\0';

        s = gettimestr();
        buff[0] = '\0';
        strcat(buff, s);
        free(s);
        strcat(buff, " ");
        strcat(buff, readbuf);

        write(connfd, buff, strlen(buff));
        printf("child write: %s", buff);
    }

    return;

connfd_close:
    client->data.fd = -1;
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, connfd, NULL) < 0)
        perror("connfd epoll_ctl error\n");

    close(connfd);
}
#else
int main(int argc, char **argv)
{
    printf("Unsupported epoll!\n");
}
#endif /* HAVE_EPOLL */
