# socktest
Socket Test

| 文件 | 说明 |
| --- | --- |
| server.c | 服务端：阻塞 IO |
| client.c | 客户端：阻塞 IO |
| selectserver.c | 服务端：IO 复用 select |
| selectclient.c | 客户端：IO 复用 select |
| pollserver.c | 服务端：IO 复用 poll |
| epollserver.c | 服务端：IO 复用 epoll |
| kqueueserver.c | 服务端：IO 复用 kqueue |