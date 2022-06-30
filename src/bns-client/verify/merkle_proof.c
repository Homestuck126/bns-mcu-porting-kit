#include <bns-client/input/receipt.h>
#include <bns-client/input/receipt_locator.h>
#include <bns-client/sha256/sha256.h>
#include <bns-client/util/log.h>
#include <bns-client/util/numeric_util.h>
#include <bns-client/util/string_util.h>
#include <bns-client/verify/merkle_proof.h>
#include <bns-client/verify/slice.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

//get command string and url for getMerklePRoof
void build_get_merkle_proof_url(
  char**                         url,
  const char* const              serverUrl,
  const receipt_locator_t* const receiptLocator) {
  if (!url) { return; }
  size_t size = strlen(serverUrl) + strlen(LEDGER_VERIFY_MERKLE_PROOF) +
                bns_digits(receiptLocator->clearanceOrder) + 1 +
                strlen(receiptLocator->indexValue);
  *url = (char*)malloc(sizeof(char) * (size + 1));
  if (*url) {
    sprintf(*url, "%s%s%lld/%s", serverUrl, LEDGER_VERIFY_MERKLE_PROOF,
            receiptLocator->clearanceOrder, receiptLocator->indexValue);
  }
}
//call getMerkleProof from bns
bns_exit_code_t bns_get_merkle_proof(
    const bns_client_t* const      bnsClient,
    const receipt_locator_t* const receiptLocator,
    merkle_proof_t* const          merkleProof) {
  size_t          count = 0;
  bns_exit_code_t exitCode;
  char*           url = NULL;
  char*           res = NULL;
bns_get_merkle_proof_beg:
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_get_merkle_proof_fail;
  }
  if (!bnsClient->config.serverUrl) {
    exitCode = BNS_CLIENT_CONFIG_SERVER_URL_NULL_ERROR;
    goto bns_get_merkle_proof_fail;
  }
  if (!bnsClient->httpClient.get) {
    exitCode = BNS_CLIENT_HTTP_CLIENT_BNS_POST_NULL_ERROR;
    goto bns_get_merkle_proof_fail;
  }
  if (!receiptLocator) {
    exitCode = BNS_RECEIPT_LOCATOR_NULL_ERROR;
    goto bns_get_merkle_proof_fail;
  }
  if (!merkleProof) {
    exitCode = BNS_MERKLE_PROOF_NULL_ERROR;
    goto bns_get_merkle_proof_fail;
  }
  LOG_INFO("bns_get_merkle_proof() begin, " RECEIPT_LOCATOR_PRINT_FORMAT,
           RECEIPT_LOCATOR_TO_PRINT_ARGS(receiptLocator));
  build_get_merkle_proof_url(&url, bnsClient->config.serverUrl, receiptLocator);

  res = bnsClient->httpClient.get(url);
  BNS_FREE(url);
  if (!res) {
    exitCode = BNS_GET_MERKLE_PROOF_RESPONSE_NULL_ERROR;
    goto bns_get_merkle_proof_fail;
  }
  exitCode = check_and_parse_merkle_proof_response(res, merkleProof);
  if (exitCode != BNS_OK) { goto bns_get_merkle_proof_fail; }
  BNS_FREE(res);
  if (bnsClient->callback.obtain_merkle_proof) {
    bnsClient->callback.obtain_merkle_proof(receiptLocator, merkleProof);
  }
  LOG_INFO("bns_get_merkle_proof() end");
#if LOG_LEVEL >= LOG_LEVEL_INFO
  merkle_proof_print(merkleProof);
#endif
  return exitCode;

