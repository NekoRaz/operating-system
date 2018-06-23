#!/usr/bin/make
.SUFFIXES:
.PHONY: all run clean

TAR = philosophers
SRC = $(wildcard *.c)
OBJ = $(SRC:%.c=%.o)
CFLAGS = -std=gnu11 -c -g -Os -Wall -Werror -Wno-unused-value -Wno-deprecated
LFLAGS = -pthread

%.o: %.c %.h
	$(CC) $(CFLAGS) $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(TAR): $(OBJ)
	$(CC) $(LFLAGS) $^ -o $@

all: $(TAR)

run: all
	./$(TAR)

clean:
	$(RM) $(RMFILES) $(OBJ) $(TAR)
