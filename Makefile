all: teapot.c
	gcc -L/usr/lib64 -O2 teapot.c -lX11 -lGL -lGLU -lglut -lm -lXmu -o run

clean: 
	rm -f run
