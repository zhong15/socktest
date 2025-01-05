/*
 * cc --std=c89 -o client.o client.c && ./client.o
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
/* #include <sys/types.h> */
#include <unistd.h>

#include "socktest.h"

#define MAXLENGTH 1024

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr;
    char sendbuf[MAXLENGTH];
    char recvbuf[MAXLENGTH];
    char *host = "127.0.0.1";
    int port = 8081;
    int error;
    ssize_t n;
    int maxfdp1, stdineof;
    fd_set rset;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, host, &servaddr.sin_addr);

    error = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (error)
    {
        close(sockfd);
        printf("connect error: %d\n", error);
        exit(0);
    }

    stdineof = 0;
    FD_ZERO(&rset);
    for (;;)
    {
        if (stdineof == 0)
            FD_SET(fileno(stdin), &rset);
        FD_SET(sockfd, &rset);
        maxfdp1 = max(fileno(stdin), sockfd) + 1;
        select(maxfdp1, &rset, NULL, NULL, NULL);

        if (FD_ISSET(sockfd, &rset))
        {
            if ((n = read(sockfd, recvbuf, sizeof(recvbuf))) == 0)
            {
                if (stdineof == 1)
                    goto end;

                printf("read error\n");
                exit(0);
            }

            recvbuf[n] = '\0';
            fputs(recvbuf, stdout);
        }

        if (FD_ISSET(fileno(stdin), &rset))
        {
            if (fgets(sendbuf, MAXLENGTH, stdin) == NULL)
            {
                stdineof = 1;
                shutdown(sockfd, SHUT_WR);
                FD_CLR(fileno(stdin), &rset);
                continue;
            }

            if (!strcmp(sendbuf, "\n"))
                continue;
            if (!strcmp(sendbuf, "close\n"))
                goto end;

            n = write(sockfd, sendbuf, strlen(sendbuf));
        }
    }

end:
    close(sockfd);

    return 0;
}
