CC = cc
CFLAGS = -Wall

proj: main.c application.c utils.c
	$(CC) $(CFLAGS) $? -o $@

.PHONY: clean
clean:
	rm proj
