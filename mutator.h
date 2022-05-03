#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "hexdump.h"
#include "custom_mutator_helpers.h"

typedef struct custom_mutator {
    // Buffers reused at each iteration
    BUF_VAR(uint8_t, fuzz);
    BUF_VAR(uint8_t, post_process);
} custom_mutator_t;

custom_mutator_t *afl_custom_init(void *afl, unsigned int seed);
size_t afl_custom_fuzz(custom_mutator_t *data, uint8_t *buf, size_t buf_size, uint8_t **out_buf, uint8_t *add_buf, size_t add_buf_size, size_t max_size);
size_t afl_custom_post_process(custom_mutator_t *data, uint8_t *buf, size_t buf_size, uint8_t **out_buf);
size_t afl_custom_post_process(custom_mutator_t *data, uint8_t *buf, size_t buf_size, uint8_t **out_buf);
void afl_custom_deinit(custom_mutator_t *data);
