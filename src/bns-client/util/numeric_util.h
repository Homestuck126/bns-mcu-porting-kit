#ifndef BNS_C_CLIENT_SRC_NUMERIC_UTIL_H_
#define BNS_C_CLIENT_SRC_NUMERIC_UTIL_H_

#include <bns-client/core/bns_definitions.h>

//number of digits for the int
_CHECK_RESULT
size_t bns_digits(long long int num);
//change long long to string
_CHECK_RESULT
char* bns_lltos(long long int num);

#endif  // BNS_C_CLIENT_SRC_NUMERIC_UTIL_H_
