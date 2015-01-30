all:
	gcc -g debug.c disasm.c main.c mem.c readfile.c readline.c run.c -o sim
