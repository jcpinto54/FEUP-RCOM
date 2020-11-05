 CC = gcc
CFLAGS = -Wall
DEPENDENCIES = utils/utils.h dataLayer/dataLayerPrivate.h dataLayer/dataLayer.h appLayer/applicationLayer.h
OBJECTS = utils/utils.o dataLayer/dataLayerPrivate.o dataLayer/dataLayer.o  appLayer/applicationLayer.o main.o

%.o: %.c $(DEPENDENCIES)
    $(CC) -c -o $@ $< $(CFLAGS)

proj: $(OBJECTS)
    $(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean
clean:
    rm proj *.o