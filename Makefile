all: build/GoGiShell

build/GoGiShell: build/src/main.o build/src/commands.o build/src/pseudoshell.o
	@mkdir -p build
	gcc build/src/main.o build/src/commands.o build/src/pseudoshell.o -o build/GoGiShell

build/src/main.o: src/main.c src/headers.h
	@mkdir -p build/src
	gcc -Wall -Wextra -c src/main.c -o build/src/main.o

build/src/commands.o: src/commands.c src/headers.h
	@mkdir -p build/src
	gcc -Wall -Wextra -c src/commands.c -o build/src/commands.o

build/src/pseudoshell.o: src/pseudoshell.c src/headers.h
	@mkdir -p build/src
	gcc -Wall -Wextra -c src/pseudoshell.c -o build/src/pseudoshell.o

run: build/GoGiShell
	./build/GoGiShell

test: build/GoGiShell build/tests/test_main
	rm -rf ~/.gogicache
	./build/tests/test_main
	rm -rf ~/.gogicache

build/tests/test_main: build/tests/test_main.o
	@mkdir -p build/tests
	gcc -Wall -Wextra -o build/tests/test_main build/tests/test_main.o

build/tests/test_main.o: tests/test_main.c
	@mkdir -p build/tests
	gcc -Wall -Wextra -c tests/test_main.c -o build/tests/test_main.o

clean:
	rm -rf build ~/.gogicache
