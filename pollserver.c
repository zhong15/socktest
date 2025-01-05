/*
 * cc --std=c89 -o selectserver.o socktest.o selectserver.c && ./selectserver.o
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/poll.h>
#ifdef __linux__
/* Linux poll() timeout INFTIM = -1 */
#include <sys/stropts.h>
#endif /* __linux__ */
#include <sys/socket.h>
/* #include <sys/types.h> */
#include <unistd.h>
#include "socktest.h"

/* sys/stropts.h Linux poll() timeout INFTIM = -1 */
#ifdef __APPLE__
#ifndef INFTIM
#define INFTIM -1
#endif /* INFTIM */
#endif /* __APPLE__ */

#define MAXLENGTH 1024

void handle_echo(int, struct pollfd *);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;
    char *host = "127.0.0.1";
    int port = 8081;
    int LISTENQ = 10;
    int error;

    int i, maxi, sockfd;
    int nready;
    struct pollfd client[OPEN_MAX];

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
        exit(0);
    }

    error = listen(listenfd, LISTENQ);
    if (error)
    {
        close(listenfd);
        printf("listen error: %d\n", error);
        exit(0);
    }

    maxi = -1;
    client[0].fd = listenfd;
    client[0].events = POLLRDNORM;
    for (i = 1; i < OPEN_MAX; i++)
        client[i].fd = -1;
    maxi = 0;

    for (;;)
    {
        /* now nready > 0 */
        nready = poll(client, maxi + 1, INFTIM);

        if (client[0].revents & POLLRDNORM)
        {
            len = sizeof(cliaddr);
            connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);

            for (i = 1; i < OPEN_MAX; i++)
            {
                if (client[i].fd < 0)
                {
                    /* client[i] is idle */
                    client[i].fd = connfd;
                    break;
                }
            }
            if (i == OPEN_MAX)
            {
                printf("too many clients");
                exit(0);
            }

            client[i].events = POLLRDNORM;
            if (i > maxi)
                maxi = i;

            /*
             * 1. nready == 1
             * 2. only listenfd readable
             */
            if (--nready <= 0)
                continue;
        }

        for (i = 1; i <= maxi; i++)
        {
            if ((sockfd = client[i].fd) < 0)
                continue;
            if (client[i].revents & (POLLRDNORM | POLLERR))
            {
                handle_echo(sockfd, &(client[i]));

                if (--nready <= 0)
                    break;
            }
        }
    }

    close(listenfd);

    return 0;
}

void handle_echo(int connfd, struct pollfd *client)
{
    ssize_t n;
    char readbuf[MAXLENGTH];
    char buff[MAXLENGTH];
    char *s;

    if ((n = read(connfd, readbuf, sizeof(readbuf))) < 0)
    {
        if (errno == ECONNRESET)
        {
            close(connfd);
            client->fd = -1;
        }
        else
        {
            printf("read error: %d\n", errno);
            exit(0);
        }
    }
    else if (n == 0)
    {
        close(connfd);
        client->fd = -1;
    }
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
}
