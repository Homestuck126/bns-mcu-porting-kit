#ifndef BNS_C_CLIENT_SRC_SLICE_H_
#define BNS_C_CLIENT_SRC_SLICE_H_

#include <bns-client/core/core.h>

#define SLICE_PRINT_FORMAT "slice=%lld.%s"
#define SLICE_TO_PRINT_ARGS(slice) slice->index, (slice)->hashString

//parse through the slicestring and put into slice
_CHECK_RESULT
bns_exit_code_t parse_slice(const char* sliceString, slice_t* slice);
//turns the slice into a string in the form of index and hashString
bns_exit_code_t slice_to_string(const slice_t* slice, char** outputString);
//frees up slice
void slice_free(slice_t* slice);

#endif  // BNS_C_CLIENT_SRC_SLICE_H_
