/*
 * cc --std=c89 -o kqueueserver.o socktest.o kqueueserver.c && ./kqueueserver.o
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if HAVE_KQUEUE
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/event.h>
#include <sys/socket.h>
/* #include <sys/types.h> */
#include <unistd.h>
#include "socktest.h"

/*
 * https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/kqueue.2.html
 * https://wiki.netbsd.org/tutorials/kqueue_tutorial/
 */

#define MAXLENGTH 1024
#define REALLOC_SIZE 8

struct kevent *malloc_kevent(int);
struct kevent *realloc_kevent(struct kevent *, int, int);
void handle_echo(int, int, struct kevent *);

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

    int kq, max_changeevent = REALLOC_SIZE;
    struct kevent *change_events;
    struct kevent events[1];

    change_events = malloc_kevent(max_changeevent);

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

    kq = kqueue();
    if (kq == -1)
    {
        perror("kevent error\n");
        exit(EXIT_FAILURE);
    }
    /*
     * kevent: <listenfd, EVFILT_READ>
     * EV_ADD: add event to kq
     * EV_DELETE: delete event from kq
     * ...
     */
    EV_SET(change_events, listenfd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
    /* EV_SET(change_events+1, listenfd, EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, NULL); */
    kevent(kq, change_events, 1, NULL, 0, NULL);

    for (;;)
    {
        nready = kevent(kq, NULL, 0, events, 1, NULL);
        if (nready < 0)
        {
            perror("kevent error\n");
            exit(EXIT_FAILURE);
        }

        for (i = 0; i < nready; i++)
        {
            if (events[i].ident == listenfd)
            {
                len = sizeof(cliaddr);
                connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);

                for (i = 1; i < max_changeevent; i++)
                {
                    if (change_events[i].udata == NULL)
                        goto connfd_set;
                }
                change_events = realloc_kevent(change_events, max_changeevent, max_changeevent + REALLOC_SIZE);
                max_changeevent += REALLOC_SIZE;

            connfd_set:
                EV_SET(change_events + i, connfd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, change_events + i);
                if (kevent(kq, change_events + i, 1, NULL, 0, NULL) < 0)
                {
                    perror("connfd kevent error\n");
                    close(connfd);
                }
            }
            else if (events[i].filter == EVFILT_READ)
            {
                handle_echo(events[i].ident, kq, &events[i]);
            }
            /*
            else if (events[i].filter == EVFILT_WRITE)
            {
            }
            */
        }
    }

    close(kq);
    free(change_events);
    close(listenfd);

    return 0;
}

struct kevent *malloc_kevent(int n)
{
    struct kevent *events;
    int i;
    events = (struct kevent *)malloc(sizeof(struct kevent) * n);
    for (i = 0; i < n; i++)
        (events + i)->udata = NULL;
    return events;
}

struct kevent *realloc_kevent(struct kevent *events, int n, int m)
{
    events = (struct kevent *)realloc(events, (sizeof(struct kevent) * m));
    if (n < m)
    {
        for (; n < m; n++)
            (events + n)->udata = NULL;
    }
    return events;
}

void handle_echo(int connfd, int kq, struct kevent *event)
{
    ssize_t n;
    char readbuf[MAXLENGTH];
    char buff[MAXLENGTH];
    char *s;
    struct kevent *client;

    client = event->udata;
    /* disconnect */
    if (event->flags & EV_EOF)
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
    EV_SET(client, connfd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    if (kevent(kq, client, 1, NULL, 0, NULL) < 0)
        perror("connfd kevent delete error\n");
    close(connfd);
}
#else
int main(int argc, char **argv)
{
    printf("Unsupported kqueue!\n");
}
#endif /* HAVE_KQUEUE */
