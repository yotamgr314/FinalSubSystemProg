all: server

clean:
	@rm -rf *.o
	@rm -rf server

server: main.o httpd.o router.o
	gcc -o server main.o httpd.o router.o

main.o: main.c httpd.h
	gcc -c -o main.o main.c

httpd.o: httpd.c httpd.h
	gcc -c -o httpd.o httpd.c

router.o: router.c httpd.h
	gcc -c -o router.o router.c
