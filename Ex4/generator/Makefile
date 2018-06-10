#!/usr/bin/make
.SUFFIXES:

CFLAGS = -c -Os -Wall -Werror -Wno-unused-value -Wno-deprecated

%.o: %.c
	$(CC) $(CFLAGS) $^ -o $@

%: %.o
	$(CC) -o $@ $^

clean:
	$(RM) $(RMFILES) *.o
