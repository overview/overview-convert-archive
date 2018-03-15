CC=gcc
CFLAGS=-I../libarchive-${LIBARCHIVE_VERSION}/libarchive -O2
LDFLAGS=-static -L../libarchive-${LIBARCHIVE_VERSION}/.libs -larchive -lbz2 -lz -llz4 -llzma -lcrypto -s

archive-to-multipart: src/archive-to-multipart.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
