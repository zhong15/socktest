/*
 * cc --std=c89 -o selectserver.o socktest.o selectserver.c && ./selectserver.o
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
/* #include <sys/types.h> */
#include <unistd.h>

#include "socktest.h"

#define MAXLENGTH 1024

void handle_echo(int, int *, fd_set *);

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;
    char *host = "127.0.0.1";
    int port = 8081;
    int LISTENQ = 10;
    int error;

    int i, maxi, maxfd, sockfd;
    int nready;
    int client[FD_SETSIZE];
    fd_set rset, allset;

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

    maxfd = listenfd;
    maxi = -1;
    for (i = 0; i < FD_SETSIZE; i++)
        client[i] = -1;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    for (;;)
    {
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &rset))
        {
            /* now nready > 0 */
            len = sizeof(cliaddr);
            connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);

            for (i = 0; i < FD_SETSIZE; i++)
            {
                if (client[i] < 0)
                {
                    /* client[i] is idle */
                    client[i] = connfd;
                    break;
                }
            }
            if (i == FD_SETSIZE)
            {
                printf("too many clients");
                exit(0);
            }

            /*
             * 1. add new conn to allset
             * 2. waiting for the next loop
             */
            FD_SET(connfd, &allset);
            if (connfd > maxfd)
                maxfd = connfd;
            if (i > maxi)
                maxi = i;

            /*
             * 1. nready == 1
             * 2. only listenfd readable
             */
            if (--nready <= 0)
                continue;
        }

        for (i = 0; i <= maxi; i++)
        {
            if ((sockfd = client[i]) < 0)
                continue;
            if (FD_ISSET(sockfd, &rset))
            {
                handle_echo(sockfd, client + i, &allset);

                if (--nready <= 0)
                    break;
            }
        }
    }

    close(listenfd);

    return 0;
}

void handle_echo(int connfd, int *client, fd_set *allset)
{
    ssize_t n;
    char readbuf[MAXLENGTH];
    char buff[MAXLENGTH];
    char *s;

    if ((n = read(connfd, readbuf, sizeof(readbuf))) == 0)
    {
        close(connfd);
        FD_CLR(connfd, allset);
        *client = -1;
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
