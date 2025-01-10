profile =

# CC = cc -c
ifeq ($(profile), DEBUG)
CFLAGS = -std=c89 --debug -D DEBUG
else
CFLAGS = -std=c89
endif

# ABC = a b c
# $(ABC) = a b c

# all: main1.o main2.o
# all make 的时候可以同时执行 main1.o main2.o

# foo.o: bar.c baz.c
# $@ = foo.o（规则）
# $^ = bar.c baz.c（所有依赖）
# $< = bar.c（第一个依赖）

# .PHONY echoFoo
# make 每次执行都会判断文件是否最新，如果存在文件就不执行
# .PHONY 存在与规则同名文件的时候令不生成文件的规则仍能执行
# .PHONY 该工作目标不是一个真正的文件

LIBRARY_OBJECTS = socktest.o
CLIENT_OBJECTS = client.o selectclient.o
SERVER_OBJECTS = server.o selectserver.o pollserver.o epollserver.o kqueueserver.o
DEPEND_OBJECTS = selectclient.o $(SERVER_OBJECTS)

all: $(LIBRARY_OBJECTS) $(CLIENT_OBJECTS) $(SERVER_OBJECTS)

$(LIBRARY_OBJECTS): %.o: %.c
	$(CC) -c $(CFLAGS) $^

$(CLIENT_OBJECTS) $(SERVER_OBJECTS): %.o: %.c
	$(CC) -o $@ $(CFLAGS) $^

$(DEPEND_OBJECTS): socktest.o

.PHONY: clean echoFoo

clean:
	rm -f *.o *.out *.h.gch

echoFoo:
	echo FOO FOO FOO FOO
