CC=gcc
CFLAGS=-Wall -Iinclude -fPIC 

acc.so: build/ build/scanner.o
	$(CC) $(CFLAGS) -shared -o build/acc.so build/scanner.o

build/scanner.o: build/ source/scanner.c   
	$(CC) $(CFLAGS) -o build/scanner.o -c source/scanner.c

build:
	mkdir -p build

clean:
	rm -rf build
	
