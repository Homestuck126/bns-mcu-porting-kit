#ifndef BNS_C_CLIENT_SRC_BNS_CLIENT_BYTE_UTIL_H_
#define BNS_C_CLIENT_SRC_BNS_CLIENT_BYTE_UTIL_H_

#include <stddef.h>
//change hex to byte
void bns_hex_to_byte(const char* hex, size_t size, unsigned char* byte);
//change byte to hex
void bns_byte_to_hex(const unsigned char* byte, size_t size, char* hex);
//Does not exist? 
void bns_print_byte(const unsigned char* byte, int size);

#endif  // BNS_C_CLIENT_SRC_BNS_CLIENT_BYTE_UTIL_H_
