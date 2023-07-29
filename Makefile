# spximg Makefile

SRC=main.c
EXE=spximg
HEADER=spximg.h
SCRIPT=build.sh

CC=gcc
STD=-std=c89
OPT=-O2
WFLAGS=-Wall -Wextra -pedantic
INC=-I.
LIB=-ljpeg -lpng -lz

CFLAGS=$(STD) $(OPT) $(WFLAGS) $(INC) $(LIB)

$(EXE): $(SRC) $(HEADER)
	$(CC) $< -o $@ $(CFLAGS)

clean:
	$(RM) $(EXE)

install: $(SCRIPT)
	./$< $@

uninstall: $(SCRIPT)
	./$< $@
