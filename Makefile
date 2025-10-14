all: main.c makesvg.c
	$(CC) -ggdb -o map2img main.c makesvg.c
