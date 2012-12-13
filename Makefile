CC = gcc

CFLAGS = -Wno-unused-parameter `pkg-config gtk+-3.0 webkitgtk-3.0 --cflags`

LDFLAGS = `pkg-config gtk+-3.0 webkitgtk-3.0 --libs`

SRCS = webshot.c

OBJS = $(SRCS:.c=.o)

MAIN = webshot

all: $(MAIN)

$(MAIN): $(OBJS)
	$(CC) $(OBJS) $(CFLAGS) $(LDFLAGS) -o $(MAIN)

.cpp.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(MAIN)
