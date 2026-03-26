CC     = gcc
CFLAGS = -Wall -Wextra -g
LDLIBS = -lm

TARGET = compare
SRCS   = compare.c wfd.c
OBJS   = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

compare.o: compare.c wfd.h
wfd.o:     wfd.c wfd.h

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
