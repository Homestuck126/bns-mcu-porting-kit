#include <bns-client/contract/contract.h>
#include <bns-client/input/receipt.h>
#include <bns-client/input/receipt_locator.h>
#include <bns-client/util/log.h>
#include <bns-client/util/signature_util.h>
#include <bns-client/util/slice_validation_util.h>
#include <bns-client/util/string_util.h>
#include <bns-client/util/time_util.h>
#include <bns-client/verify/merkle_proof.h>
#include <bns-client/verify/verify.h>
#include <stdlib.h>
//verifies data works
bns_exit_code_t verify(const bns_client_t* const      bnsClient,
                       const receipt_t* const         receipt,
                       merkle_proof_t* const          merkleProof,
                       verify_receipt_result_t* const verifyReceiptResult) {
  bns_exit_code_t   exitCode;
  receipt_locator_t receiptLocator = {0};
  //check existence 
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto verify_fail;
  }
  if (!receipt) {
    exitCode = BNS_RECEIPT_NULL_ERROR;
    goto verify_fail;
  }
  if (!merkleProof) {
    exitCode = BNS_MERKLE_PROOF_NULL_ERROR;
    goto verify_fail;
  }
  if (!verifyReceiptResult) {
    exitCode = BNS_VERIFY_RECEIPT_RESULT_NULL_ERROR;
    goto verify_fail;
  }
  //save clearanceOrder in verifyReceipt result
  LOG_INFO("verify() begin, " RECEIPT_PRINT_FORMAT,
           RECEIPT_TO_PRINT_ARGS(receipt));
  verifyReceiptResult->clearanceOrder = receipt->clearanceOrder;
  bns_strdup(&verifyReceiptResult->indexValue, receipt->indexValue);
  verifyReceiptResult->pass = false;
#if defined(RECEIPT_TIMESTAMP_IS_LONG)
  verifyReceiptResult->timestamp = get_timestamp_string();
#else
  verifyReceiptResult->timestamp = get_timestamp_string();
#endif
  verifyReceiptResult->receiptSignatureOk        = false;
  verifyReceiptResult->merkleproofSignatureOk    = false;
  verifyReceiptResult->clearanceOrderOk          = false;
  verifyReceiptResult->pbPairOk                  = false;
  verifyReceiptResult->sliceOk                   = false;
  verifyReceiptResult->clearanceRecordRootHashOk = false;
  //save clearanceOrder in receipt locator
  receiptLocator.clearanceOrder = receipt->clearanceOrder;
  bns_strdup(&receiptLocator.indexValue, receipt->indexValue);
  exitCode = bns_get_merkle_proof(bnsClient, &receiptLocator, merkleProof);
  if (exitCode != BNS_OK) { goto verify_fail; }
  receipt_locator_free(&receiptLocator);
  clearance_record_t clearanceRecord = {0};
  exitCode = contract_post_clearance_record(bnsClient, receipt->clearanceOrder,
                                            &clearanceRecord);
  if (exitCode != BNS_OK) { goto verify_fail; }

  // verify receipt signature
  exitCode = verify_receipt_signature(
      bnsClient->bnsServerInfo.serverWalletAddress, receipt,
      &verifyReceiptResult->receiptSignatureOk);
  if (exitCode != BNS_OK) { goto verify_fail; }

  // verify merkleProof signature
  exitCode = verify_merkle_proof_signature(
      bnsClient->bnsServerInfo.serverWalletAddress, merkleProof,
      &verifyReceiptResult->merkleproofSignatureOk);
  if (exitCode != BNS_OK) { goto verify_fail; }

  // verify clearanceOrder
  exitCode = verify_clearance_order(receipt, merkleProof, &clearanceRecord,
                                    &verifyReceiptResult->clearanceOrderOk);
  if (exitCode != BNS_OK) { goto verify_fail; }

  // verify PbPair
  exitCode =
      verify_pb_pair(receipt, merkleProof, &verifyReceiptResult->pbPairOk);
  if (exitCode != BNS_OK) { goto verify_fail; }

  // verify slice
  exitCode = verify_merkle_proof_slice(&merkleProof->slice,
                                       &verifyReceiptResult->sliceOk);
  if (exitCode != BNS_OK) { goto verify_fail; }

  // verify if clearanceRecord rootHash and merkle proof slice rootHash match
  exitCode = verify_root_hash(merkleProof, &clearanceRecord,
                              &verifyReceiptResult->clearanceRecordRootHashOk);
  if (exitCode != BNS_OK) { goto verify_fail; }
  //set pass to true
  verifyReceiptResult->pass = true;
  //insert ok status
  bns_strdup(&verifyReceiptResult->status, BNS_STATUS_OK);
  bns_strdup(&verifyReceiptResult->description, BNS_STATUS_OK);
  LOG_INFO("verify() end, " VERIFY_RECEIPT_RESULT_PRINT_FORMAT,
           VERIFY_RECEIPT_RESULT_TO_PRINT_ARGS(verifyReceiptResult));
  return exitCode;

