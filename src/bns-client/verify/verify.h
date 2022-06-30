#ifndef BNS_C_CLIENT_VERIFY_H
#define BNS_C_CLIENT_VERIFY_H

#include <bns-client/core/core.h>

#define VERIFY_RECEIPT_RESULT_PRINT_FORMAT                                   \
  "verifyReceiptResult=verify_receipt_result_t(clearanceOrder=%lld, "        \
  "indexValue=%s, pass=%s, status=%s, timestamp=%s, receiptSignatureOk=%s, " \
  "merkleproofSignatureOk=%s, "                                              \
  "clearanceOrderOk=%s, pbPairOk=%s, sliceOk=%s, "                           \
  "clearanceRecordRootHashOk=%s)"
#define VERIFY_RECEIPT_RESULT_TO_PRINT_ARGS(verifyReceiptResult)          \
  verifyReceiptResult->clearanceOrder, (verifyReceiptResult)->indexValue, \
      (verifyReceiptResult)->pass ? "true" : "false",                     \
      (verifyReceiptResult)->status, (verifyReceiptResult)->timestamp,    \
      (verifyReceiptResult)->receiptSignatureOk ? "true" : "false",       \
      (verifyReceiptResult)->merkleproofSignatureOk ? "true" : "false",   \
      (verifyReceiptResult)->clearanceOrderOk ? "true" : "false",         \
      (verifyReceiptResult)->pbPairOk ? "true" : "false",                 \
      (verifyReceiptResult)->sliceOk ? "true" : "false",                  \
      (verifyReceiptResult)->clearanceRecordRootHashOk ? "true" : "false"

#define VERIFY_FAIL_DESCRIPTION "verify fail"
_CHECK_RESULT
//verifies data works
bns_exit_code_t verify(const bns_client_t*      bnsClient,
                       const receipt_t*         receipt,
                       merkle_proof_t*          merkleProof,
                       verify_receipt_result_t* verifyReceiptResult);
//Verify MerkleProof Signature 
_CHECK_RESULT
bns_exit_code_t verify_merkle_proof_signature(const char* serverWalletAddress,
                                              const merkle_proof_t* merkleProof,
                                              bool*                 result);
//check that receipt is correct with address
_CHECK_RESULT
bns_exit_code_t verify_receipt_signature(const char*      serverWalletAddress,
                                         const receipt_t* receipt,
                                         bool*            result);
//verify that clearanceOrder of receipt, MerkleProof, and clearanceRecord are the same
_CHECK_RESULT
bns_exit_code_t verify_clearance_order(
    const receipt_t*          receipt,
    const merkle_proof_t*     merkleProof,
    const clearance_record_t* clearanceRecord,
    bool*                     result);
//check pbPairs are correct
_CHECK_RESULT
bns_exit_code_t verify_pb_pair(const receipt_t*      receipt,
                               const merkle_proof_t* merkleProof,
                               bool*                 result);
//verify root hash of slice and merkle proof
_CHECK_RESULT
bns_exit_code_t verify_merkle_proof_slice(const slice_t* slice, bool* result);
//check that root hash is the same for the slice and the ClearanceRecord
_CHECK_RESULT
bns_exit_code_t verify_root_hash(const merkle_proof_t*     merkleProof,
                                 const clearance_record_t* clearanceRecord,
                                 bool*                     result);

#endif  // BNS_C_CLIENT_VERIFY_H
