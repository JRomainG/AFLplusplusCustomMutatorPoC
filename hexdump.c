#include <ctype.h>
#include <stdio.h>

// Modified from https://stackoverflow.com/a/7776146
void hexdump(const void *buf, const unsigned int len) {
    unsigned int i;
    unsigned char buff[17];
    const unsigned char *pc = (const unsigned char *)buf;

    for (i = 0; i < len; i++) {
        if ((i % 16) == 0) {
            if (i != 0) printf("  %s\n", buff);
            printf("  %04x ", i);
        }

        printf(" %02x", pc[i]);

        if (isprint(pc[i]) == 0)
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    while ((i % 16) != 0) {
        printf("   ");
        i++;
    }

    printf("  %s\n", buff);
}
