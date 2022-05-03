#include "mutator.h"

#define DEBUG
#define MUTATOR_APPEND "0123456789ABCDEF"

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

// Our custom mutator handles data with a custom encoding
// In this example, the size of the buffer is simply encoded at the start of the
// buffer
// A more realistic example would be a mutator working on protobuf data and
// decoding it before passing it to the target
uint32_t decode_size(uint8_t *buf) {
    return buf[3] + (buf[2] << 8) + (buf[1] << 16) + (buf[0] << 24);
}

void encode_size(uint32_t size, uint8_t *buf) {
    buf[3] = size >> 0;
    buf[2] = size >> 8;
    buf[1] = size >> 16;
    buf[0] = size >> 24;
}

int is_valid_encoding(uint8_t *buf, size_t buf_size) {
    if (buf_size < 4 || buf_size > 0xffffff00) {
        printf("[mutator] Buffer size out of range: %ld\n", buf_size);
        return 0;
    }

    uint32_t size = decode_size(buf);
    if (size + 4 != buf_size) {
        printf("[mutator] Buffer size doesn't match encoded size: expected %d but got %ld\n", size + 4, buf_size);
        debug_printf("[mutator] According to the encoding, the buffer looks like:\n");
        debug_hexdump(buf, size + 4);
        return 0;
    }
    return 1;
}

void encode(uint8_t *from_buf, uint32_t from_buf_size, uint8_t *to_buf, uint32_t *to_buf_size) {
    uint8_t header[4];
    encode_size(from_buf_size, header);
    memmove(to_buf + 4, from_buf, from_buf_size);
    memmove(to_buf, header, sizeof(header));
    if (to_buf_size != NULL) {
        *to_buf_size = from_buf_size + sizeof(header);
    }
}

void decode(uint8_t *from_buf, uint8_t *to_buf, uint32_t *to_buf_size) {
    uint32_t size = decode_size(from_buf);
    memmove(to_buf, from_buf + 4, size);
    if (to_buf_size != NULL) {
        *to_buf_size = size;
    }
}

/**
 * Initialize this custom mutator
 *
 * @param[in] afl a pointer to the internal state object. Can be ignored for
 * now.
 * @param[in] seed A seed for this mutator - the same seed should always mutate
 * in the same way.
 * @return Pointer to the data object this custom mutator instance should use.
 *         There may be multiple instances of this mutator in one afl-fuzz run!
 *         Return NULL on error.
 */
custom_mutator_t *afl_custom_init(void *afl, unsigned int seed) {
    custom_mutator_t *data = (custom_mutator_t *)calloc(1, sizeof(custom_mutator_t));
    if (!data) {
        perror("[mutator] custom_mutator alloc failed");
        return NULL;
    }
    srand(seed);
    return data;
}

/**
 * Perform custom mutations on a given input
 *
 * @param[in] data pointer returned in afl_custom_init for this fuzz case
 * @param[in] buf Pointer to input data to be mutated
 * @param[in] buf_size Size of input data
 * @param[out] out_buf the buffer we will work on. we can reuse *buf. NULL on
 * error.
 * @param[in] add_buf Buffer containing the additional test case
 * @param[in] add_buf_size Size of the additional test case
 * @param[in] max_size Maximum size of the mutated output. The mutation must not
 *     produce data larger than max_size.
 * @return Size of the mutated output.
 */
extern size_t afl_custom_fuzz(custom_mutator_t *data,
                                  uint8_t *buf, size_t buf_size,
                                  uint8_t **out_buf,
                                  uint8_t *add_buf, size_t add_buf_size,
                                  size_t max_size) {
    debug_printf("\n[mutator] Called afl_custom_fuzz with buffer of size %ld\n", buf_size);
    debug_hexdump(buf, buf_size);

    // The buffer is supposed to be encoded in a custom internal representation,
    // with the size at the start
    if (!is_valid_encoding(buf, buf_size)) {
        *out_buf = NULL;
        return 0;
    }

    // Append a random number of characters at the end of the buffer
    int append = rand() % sizeof(MUTATOR_APPEND);

    if (!maybe_grow(BUF_PARAMS(data, fuzz), buf_size + append)) {
        *out_buf = NULL;
        perror("[mutator] maybe_grow failed");
        return 0;
    }

    // Mutate our data
    uint32_t decoded_size, encoded_size;
    decode(buf, data->fuzz_buf, &decoded_size);
    memcpy(data->fuzz_buf + decoded_size, MUTATOR_APPEND, append);
    encode(data->fuzz_buf, decoded_size + append, data->fuzz_buf, &encoded_size);

    debug_printf("[mutator] Generated mutated data of size %ld\n", encoded_size);
    debug_hexdump(data->fuzz_buf, encoded_size);

    *out_buf = data->fuzz_buf;
    return encoded_size;
}

/**
 * A post-processing function to use right before AFL writes the test case to
 * disk in order to execute the target.
 *
 * (Optional) If this functionality is not needed, simply don't define this
 * function.
 *
 * @param[in] data pointer returned in afl_custom_init for this fuzz case
 * @param[in] buf Buffer containing the test case to be executed
 * @param[in] buf_size Size of the test case
 * @param[out] out_buf Pointer to the buffer containing the test case after
 *     processing. External library should allocate memory for out_buf.
 *     The buf pointer may be reused (up to the given buf_size);
 * @return Size of the output buffer after processing or the needed amount.
 *     A return of 0 indicates an error.
 */
extern size_t afl_custom_post_process(custom_mutator_t *data, uint8_t *buf,
                                          size_t buf_size, uint8_t **out_buf) {
    debug_printf("\n[mutator] Called afl_custom_post_process with buffer of size %ld\n", buf_size);
    debug_hexdump(buf, buf_size);

    if (!is_valid_encoding(buf, buf_size)) {
        *out_buf = NULL;
        return 0;
    }

    // Before passing the data to the target, we need to convert from our
    // internal representation to the representation expected by the target
    if (!maybe_grow(BUF_PARAMS(data, post_process), buf_size - 4)) {
        *out_buf = NULL;
        perror("[mutator] [afl_custom_post_process] maybe_grow failed");
        return 0;
    }
    uint32_t decoded_size;
    decode(buf, data->post_process_buf, &decoded_size);

    debug_printf("[mutator] Generated post-process data of size %ld\n", decoded_size);
    debug_hexdump(data->post_process_buf, decoded_size);

    *out_buf = data->post_process_buf;
    return decoded_size;
}

/**
 * Deinitialize everything
 *
 * @param data The data ptr from afl_custom_init
 */
extern void afl_custom_deinit(custom_mutator_t *data) {
    debug_printf("[mutator] Cleaning up\n");
    free(data->fuzz_buf);
    free(data->post_process_buf);
    free(data);
}
