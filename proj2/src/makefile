CC = gcc
CFLAGS = -Wall -g
DEPS = urlHandler.h macros.h clientTCP.h
OBJS = main.o urlHandler.o clientTCP.o
TARGET = download

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS) 
	$(CC) -o $(TARGET) $(OBJS)

.PHONY: clean
clean:
	rm $(TARGET) *.o
