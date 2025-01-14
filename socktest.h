#ifndef SOCKTEST_H_
#define SOCKTEST_H_

#if defined(__unix__) || defined(__linux__)
#define HAVE_EPOLL 1
#endif

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#define HAVE_KQUEUE 1
#endif

#define MAX(a, b) ((a) > (b)) ? (a) : (b)

char *gettimestr(void);

#endif /* SOCKTEST_H_ */
