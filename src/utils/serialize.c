#include "../../include/utils/serialize.h"

void encode_uint32(uint32_t data, uint8_t *buf) {
    buf[0] = (data >> 24) & 0xFF;
    buf[1] = (data >> 16) & 0xFF;
    buf[2] = (data >> 8) & 0xFF;
    buf[3] = data & 0xFF;
}

uint32_t decode_uint32(uint8_t *buf) { return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3]; }

void encode_uint16(uint16_t data, uint8_t *buf) {
    buf[0] = (data >> 8) & 0xFF;
    buf[1] = data & 0xFF;
}

uint16_t decode_uint16(uint8_t *buf) { return (buf[0] << 8) | buf[1]; }
