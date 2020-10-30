CC = cc
CFLAGS = -Wall


proj: main.c dataLayer.c dataLayerPrivate.c utils.c
	$(CC) $(CFLAGS) $? -o $@

.PHONY: clean
clean:
	rm proj
