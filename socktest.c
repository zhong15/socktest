/*
 * cc --std=c89 -c -o socktest.o socktest.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "socktest.h"

int max(int a, int b)
{
    return a > b ? a : b;
}

char *gettimestr(void)
{
    time_t t;
    struct tm *i;
    char *s;
    int n;

    t = time(NULL);
    i = localtime(&t);
    s = (char *)malloc(sizeof(char) * 20);

    n = sprintf(s, "%d", i->tm_year + 1900);
    n += sprintf(s + n, "-%02d", i->tm_mon + 1);
    n += sprintf(s + n, "-%02d", i->tm_mday);
    n += sprintf(s + n, " %02d", i->tm_hour);
    n += sprintf(s + n, ":%02d", i->tm_min);
    n += sprintf(s + n, ":%02d", i->tm_sec);

    return s;
}
