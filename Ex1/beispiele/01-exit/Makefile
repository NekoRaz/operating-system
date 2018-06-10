#!/usr/bin/make
.SUFFIXES:

%.o: %.s
	$(AS) -o $@ $^

%: %.o
	$(LD) -o $@ $^