bns_get_merkle_proof_fail:
  if (url) { BNS_FREE(url); }
  if (res) { BNS_FREE(res); }
  LOG_ERROR("bns_get_merkle_proof() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  if (bnsClient && bnsClient->maxRetryCount) {
    if (count++ < *bnsClient->maxRetryCount) {
      LOG_DEBUG("bns_get_merkle_proof() retry, count=%ld", count);
      if (bnsClient->retryDelaySec) { sleep(*bnsClient->retryDelaySec); }
      goto bns_get_merkle_proof_beg;
    }
  }
  return exitCode;
}
//copies all the information from pbPair into pbPairValue
bns_exit_code_t parse_pb_pair(cJSON* const root, pb_pair_t* const pbPair) {
  bns_exit_code_t exitCode = BNS_OK;
  cJSON*          temp     = NULL;
  pbPair->size             = cJSON_GetArraySize(root);
  pb_pair_value_t* pbPairValue =
      (pb_pair_value_t*)malloc(sizeof(pb_pair_value_t) * pbPair->size);
  //copies all the information from cJson Root into pbPairValue
  for (size_t i = 0; i < pbPair->size; i++) {
    cJSON* pbpContent;
    // grab cJson array item
    pbpContent = cJSON_GetArrayItem(root, (int)i);
    // grab what is in index at that location
    temp       = cJSON_GetObjectItem(pbpContent, "index");
    //if temp is not string, fail
    if (!cJSON_IsNumber(temp)) {
      exitCode = BNS_RESPONSE_PBPAIR_PARSE_ERROR;
      goto parse_pb_pair_fail;
    }

    // grabbed index of item at i
    pbPairValue[i].index = (int)temp->valueint;
    //detach phb from temp, delete temp
    cJSON_DetachItemViaPointer(pbpContent, temp);
    cJSON_Delete(temp);
    //set temp as phpContents keyHash
    temp = cJSON_GetObjectItem(pbpContent, "keyHash");
    //if temp is not string, fail
    if (!temp) {
      exitCode = BNS_RESPONSE_PBPAIR_PARSE_ERROR;
      goto parse_pb_pair_fail;
    }
    //copy keyHash into pbPairValue
    strcpy(pbPairValue[i].keyHash, temp->valuestring);
    //detach phb from temp, delete temp
    cJSON_DetachItemViaPointer(pbpContent, temp);
    cJSON_Delete(temp);
    //set temp as php content's value
    temp = cJSON_GetObjectItem(pbpContent, "value");
    //if CJson is not string, fail
    if (!cJSON_IsString(temp)) {
      exitCode = BNS_RESPONSE_PBPAIR_PARSE_ERROR;
      goto parse_pb_pair_fail;
    }
    //copy value into pbPairValue
    strcpy(pbPairValue[i].value, temp->valuestring);
    cJSON_DetachItemViaPointer(pbpContent, temp);
    cJSON_Delete(temp);
  }
  //put pbPairValue into pbPair
  pbPair->pbPairValue = pbPairValue;
  return exitCode;

parse_pb_pair_fail:
  LOG_ERROR("parse_pb_pair() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//forms json object for merkleProof
bns_exit_code_t check_and_parse_merkle_proof_response(
    const char* const res, merkle_proof_t* const merkleProof) {
  LOG_DEBUG("check_and_parse_merkle_proof_response() begin");
  bns_exit_code_t exitCode;
  cJSON*          root = NULL;
  //if merkleProof DNE, error
  if (!merkleProof) {
    exitCode = BNS_MERKLE_PROOF_NULL_ERROR;
    goto check_and_parse_merkle_proof_response_fail;
  }
  //changes res into a cJson parsable
  root          = cJSON_Parse(res);
  cJSON* status = cJSON_GetObjectItem(root, "status");
  //check if status exists, if not, error
  if (!cJSON_IsString(status)) {
    exitCode = BNS_RESPONSE_STATUS_PARSE_ERROR;
    goto check_and_parse_merkle_proof_response_fail;
  }
  //check if BNS_STATUS_OK is the same as status's value string, if not error
  if (strcmp(BNS_STATUS_OK, status->valuestring) != 0) {
    exitCode = BNS_RESPONSE_STATUS_ERROR;
    goto check_and_parse_merkle_proof_response_fail;
  }
  //detach status from root, delete status
  cJSON_DetachItemViaPointer(root, status);
  cJSON_Delete(status);
  //Get MerkleProof cJSON from the cJSON root
  cJSON* merkleProofCJSON = cJSON_GetObjectItem(root, "merkleProof");
  cJSON* temp;
  cJSON* pbPair;
  //grab the slice from the MerkleProof
  temp = cJSON_GetObjectItem(merkleProofCJSON, "slice");
  //if temp is not a string, error
  if (!cJSON_IsString(temp)) {
    exitCode = BNS_RESPONSE_SLICE_PARSE_ERROR;
    goto check_and_parse_merkle_proof_response_fail;
  }
  //parse through slice and ensure no errors
  if ((exitCode = parse_slice(temp->valuestring, &merkleProof->slice)) !=
      BNS_OK) {
    goto check_and_parse_merkle_proof_response_fail;
  }
  //detach temp from MerkleJson
  cJSON_DetachItemViaPointer(merkleProofCJSON, temp);
  cJSON_Delete(temp);
  //Get pbPair from MerkleProofCJSON
  pbPair = cJSON_GetObjectItem(merkleProofCJSON, "pbPair");
  if ((exitCode = parse_pb_pair(pbPair, &merkleProof->pbPair)) != BNS_OK) {
    goto check_and_parse_merkle_proof_response_fail;
  }
  //clean pbPair
  cJSON_DetachItemViaPointer(merkleProofCJSON, pbPair);
  cJSON_Delete(pbPair);
  //Get ClearanceOrder
  temp = cJSON_GetObjectItem(merkleProofCJSON, "clearanceOrder");
  //check ClearanceOrder is a number
  if (!cJSON_IsNumber(temp)) {
    exitCode = BNS_JSON_PARSE_ERROR;
    goto check_and_parse_merkle_proof_response_fail;
  }
  //Set CleranceOrder of merkle proof 
  merkleProof->clearanceOrder = (long int)temp->valuedouble;
  //cleanup
  cJSON_DetachItemViaPointer(merkleProofCJSON, temp);
  cJSON_Delete(temp);
  //grab r from signature and sigServer from merkleProof
  cJSON* sig;
  sig  = cJSON_GetObjectItem(merkleProofCJSON, "sigServer");
  temp = cJSON_GetObjectItem(sig, "r");
  //checks if sig is string , else error
  if (!cJSON_IsString(temp)) {
    exitCode = BNS_RESPONSE_SIG_SERVER_PARSE_ERROR;
    goto check_and_parse_merkle_proof_response_fail;
  }
  //copy signature into merkle proof
  strcpy(merkleProof->sigServer.r, temp->valuestring);
  cJSON_DetachItemViaPointer(sig, temp);
  cJSON_Delete(temp);

  temp = cJSON_GetObjectItem(sig, "s");
  if (!cJSON_IsString(temp)) {
    exitCode = BNS_RESPONSE_SIG_SERVER_PARSE_ERROR;
    goto check_and_parse_merkle_proof_response_fail;
  }
  strcpy(merkleProof->sigServer.s, temp->valuestring);
  cJSON_DetachItemViaPointer(sig, temp);
  cJSON_Delete(temp);

  temp = cJSON_GetObjectItem(sig, "v");
  if (!cJSON_IsString(temp)) {
    exitCode = BNS_RESPONSE_SIG_SERVER_PARSE_ERROR;
    goto check_and_parse_merkle_proof_response_fail;
  }

  strcpy(merkleProof->sigServer.v, temp->valuestring);
  cJSON_DetachItemViaPointer(sig, temp);
  cJSON_Delete(temp);
  cJSON_DetachItemViaPointer(root, merkleProofCJSON);
  cJSON_Delete(merkleProofCJSON);

  cJSON_Delete(root);
  LOG_DEBUG("check_and_parse_merkle_proof_response() end");
  return exitCode;

check_and_parse_merkle_proof_response_fail:
  cJSON_Delete(root);
  LOG_ERROR("check_merkle_proof_response() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
// free data of pb_pair
void pb_pair_free(pb_pair_t* const pbPair) {
  if (pbPair) {
    if (pbPair->pbPairValue) { BNS_FREE(pbPair->pbPairValue); }
  }
}
//free data of merkle proof
void merkle_proof_free(merkle_proof_t* const merkleProof) {
  if (merkleProof) {
    pb_pair_free(&merkleProof->pbPair);
    slice_free(&merkleProof->slice);
  }
}
//prints merkle_Proof
void merkle_proof_print(const merkle_proof_t* const merkleProof) {
  if (!merkleProof) { 
    return; 
    }
  //log_backend is printf
  LOG_BACKEND("merkleProof=merkle_proof_t(slice=%lld.%s",
              merkleProof->slice.index, merkleProof->slice.hashString);
  LOG_BACKEND(", pbPair=[");
  //
  for (size_t i = 0; i < merkleProof->pbPair.size; i++) {
    if (i != 0) { LOG_BACKEND(", "); }
    LOG_BACKEND("pb_pair_t(index=%lld, keyHash=%s, value=%s)",
                merkleProof->pbPair.pbPairValue[i].index,
                merkleProof->pbPair.pbPairValue[i].keyHash,
                merkleProof->pbPair.pbPairValue[i].value);
  }
  LOG_BACKEND("]");
  LOG_BACKEND(", clearanceOrder=%lld", merkleProof->clearanceOrder);
  LOG_BACKEND(", sigServer=sig_t(r=%s, s=%s, v=%s)", merkleProof->sigServer.r,
              merkleProof->sigServer.s, merkleProof->sigServer.v);
  LOG_BACKEND(")\n");
}
//Get toSignData from merkleProof which is just pbPair and clearanceOrder
bns_exit_code_t merkle_proof_to_sign_data(
    const merkle_proof_t* const merkleProof, char** const toSignData) {
  LOG_DEBUG("merkle_proof_to_sign_data() begin");
  bns_exit_code_t exitCode    = BNS_OK;
  char*           sliceString = NULL;
  //
  if (!merkleProof) {
    exitCode = BNS_MERKLE_PROOF_NULL_ERROR;
    goto merkle_proof_to_sign_data_fail;
  }
  if (!toSignData) {
    exitCode = BNS_MERKLE_PROOF_TO_SIGN_DATA_NULL_ERROR;
    goto merkle_proof_to_sign_data_fail;
  }
  
  size_t size = 0;
  //turn slice into string
  slice_to_string(&merkleProof->slice, &sliceString);
  //get length of merkleProofpairs and slice
  size_t sliceStringLen = strlen(sliceString);
  size += sliceStringLen;
  for (size_t i = 0; i < merkleProof->pbPair.size; i++) {
    size += bns_digits(merkleProof->pbPair.pbPairValue[i].index);
    size += strlen(merkleProof->pbPair.pbPairValue[i].keyHash);
    size += strlen(merkleProof->pbPair.pbPairValue[i].value);
  }
  //get clearanceOrder digits
  size += bns_digits(merkleProof->clearanceOrder);
  //allocate space for toSignData
  if (*toSignData) {
    *toSignData = (char*)realloc(*toSignData, sizeof(char) * (size + 1));
  } else {
    *toSignData = (char*)malloc(sizeof(char) * (size + 1));
  }
  //cpy sliceString to toSignData
  strcpy(*toSignData, sliceString);
  //
  for (size_t i = 0; i < merkleProof->pbPair.size; i++) {
    sprintf((*toSignData) + strlen(*toSignData), "%lld%s%s",
            merkleProof->pbPair.pbPairValue[i].index,
            merkleProof->pbPair.pbPairValue[i].keyHash,
            merkleProof->pbPair.pbPairValue[i].value);
  }
  // formats it to have clearanceOrder
  sprintf((*toSignData) + strlen(*toSignData), "%lld",
          merkleProof->clearanceOrder);
  BNS_FREE(sliceString);
  LOG_DEBUG("merkle_proof_to_sign_data() end");
  return exitCode;

merkle_proof_to_sign_data_fail:
  LOG_ERROR("merkle_proof_to_sign_data() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//check hash of receipt is the same as pbPair value is the same for the receipt 
bns_exit_code_t check_receipt_in_pbpair(const receipt_t* const receipt,
                                        const pb_pair_t* const pbPair) {
  bns_exit_code_t exitCode;
  char*           digestValue = NULL;
  //check receipt and pair exist
  if (!receipt) {
    exitCode = BNS_RECEIPT_NULL_ERROR;
    goto check_receipt_in_pbpair_fail;
  }
  if (!pbPair) {
    exitCode = BNS_PB_PAIR_NULL_ERROR;
    goto check_receipt_in_pbpair_fail;
  }
  LOG_DEBUG("check_receipt_in_pbpair() begin, " RECEIPT_PRINT_FORMAT,
            RECEIPT_TO_PRINT_ARGS(receipt));
  //go through entire pbPair
  for (size_t position = 0; position < pbPair->size; position++) {
    char indexValueHash[HASH_STR_LEN] = {0};
    //Get encrypted indexValue
    sha256((unsigned char*)receipt->indexValue, strlen(receipt->indexValue),
           indexValueHash);
    // if the pbPairs Hash and the encrypted indexValue is the same
    if (bns_equals_ignore_case(indexValueHash,
                               pbPair->pbPairValue[position].keyHash) == true) {
      //get digest value which is a hash of data from receipt
      if ((exitCode = receipt_to_digest_value(receipt, &digestValue)) !=
          BNS_OK) {
        goto check_receipt_in_pbpair_fail;
      }
      //check that digestvalue is the same as one of the values of pbPair
      if (bns_equals_ignore_case(digestValue,
                                 pbPair->pbPairValue[position].value) == true) {
        BNS_FREE(digestValue);
        LOG_DEBUG("check_receipt_in_pbpair() end");
        return exitCode;
      }
      exitCode = BNS_CHECK_RECEIPT_IN_PBPAIR_ERROR;
      goto check_receipt_in_pbpair_fail;
    }
  }
  exitCode = BNS_CHECK_RECEIPT_IN_PBPAIR_ERROR;

check_receipt_in_pbpair_fail:
  if (digestValue) { BNS_FREE(digestValue); }
  LOG_ERROR("check_receipt_in_pbpair() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
