/*
 * cc --std=c89 -o server.o socktest.o server.c && ./server.o
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
/* #include <sys/types.h> */
#include <unistd.h>

#include "socktest.h"

#define MAXLENGTH 1024

void handle_echo();

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cliaddr;
    char *host = "127.0.0.1";
    int port = 8081;
    int LISTENQ = 10;
    int error;
    pid_t childpid;

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

    for (;;)
    {
        len = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &len);

        if ((childpid = fork()) == 0)
        {
            handle_echo(listenfd, connfd);

            return 0;
        }

        close(connfd);
    }

    close(listenfd);

    return 0;
}

void handle_echo(int listenfd, int connfd)
{
    ssize_t n;
    char readbuf[MAXLENGTH];
    char buff[MAXLENGTH];

    close(listenfd);

    while ((n = read(connfd, readbuf, sizeof(readbuf))) > 0)
    {
        readbuf[n] = '\0';

        char *s = gettimestr();
        buff[0] = '\0';
        strcat(buff, s);
        free(s);
        strcat(buff, " ");
        strcat(buff, readbuf);

        write(connfd, buff, strlen(buff));
        printf("child write: %s", buff);
    }

    close(connfd);
}
