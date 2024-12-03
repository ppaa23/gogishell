all: build/program_name

build/program_name: build/src/main.o
	gcc build/src/main.o -o build/program_name

build/src/main.o: src/main.c
	mkdir -p build/src
	gcc -Wall -Wextra -c src/main.c -o build/src/main.o

test: build/run_tests
	./build/run_tests

build/run_tests: build/tests/test_main.o
	gcc build/tests/test_main.o -o build/run_tests

build/tests/test_main.o: tests/test_main.c
	mkdir -p build/tests
	gcc -Wall -Wextra -c tests/test_main.c -o build/tests/test_main.o

clean:
	rm -rf build
