#include <bns-client/sha256/sha256.h>
#include <bns-client/util/byte_util.h>
#include <bns-client/util/log.h>
#include <bns-client/util/slice_validation_util.h>
#include <bns-client/util/string_util.h>
#include <bns-client/verify/slice.h>
#include <string.h>
//check if leaf node is same as pbPairs 
bns_exit_code_t is_leaf_node(const slice_t* const   slice,
                             const pb_pair_t* const pbPair) {
  bns_exit_code_t exitCode;
  char*           pbPairValue     = NULL;
  unsigned char*  pbPairValueByte = NULL;
  //If slice or pbPair does not exist, error
  if (!slice) {
    exitCode = BNS_SLICE_NULL_ERROR;
    goto is_leaf_node_fail;
  }
  if (!pbPair) {
    exitCode = BNS_PB_PAIR_NULL_ERROR;
    goto is_leaf_node_fail;
  }
  LOG_DEBUG("is_leaf_node() begin, " SLICE_PRINT_FORMAT,
            SLICE_TO_PRINT_ARGS(slice));
  //intalizes leafNodeHash as size hash length as empty 
  char leafNodeHash[HASH_STR_LEN] = {0};
  //get leaf nodes hash
  if ((exitCode = get_leaf_node_hash(slice, leafNodeHash)) != BNS_OK) {
    goto is_leaf_node_fail;
  }
  //sets size to size * 64 
  size_t size = pbPair->size * (HASH_STR_LEN - 1);
  //allocate size for pbPairValue 
  pbPairValue = (char*)malloc(sizeof(char) * (size + 1));
  memset(pbPairValue, 0, size + 1);
  //put pb pair values into pbPairValue combined
  for (size_t i = 0; i < pbPair->size; i++) {
    strcat(pbPairValue, pbPair->pbPairValue[i].value);
  }
  //allocate size for pbPair
  pbPairValueByte = (unsigned char*)malloc(sizeof(unsigned char) * (size / 2));
  bns_hex_to_byte(pbPairValue, size, pbPairValueByte);
  BNS_FREE(pbPairValue);
  //encrypt pbPairValueByte
  char hashValue[HASH_STR_LEN] = {0};
  sha256(pbPairValueByte, size / 2, hashValue);

  BNS_FREE(pbPairValueByte);
  //check that  leafnodeHash is same as Hash
  if (bns_equals_ignore_case(leafNodeHash, hashValue) != true) {
    exitCode = BNS_PB_PAIR_IN_LEAF_NODE_ERROR;
    goto is_leaf_node_fail;
  }
  LOG_DEBUG("is_leaf_node() end");
  return exitCode;

is_leaf_node_fail:
  LOG_ERROR("is_leaf_node() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//grabs either slices 0 or 1 from hashStringList dependeing on index
bns_exit_code_t get_leaf_node_hash(const slice_t* const slice,
                                   char*                leafNodeHash) {
  bns_exit_code_t exitCode = BNS_OK;
  //if slice does not exist, error
  if (!slice) {
    exitCode = BNS_SLICE_NULL_ERROR;
    goto get_leaf_node_hash_fail;
  }
  //if the index is <0 , error
  if (slice->index < 0) {
    exitCode = BNS_SLICE_INDEX_NEGATIVE_ERROR;
    goto get_leaf_node_hash_fail;
  }
  //if leafNodeHash DNE, error
  if (!leafNodeHash) {
    exitCode = BNS_LEAF_NODE_HASH_NULL_ERROR;
    goto get_leaf_node_hash_fail;
  }
  //if slice->index is even, or 1, index is 0, else index is 1
  size_t index;
  if (slice->index % 2 == 0 || slice->index == 1) {
    index = 0;
  } else {
    index = 1;
  }
  //if index >= slices size
  if (index >= slice->size) {
    exitCode = BNS_GET_LEAF_NODE_HASH_OUT_OF_RANGE_ERROR;
    goto get_leaf_node_hash_fail;
  }
  //copy leafNodeHash from slice
  strncpy(leafNodeHash, slice->hashStringList[index], HASH_STR_LEN - 1);
  LOG_DEBUG("get_leaf_node_hash() end, leafNodeHash=%s", leafNodeHash);
  return exitCode;

get_leaf_node_hash_fail:
  LOG_ERROR("get_leaf_node_hash() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//get root hash from slice 
bns_exit_code_t eval_root_hash_from_slice(const slice_t* const slice,
                                          bool* const          result) {
  bns_exit_code_t exitCode = BNS_OK;
  //check existence of parameters
  if (!slice) {
    exitCode = BNS_SLICE_NULL_ERROR;
    goto check_slice_fail;
  }
  if (!result) {
    exitCode = BNS_OUTPUT_NULL_ERROR;
    goto check_slice_fail;
  }
  //check slice
  LOG_DEBUG("check_slice() begin, " SLICE_PRINT_FORMAT,
            SLICE_TO_PRINT_ARGS(slice));
  size_t parentIndex;
  size_t index = (size_t)slice->index;
  char   parentDigest[HASH_STR_LEN];
  //for i > 1 
  for (size_t i = 0; index > 1; i += 2, index /= 2) {
    //get parentIndex
    parentIndex = i + 2 + ((index / 2) == 1 ? 0 : (index / 2)) % 2;
    //concatenate 2 hashStringLists
    char concatTwoInterNodes[(HASH_STR_LEN - 1) * 2 + 1] = {0};
    strncpy(concatTwoInterNodes, slice->hashStringList[i], HASH_STR_LEN - 1);
    strncpy(concatTwoInterNodes + HASH_STR_LEN - 1,
            slice->hashStringList[i + 1], HASH_STR_LEN - 1);
    unsigned char byteConcatString[HASH_STR_LEN] = {0};
    //convert from hex to byte
    bns_hex_to_byte(concatTwoInterNodes, (HASH_STR_LEN - 1) * 2,
                    byteConcatString);
    //allocate size
    memset(parentDigest, 0, sizeof(char) * HASH_STR_LEN);
    //encrypt
    sha256(byteConcatString, strlen(concatTwoInterNodes) / 2, parentDigest);
    //check that parentDigest and hashStringLists parent is the same
    if (bns_equals_n_ignore_case(parentDigest,
                                 slice->hashStringList[parentIndex],
                                 HASH_STR_LEN - 1) != true) {
      LOG_ERROR("check_slice() error");
      exitCode = BNS_CHECK_SLICE_ERROR;
      goto check_slice_fail;
    }
  }
  //check result as true 
  *result = true;
  LOG_DEBUG("check_slice() end");
  return exitCode;

check_slice_fail:
  if (result) { *result = false; }
  LOG_ERROR("check_clice() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//get root hash
char* slice_get_root_hash(const slice_t* const slice) {
  if (!slice) { return NULL; }
  return slice->hashStringList[slice->size - 1];
}
