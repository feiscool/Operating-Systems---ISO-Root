.PHONY: all clean

all: lsisoroot

lsisoroot: lsisoroot.c
	gcc -o lsisoroot lsisoroot.c

clean:
	rm -f lsisoroot
