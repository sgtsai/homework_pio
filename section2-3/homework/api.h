#ifndef HW23_API_H
#define HW23_API_H
#include <stddef.h>
#include <stdbool.h>

#define HW_CAPACITY 8
typedef struct {
    const void *src;
    void *dst;
    size_t count;          /* transfers, not bytes */
    size_t size;           /* 1, 2, or 4 bytes */
    size_t src_bytes;      /* available bytes beginning at src */
    size_t dst_bytes;      /* available bytes beginning at dst */
    bool src_increment;
    bool dst_increment;
} hw_job;

/* Supplied by the judge. Call exactly once per transfer after validation.
 * It copies size bytes and can deliberately wait to model a slow device. */
void hw_copy_unit(void *dst, const void *src, size_t size);
#endif
