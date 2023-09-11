#include "../../include/utils/serialize.h"
#include <string.h>

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

// Warning: whether we get the correct negative number when converting from int to uint
// and the other way around is technically implementation defined, but it seems to work correctly in g++
// https://timsong-cpp.github.io/cppwp/n3337/conv.integral#3
void encode_int32(int data, uint8_t *buf) { encode_uint32((uint32_t)data, buf); }

int32_t decode_int32(uint8_t *buf) { return (int32_t)decode_uint32(buf); }

void encode_int16(int data, uint8_t *buf) { encode_uint16((uint16_t)data, buf); }

int16_t decode_int16(uint8_t *buf) { return (int16_t)decode_uint16(buf); }

// For doubles, we will assume there needs to be no difference between disk and memory representation,
// since it simplifies the implementation and will generally be fine since vast majority of CPUs follow the IEE754
// https://stackoverflow.com/questions/2234468/do-any-real-world-cpus-not-use-ieee-754
void encode_double(double data, uint8_t *buf) { memcpy(buf, &data, sizeof(double)); }

double decode_double(uint8_t *buf) {
    double val;
    memcpy(&val, buf, sizeof(double));
    return val;
}

void encode_bool(bool data, uint8_t *buf) { buf[0] = data; };

bool decode_bool(uint8_t *buf) { return buf[0]; };