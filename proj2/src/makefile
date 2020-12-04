CC = gcc
CFLAGS = -Wall -g
DEPS = utils/utils.h macros.h
OBJS = utils/utils.o main.o
TARGET = download

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS) 
	$(CC) -o $(TARGET) $(OBJS)

.PHONY: clean
clean:
	rm $(TARGET) *.o ./*/*.o
