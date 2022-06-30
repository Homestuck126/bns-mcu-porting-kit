#ifndef BNS_C_CLIENT_RECEIPT_H
#define BNS_C_CLIENT_RECEIPT_H

#include <bns-client/core/core.h>
#include <cJSON.h>

#if defined(RECEIPT_TIMESTAMP_IS_LONG)
#define RECEIPT_PRINT_FORMAT                                         \
  "receipt=receipt_t(callerAddress=%s, timestamp=%lld, cmd=%s, "     \
  "indexValue=%s, "                                                  \
  "metadata=%s, clearanceOrder=%lld, timestampSPO=%lld, result=%s, " \
  "sigClient=sig_t(r=%s, s=%s, v=%s), sigServer=sig_t(r=%s, s=%s, v=%s))"
#else
#define RECEIPT_PRINT_FORMAT                                                  \
  "receipt=receipt_t(callerAddress=%s, timestamp=%s, cmd=%s, indexValue=%s, " \
  "metadata=%s, clearanceOrder=%lld, timestampSPO=%s, result=%s, "            \
  "sigClient=sig_t(r=%s, s=%s, v=%s), sigServer=sig_t(r=%s, s=%s, v=%s))"
#endif
#define RECEIPT_TO_PRINT_ARGS(receipt)                                        \
  receipt->callerAddress, (receipt)->timestamp, (receipt)->cmd,               \
      (receipt)->indexValue, (receipt)->metadata, (receipt)->clearanceOrder,  \
      (receipt)->timestampSPO, (receipt)->result, (receipt)->sigClient.r,     \
      (receipt)->sigClient.s, (receipt)->sigClient.v, (receipt)->sigServer.r, \
      (receipt)->sigServer.s, (receipt)->sigServer.v
//makes receipts clearanceORder and indexValue into 1 string
_CHECK_RESULT
bns_exit_code_t receipt_first_part_to_sign_data(const receipt_t* receipt,
                                                char**           firstPart);
//Combine CallerAddress, timetamp, cmd, metadata,timestampSPO, result, signature together
_CHECK_RESULT
bns_exit_code_t receipt_second_part_hash_to_sign_data(const receipt_t* receipt,
                                                      char** secondPartHash);
//combine both first and second part of toSignData info
_CHECK_RESULT
bns_exit_code_t receipt_to_sign_data(const receipt_t* receipt,
                                     char**           toSignData);
//parses receipt and checks for errors while saving everything in receipt
_CHECK_RESULT
bns_exit_code_t parse_receipt_from_cjson(cJSON* root, receipt_t* receipt);
//check signature in receipt using address
_CHECK_RESULT
bns_exit_code_t receipt_check_sig(const char*      serverWalletAddress,
                                  const receipt_t* receipt);
//get digest value which is a hash of the data in the buffer which is from receipt
_CHECK_RESULT
bns_exit_code_t receipt_to_digest_value(const receipt_t* receipt,
                                        char**           digestValue);

#endif  // BNS_C_CLIENT_RECEIPT_H
