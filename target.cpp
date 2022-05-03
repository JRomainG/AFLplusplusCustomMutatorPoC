#include <dlfcn.h>
#include <stdarg.h>
#include <fstream>
#include <cstring>

extern "C" {
    #include "hexdump.h"
}

#define DEBUG

void debug_printf(const char *format, ...) {
#ifdef DEBUG
    va_list args;
	va_start(args, format);
    vprintf(format, args);
#endif
}

void debug_hexdump(const void *buf, const unsigned int len) {
#ifdef DEBUG
    hexdump(buf, len);
#endif
}

std::string readFile(const char* path) {
    debug_printf("[target] Reading file at %s\n", path);
    std::ifstream file(path, std::ios::in | std::ios::binary);

    if (!file) {
        printf("[target] File not found: %s\n", path);
        exit(1);
    }

    std::string str = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return str;
}

int fuzz(char *path) {
    char buf[16];
    std::string input = readFile(path);

    debug_printf("[target] Read %ld bytes from file\n", input.length());
    debug_hexdump(input.c_str(), input.length());
    memcpy(buf, input.c_str(), input.length());
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("[target] Usage: ./target.o <file path>\n");
        return 1;
    }
    return fuzz(argv[1]);
}
