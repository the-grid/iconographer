negl: negl.c Makefile
	gcc -o negl negl.c `pkg-config --cflags --libs gegl-0.3` -Wall -Wextra -std=c99
