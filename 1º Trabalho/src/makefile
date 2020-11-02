CC = gcc
CFLAGS = -Wall


proj: utils/utils.c dataLayer/dataLayerPrivate.c dataLayer/dataLayer.c  appLayer/applicationLayer.c main.c
	$(CC) $(CFLAGS) $? -o $@

.PHONY: clean
clean:
	rm proj
