/*
 * cc --std=c89 -o client.o client.c && ./client.o
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
/* #include <sys/types.h> */
#include <unistd.h>

#define MAXLENGTH 1024

int main(int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr;
    char readbuf[MAXLENGTH];
    char *host = "127.0.0.1";
    int port = 8081;
    int error;
    ssize_t n;

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

    while (fgets(readbuf, MAXLENGTH, stdin) != NULL)
    {
        if (!strcmp(readbuf, "\n"))
            continue;
        if (!strcmp(readbuf, "close\n"))
            break;

        n = write(sockfd, readbuf, strlen(readbuf));

        while ((n = read(sockfd, readbuf, sizeof(readbuf))) > 0)
        {
            readbuf[n] = '\0';
            fputs(readbuf, stdout);
            if (readbuf[n - 1] == '\n')
                break;
        }
    }

    close(sockfd);

    return 0;
}
