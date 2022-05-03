AFL_DIR=./aflplusplus

all: hexdump mutator target

hexdump: hexdump.c
	gcc -c -Wall -Werror -ldl -o hexdump.o hexdump.c

mutator: mutator.c
	gcc -c -Wall -Wno-unused-function -Werror -fPIC -o mutator.o mutator.c -I$(AFL_DIR)/include -I$(AFL_DIR)/custom_mutators/examples
	gcc -shared -o libmutator.so mutator.o hexdump.o

target: target.cpp
	$(AFL_DIR)/afl-g++ -Wall -Werror -O0 -o target.o target.cpp hexdump.o

clean:
	$(RM) *.o *.so
