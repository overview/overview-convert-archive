CC=gcc
LDFLAGS=-larchive

archive-to-multipart: src/archive-to-multipart.c
	$(CC) $< -o $@ $(LDFLAGS)
