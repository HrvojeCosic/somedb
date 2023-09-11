#include <stdint.h>

//--------------------------------/* Serialization sizes and offsets */--------------------------------
// BTree index page:
#define TREE_KV_PTR_SIZE 4
#define AVAILABLE_SPACE_START_SIZE 2
#define AVAILABLE_SPACE_START_OFFSET 0
#define AVAILABLE_SPACE_END_SIZE 2
#define AVAILABLE_SPACE_END_OFFSET AVAILABLE_SPACE_START_OFFSET + AVAILABLE_SPACE_START_SIZE
#define NEXT_PID_SIZE 4
#define NEXT_PID_OFFSET AVAILABLE_SPACE_END_OFFSET + AVAILABLE_SPACE_END_SIZE
#define RIGHTMOST_PID_SIZE 4
#define RIGHTMOST_PID_OFFSET NEXT_PID_OFFSET + NEXT_PID_SIZE
#define TREE_FLAGS_SIZE 1
#define TREE_FLAGS_OFFSET RIGHTMOST_PID_OFFSET + RIGHTMOST_PID_SIZE
#define IS_LEAF_SIZE 1
#define IS_LEAF_OFFSET TREE_FLAGS_OFFSET + TREE_FLAGS_SIZE
#define INDEX_PAGE_HEADER_SIZE                                                                                         \
    AVAILABLE_SPACE_START_SIZE + AVAILABLE_SPACE_END_SIZE + NEXT_PID_SIZE + RIGHTMOST_PID_SIZE + TREE_FLAGS_SIZE +     \
        IS_LEAF_OFFSET

// BTree metadata page:
#define BTREE_METADATA_PAGE_ID 0
#define MAGIC_NUM_SIZE 4
#define MAGIC_NUMBER_OFFSET 0
#define NODE_COUNT_SIZE 2
#define NODE_COUNT_OFFSET MAGIC_NUM_SIZE
#define MAX_KEYSIZE_SIZE 1
#define MAX_KEYSIZE_OFFSET MAGIC_NUM_SIZE + NODE_COUNT_SIZE
#define CURR_ROOT_PID_SIZE 4
#define CURR_ROOT_PID_OFFSET MAGIC_NUM_SIZE + NODE_COUNT_SIZE + MAX_KEYSIZE_SIZE

// Heap file (TODO)
//------------------------------------------------------------------------------------------------------

/**
 * Encoding/decoding functions for serializing different data types used for disk persistence
 */

void encode_uint32(uint32_t data, uint8_t *buf);
uint32_t decode_uint32(uint8_t *buf);

void encode_uint16(uint16_t data, uint8_t *buf);
uint16_t decode_uint16(uint8_t *buf);

void encode_int32(int data, uint8_t *buf);
int32_t decode_int32(uint8_t *buf);

void encode_int16(int data, uint8_t *buf);
int16_t decode_int16(uint8_t *buf);

void encode_double(double data, uint8_t *buf);
double decode_double(uint8_t *buf);

void encode_bool(bool data, uint8_t *buf);
bool decode_bool(uint8_t *buf);