verify_fail:
  receipt_locator_free(&receiptLocator);
  bns_strdup(&verifyReceiptResult->status, BNS_STATUS_ERROR);
  bns_strdup(&verifyReceiptResult->description, VERIFY_FAIL_DESCRIPTION);
  LOG_ERROR("verify() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  LOG_INFO("verify() end, " VERIFY_RECEIPT_RESULT_PRINT_FORMAT,
           VERIFY_RECEIPT_RESULT_TO_PRINT_ARGS(verifyReceiptResult));
  return exitCode;
}
//Verify MerkleProof Signature 
bns_exit_code_t verify_merkle_proof_signature(
    const char* const           serverWalletAddress,
    const merkle_proof_t* const merkleProof,
    bool* const                 result) {
  bns_exit_code_t exitCode;
  char*           toSignData = NULL;
  //check existence
  if (!serverWalletAddress) {
    exitCode = BNS_SERVER_WALLET_ADDRESS_NULL_ERROR;
    goto verify_merkle_proof_signature_fail;
  }
  if (!merkleProof) {
    exitCode = BNS_MERKLE_PROOF_NULL_ERROR;
    goto verify_merkle_proof_signature_fail;
  }
  if (!result) {
    exitCode = BNS_OUTPUT_NULL_ERROR;
    goto verify_merkle_proof_signature_fail;
  }
  //to signData from merkleproof
  if ((exitCode = merkle_proof_to_sign_data(merkleProof, &toSignData)) !=
      BNS_OK) {
    goto verify_merkle_proof_signature_fail;
  }
  //Verify signature via ServerWalletAddress
  if ((exitCode = verify_signature(serverWalletAddress + 2, toSignData,
                                   &merkleProof->sigServer)) != BNS_OK) {
    goto verify_merkle_proof_signature_fail;
  }
  *result = true;
  BNS_FREE(toSignData);
  return exitCode;

verify_merkle_proof_signature_fail:
  *result = false;
  BNS_FREE(toSignData);
  LOG_ERROR(
      "verify_merkle_proof_signature() error, " BNS_EXIT_CODE_PRINT_FORMAT,
      bns_strerror(exitCode));
  return exitCode;
}
//check that receipt is correct with address
bns_exit_code_t verify_receipt_signature(const char*      serverWalletAddress,
                                         const receipt_t* receipt,
                                         bool*            result) {
  LOG_DEBUG("verify_receipt_signature() start");
  bns_exit_code_t exitCode;
  //check existence
  if (!serverWalletAddress) {
    exitCode = BNS_SERVER_WALLET_ADDRESS_NULL_ERROR;
    goto verify_receipt_signature_fail;
  }
  if (!receipt) {
    exitCode = BNS_RECEIPT_NULL_ERROR;
    goto verify_receipt_signature_fail;
  }
  if (!result) {
    exitCode = BNS_OUTPUT_NULL_ERROR;
    goto verify_receipt_signature_fail;
  }
  //check signature of receipt
  if ((exitCode = receipt_check_sig(serverWalletAddress, receipt)) != BNS_OK) {
    goto verify_receipt_signature_fail;
  }
  *result = true;
  LOG_DEBUG("verify_receipt_signature() end");
  return exitCode;
verify_receipt_signature_fail:
  *result = false;
  LOG_ERROR("verify_receipt_signature() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//verify that clearanceOrder of receipt, MerkleProof, and clearanceRecord are the same
bns_exit_code_t verify_clearance_order(
    const receipt_t* const          receipt,
    const merkle_proof_t* const     merkleProof,
    const clearance_record_t* const clearanceRecord,
    bool* const                     result) {
  LOG_DEBUG("verify_clearance_order() start");
  bns_exit_code_t exitCode = BNS_OK;
  //check existence
  if (!receipt) {
    exitCode = BNS_RECEIPT_NULL_ERROR;
    goto verify_clearanceOrderOk_fail;
  }
  if (!merkleProof) {
    exitCode = BNS_MERKLE_PROOF_NULL_ERROR;
    goto verify_clearanceOrderOk_fail;
  }
  if (!clearanceRecord) {
    exitCode = BNS_CLEARANCE_RECORD_NULL_ERROR;
    goto verify_clearanceOrderOk_fail;
  }
  if (!result) {
    exitCode = BNS_OUTPUT_NULL_ERROR;
    goto verify_clearanceOrderOk_fail;
  }
  //check clearanceOrders
  if (receipt->clearanceOrder != merkleProof->clearanceOrder) {
    exitCode = BNS_VERIFY_CLEARANCE_ORDER_ERROR;
    goto verify_clearanceOrderOk_fail;
  }
  if (merkleProof->clearanceOrder != clearanceRecord->clearanceOrder) {
    exitCode = BNS_VERIFY_CLEARANCE_RECORD_CLEARANCE_ORDER_ERROR;
    goto verify_clearanceOrderOk_fail;
  }
  *result = true;
  LOG_DEBUG("verify_clearance_order() end");
  return exitCode;

verify_clearanceOrderOk_fail:
  *result = false;
  LOG_ERROR("verify_clearance_order() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//check pbPairs are correct
bns_exit_code_t verify_pb_pair(const receipt_t* const      receipt,
                               const merkle_proof_t* const merkleProof,
                               bool* const                 result) {
  LOG_DEBUG("verify_pb_pair() start");
  bns_exit_code_t exitCode;
  //check existence
  if (!receipt) {
    exitCode = BNS_RECEIPT_NULL_ERROR;
    goto verify_pbPairOk_fail;
  }
  if (!merkleProof) {
    exitCode = BNS_MERKLE_PROOF_NULL_ERROR;
    goto verify_pbPairOk_fail;
  }
  if (!result) {
    exitCode = BNS_OUTPUT_NULL_ERROR;
    goto verify_pbPairOk_fail;
  }
  //check pbpair of receipt is same as merkleProof
  if ((exitCode = check_receipt_in_pbpair(receipt, &merkleProof->pbPair)) !=
      BNS_OK) {
    goto verify_pbPairOk_fail;
  }
  //check that the pbPair are leafnodes in the slice
  if ((exitCode = is_leaf_node(&merkleProof->slice, &merkleProof->pbPair)) !=
      BNS_OK) {
    goto verify_pbPairOk_fail;
  }
  *result = true;
  LOG_DEBUG("verify_pb_pair() end");
  return exitCode;

verify_pbPairOk_fail:
  *result = false;
  LOG_ERROR("verify_pb_pair() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//verify root hash of slice and merkle proof
bns_exit_code_t verify_merkle_proof_slice(const slice_t* const slice,
                                          bool* const          result) {
  LOG_DEBUG("verify_merkle_proof_slice() start");
  bns_exit_code_t exitCode;
  //check existence
  if (!slice) {
    exitCode = BNS_SLICE_NULL_ERROR;
    goto verify_sliceOk_fail;
  }
  if (!result) {
    exitCode = BNS_OUTPUT_NULL_ERROR;
    goto verify_sliceOk_fail;
  }
  //check that you can get root hash of slice
  if ((exitCode = eval_root_hash_from_slice(slice, result)) != BNS_OK) {
    goto verify_sliceOk_fail;
  }
  LOG_DEBUG("verify_merkle_proof_slice() end");
  return exitCode;

verify_sliceOk_fail:
  LOG_ERROR("verify_merkle_proof_slice() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//check that root hash is the same for the slice and the ClearanceRecord
bns_exit_code_t verify_root_hash(
    const merkle_proof_t* const     merkleProof,
    const clearance_record_t* const clearanceRecord,
    bool* const                     result) {
  LOG_DEBUG("verify_root_hash() start");
  bns_exit_code_t exitCode = BNS_OK;
  //check existence 
  if (!merkleProof) {
    exitCode = BNS_MERKLE_PROOF_NULL_ERROR;
    goto verify_clearanceRecordRootHashOk_fail;
  }
  if (!clearanceRecord) {
    exitCode = BNS_CLEARANCE_RECORD_NULL_ERROR;
    goto verify_clearanceRecordRootHashOk_fail;
  }
  if (!result) {
    exitCode = BNS_OUTPUT_NULL_ERROR;
    goto verify_clearanceRecordRootHashOk_fail;
  }
  //check if slice root hash and clearanceRecord root hash are the same
  if (bns_equals_ignore_case(slice_get_root_hash(&merkleProof->slice),
                             clearanceRecord->rootHash) != true) {
    exitCode = BNS_VERIFY_CLEARANCE_RECORD_ROOT_HASH_ERROR;
    goto verify_clearanceRecordRootHashOk_fail;
  }
  *result = true;
  LOG_DEBUG("verify_root_hash() end");
  return exitCode;

verify_clearanceRecordRootHashOk_fail:
  *result = false;
  LOG_ERROR("verify_root_hash() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
