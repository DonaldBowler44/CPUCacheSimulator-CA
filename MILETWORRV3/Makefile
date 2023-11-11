CC = gcc
CFLAGS = -Wall -g
TARGET = mainMileTwo
SRCS = mainMileTwo.c cache.c queue.c
HDRS = cache.h queue.h
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)
