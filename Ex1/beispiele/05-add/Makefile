#!/usr/bin/make
.SUFFIXES:

%.s: %.o
	objdump -d -j .text --no-show-raw-insn -S $^ > $@

%.o: %.c
	$(CC) -g -o $@ $^

%.o: %.s
	$(AS) -o $@ $^

%: %.o
	$(LD) -o $@ $^

%: %.c
	$(CC) -g -o $@ $^
