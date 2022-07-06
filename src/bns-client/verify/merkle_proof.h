#ifndef BNS_C_CLIENT_MERKLE_PROOF_H
#define BNS_C_CLIENT_MERKLE_PROOF_H

#include <bns-client/core/core.h>
#include <cJSON.h>

#define LEDGER_VERIFY_MERKLE_PROOF "/verify/merkleProof/"
//get command string and url for getMerklePRoof
void build_get_merkle_proof_url(char**                   url,
                                const char*              serverUrl,
                                const receipt_locator_t* receiptLocator);
//call getMerkleProof from bnsClient
_CHECK_RESULT
bns_exit_code_t bns_get_merkle_proof(const bns_client_t*      bnsClient,
                                     const receipt_locator_t* receiptLocator,
                                     merkle_proof_t*          merkleProof);
//copies all the information from pbPair into pbPairValue
_CHECK_RESULT
bns_exit_code_t parse_pb_pair(cJSON* root, pb_pair_t* pbPair);
//forms json object for merkleProof
_CHECK_RESULT
bns_exit_code_t check_and_parse_merkle_proof_response(
    const char* res, merkle_proof_t* merkleProof);
// free data of pb_pair
void pb_pair_free(pb_pair_t* pbPair);
//free data of merkle proof
void merkle_proof_free(merkle_proof_t* merkleProof);
//prints merkle_Proof
void merkle_proof_print(const merkle_proof_t* merkleProof);
//Get toSignData from merkleProof which is just pbPair and clearanceOrder
_CHECK_RESULT
bns_exit_code_t merkle_proof_to_sign_data(const merkle_proof_t* merkleProof,
                                          char**                toSignData);
//check hash of receipt is the same as pbPair value is the same for the receipt 
_CHECK_RESULT
bns_exit_code_t check_receipt_in_pbpair(const receipt_t* receipt,
                                        const pb_pair_t* pbPair);

#endif  // BNS_C_CLIENT_MERKLE_PROOF_H
