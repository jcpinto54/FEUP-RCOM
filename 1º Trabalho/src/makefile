CC = gcc
CFLAGS = -Wall -g
DEPS = appLayer/applicationLayer.h dataLayer/dataLayer.h dataLayer/dataLayerPrivate.h utils/utils.h macros.h
OBJS = utils/utils.o dataLayer/dataLayerPrivate.o dataLayer/dataLayer.o  appLayer/applicationLayer.o main.o
TARGET = proj

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJS) 
	$(CC) -o $(TARGET) $(OBJS)

.PHONY: clean
clean:
	rm proj *.o ./*/*.o
