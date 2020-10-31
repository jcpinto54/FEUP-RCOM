CC = gcc
CFLAGS = -Wall


proj: utils.c dataLayerPrivate.c dataLayer.c  main.c
	$(CC) $(CFLAGS) $? -o $@

.PHONY: clean
clean:
	rm proj
