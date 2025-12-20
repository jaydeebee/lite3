/*
    TRON (formerly known as Lite³): Tree Root Object Notation

    Copyright © 2025 Elias de Jong <elias@fastserial.com>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

      __ __________________        ____
    _  ___ ___/ /___(_)_/ /_______|_  /
     _  _____/ / __/ /_  __/  _ \_/_ <
      ___ __/ /___/ / / /_ /  __/____/
           /_____/_/  \__/ \___/
*/
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "lite3.h"

int main() {
    // Expected padding value
    #ifdef LITE3_DEBUG
        unsigned char expected_padding = 0x5F;
    #else
        unsigned char expected_padding = 0x00;
    #endif

    unsigned char buf[1024] __attribute__((aligned(LITE3_NODE_ALIGNMENT)));
    size_t buflen = 0;

    /*
     * TEST 1: Alignment padding during NEW KEY insertion.
     *
     * Initial size of an empty object: LITE3_NODE_SIZE (96).
     * Inserting key "a" with an empty object will add 99 bytes:
     * unaligned_val_ofs = LITE3_NODE_SIZE (96) + "a" (size 2 including \0) + key_tag (size 1) = 99
     * If LITE3_NODE_SIZE is 96, then it needs 1 byte padding to reach 100 bytes, which is 4 bytes alignment.
     * The padding should be at index LITE3_NODE_SIZE (96).
     */

    // Fill buffer with non-zero garbage
    memset(buf, 0xEE, sizeof(buf));

    // Initialize as object
    if (lite3_init_obj(buf, &buflen, sizeof(buf)) < 0) {
        perror("lite3_init_obj");
        return 1;
    }

    #ifdef LITE3_DEBUG
        printf("Test 1\n");
        printf("buflen after init: %zu\n", buflen);
    #endif


    size_t obj_ofs;
    if (lite3_set_obj(buf, &buflen, 0, sizeof(buf), "a", &obj_ofs) < 0) {
        perror("lite3_set_obj");
        return 1;
    }

    #ifdef LITE3_DEBUG
        printf("buflen after 'a': %zu\n", buflen);
        printf("Padding byte at index %d: 0x%02X (expected 0x%02X)\n", LITE3_NODE_SIZE, buf[LITE3_NODE_SIZE], expected_padding);
    #endif

    // Validate padding was zeroed
    assert(buf[LITE3_NODE_SIZE] == expected_padding);

    /*
     * TEST 2: Alignment padding during value UPDATE (append).
     *
     * Initial size after inserting "key1":"val1": 112 bytes (LITE3_NODE_SIZE (96) + keyval (16)).
     * keyval size: key_tag(1) + "key1\0"(5) + val_tag(1) + str_len(4) + "val1\0"(5) = 16 bytes.
     *
     * Updating "key1" to an Object (size 96). Since new size is larget, it appends.
     * unaligned_val_ofs = current_buflen (112) + key_tag(1) + "key1\0"(5) = 118.
     * Message requires 4-byte alignment.
     * 118 needs 2 bytes of padding to reach 120 (the next multiple of 4).
     * The padding bytes should be at indices 112 and 113.
     */

    // Reset buffer to garbage for second test
    memset(buf, 0xEE, sizeof(buf));
    buflen = 0;
    lite3_init_obj(buf, &buflen, sizeof(buf));

    #ifdef LITE3_DEBUG
        printf("\nTest 2\n");
        printf("buflen after init: %zu\n", buflen);
    #endif

    // Insert "key1": "val1"
    if (lite3_set_str(buf, &buflen, 0, sizeof(buf), "key1", "val1") < 0) {
        perror("lite3_set_str");
        return 1;
    }

    #ifdef LITE3_DEBUG
        printf("buflen after 'key1': %zu\n", buflen);
    #endif

    // Update "key1" to an Object
    size_t end_of_first_insert = LITE3_NODE_SIZE + 16;

    if (lite3_set_obj(buf, &buflen, 0, sizeof(buf), "key1", &obj_ofs) < 0) {
        perror("lite3_set_obj");
        return 1;
    }

    #ifdef LITE3_DEBUG
        printf("buflen after update 'key1': %zu\n", buflen);
        printf("Padding bytes at %zu, %zu: 0x%02X 0x%02X\n", end_of_first_insert, end_of_first_insert + 1, buf[end_of_first_insert], buf[end_of_first_insert + 1]);
    #endif

    assert(buf[end_of_first_insert] == expected_padding);
    assert(buf[end_of_first_insert + 1] == expected_padding);

    #ifdef LITE3_DEBUG
        printf("All alignment zeroing tests passed!\n");
    #endif
    return 0;
}
