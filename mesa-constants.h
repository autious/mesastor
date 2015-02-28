#ifndef MESA_CONSTANTS_H
#define MESA_CONSTANTS_H

static const char magic_bytes[] = { 0x4d, 0x45, 0x53, 0x41, 0x53, 0x54, 0x4f, 0x52 };
static const int MAGIC_LEN = 8;

static const long ENDIANNESS_POS = 8;
static const long VERSION_POS = 12;
static const long INSTANCE_LENGTH_POS = 16;
static const long INSTANCE_COUNT_POS = 20;
static const long INSTANCE_START = 32;

static const int UINT_LEN = 4;

static const uint32_t endianess = 0xffff0000; //This will always match endianess on "my" system.
static const uint32_t other_endianess = 0x0000ffff; //This will always match endianess on "other" system 
                                                    //(little/big) meaning you will need to flip to 
                                                    //interpret correctly.
static const uint32_t version_number = 1;

#endif
