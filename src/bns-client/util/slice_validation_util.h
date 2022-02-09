#ifndef BNS_CLIENT_SRC_BNS_CLIENT_UTIL_SLICE_VALIDATION_UTIL_H_
#define BNS_CLIENT_SRC_BNS_CLIENT_UTIL_SLICE_VALIDATION_UTIL_H_

#include <bns-client/core/core.h>

_CHECK_RESULT
bns_exit_code_t is_leaf_node(const slice_t* slice, const pb_pair_t* pbPair);

_CHECK_RESULT
bns_exit_code_t get_leaf_node_hash(const slice_t* slice, char* leafNodeHash);

_CHECK_RESULT
bns_exit_code_t eval_root_hash_from_slice(const slice_t* slice, bool* result);

_CHECK_RESULT
char* slice_get_root_hash(const slice_t* slice);

#endif  // BNS_CLIENT_SRC_BNS_CLIENT_UTIL_SLICE_VALIDATION_UTIL_H_
