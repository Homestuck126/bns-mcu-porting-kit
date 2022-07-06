#ifndef BNS_C_CLIENT_SRC_STRING_UTIL_H_
#define BNS_C_CLIENT_SRC_STRING_UTIL_H_

#include <bns-client/core/bns_definitions.h>
#include <stdbool.h>

//equal strings check
_CHECK_RESULT
bool bns_equals_ignore_case(const char* a, const char* b);
//equal strings check with a check that their length is < n
_CHECK_RESULT
bool bns_equals_n_ignore_case(const char* a, const char* b, size_t n);
//copy src to dest 
void bns_strdup(char** dest, const char* src);
//removes end slash 
void remove_end_slash(char* string);

#endif  // BNS_C_CLIENT_SRC_STRING_UTIL_H_
