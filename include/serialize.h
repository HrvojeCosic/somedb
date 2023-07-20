#include <stdint.h>

/**
 * Header file containing encoding/decoding functions for serializing data used to persist it on disk
 *
 * Serialization formats:
 * uint32 - big endian binary format
 * uint16 - big endian binary format
 */

void encode_uint32(uint32_t data, uint8_t *buf);
uint32_t decode_uint32(uint8_t *buf);

void encode_uint16(uint16_t data, uint8_t *buf);
uint32_t decode_uint16(uint8_t *buf);
