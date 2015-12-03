iconographer: iconographer.c Makefile *.c
	gcc -O3 -ffast-math -o iconographer *.c `pkg-config --cflags --libs gegl-0.3` -Wall -Wextra -std=c99
clean:
	rm iconographer
install:
	cp iconographer /usr/local/bin
