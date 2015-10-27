iconographer: iconographer.c Makefile
	gcc -O3 -o iconographer iconographer.c `pkg-config --cflags --libs gegl-0.3` -Wall -Wextra -std=c99
clean:
	rm iconographer
install:
	cp iconographer /usr/local/bin
