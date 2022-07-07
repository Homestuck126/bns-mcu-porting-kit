#include <bns-client/input/ledger_input_request.h>
#include <bns-client/input/receipt_locator.h>
#include <bns-client/util/log.h>
#include <bns-client/util/numeric_util.h>
#include <bns-client/util/signature_util.h>
#include <bns-client/util/string_util.h>
#include <bns-client/util/time_util.h>
#include <cJSON.h>
#include <stdio.h>
#include <string.h>
//built Ledger Input Request
bns_exit_code_t build_ledger_input_request_json(
    const bns_client_t*      bnsClient,
    const char*              cmdJson,
    const receipt_locator_t* receiptLocator,
    char**                   reqJson) {
  LOG_DEBUG("build_ledger_input_request_json() begin");
  ledger_input_request_t ledgerInputRequest = {0};
  bns_exit_code_t        exitCode;
  //check parameters
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto build_ledger_input_request_json_fail;
  }
  if (!cmdJson) {
    exitCode = BNS_CMD_JSON_NULL_ERROR;
    goto build_ledger_input_request_json_fail;
  }
  if (!receiptLocator) {
    exitCode = BNS_RECEIPT_LOCATOR_NULL_ERROR;
    goto build_ledger_input_request_json_fail;
  }
  if (!reqJson) {
    exitCode = BNS_LEDGER_INPUT_REQUEST_JSON_NULL_ERROR;
    goto build_ledger_input_request_json_fail;
  }
  //build ledger input request and check if it worked 
  if ((exitCode = build_ledger_input_request(bnsClient, cmdJson, receiptLocator,
                                             &ledgerInputRequest)) != BNS_OK) {
    goto build_ledger_input_request_json_fail;
  }
  
  if ((exitCode = ledger_input_request_sign(
           &ledgerInputRequest, bnsClient->config.privateKey)) != BNS_OK) {
    goto build_ledger_input_request_json_fail;
  }
  if (bnsClient->callback.create_ledger_input_by_cmd) {
    bnsClient->callback.create_ledger_input_by_cmd(receiptLocator,
                                                   &ledgerInputRequest);
  }
  if ((exitCode = ledger_input_request_to_json(&ledgerInputRequest, reqJson)) !=
      BNS_OK) {
    goto build_ledger_input_request_json_fail;
  }
  //cleanup
  ledger_input_request_free(&ledgerInputRequest);
  LOG_DEBUG("build_ledger_input_request_json() end");
  return exitCode;

build_ledger_input_request_json_fail:
  ledger_input_request_free(&ledgerInputRequest);
  LOG_ERROR(
      "build_ledger_input_request_json() error, " BNS_EXIT_CODE_PRINT_FORMAT,
      bns_strerror(exitCode));
  return exitCode;
}
//built ledger Input Request 
bns_exit_code_t build_ledger_input_request(
    const bns_client_t* const      bnsClient,
    const char* const              cmdJson,
    const receipt_locator_t* const receiptLocator,
    ledger_input_request_t* const  ledgerInputRequest) {
  bns_exit_code_t exitCode = BNS_OK;
  // check existence
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto build_ledger_input_request_fail;
  }
  if (!cmdJson) {
    exitCode = BNS_CMD_JSON_NULL_ERROR;
    goto build_ledger_input_request_fail;
  }
  if (!receiptLocator) {
    exitCode = BNS_RECEIPT_LOCATOR_NULL_ERROR;
    goto build_ledger_input_request_fail;
  }
  if (!ledgerInputRequest) {
    exitCode = BNS_LEDGER_INPUT_REQUEST_NULL_ERROR;
    goto build_ledger_input_request_fail;
  }
  LOG_DEBUG(
      "build_ledger_input_request() begin, "
      "cmdJson=%s, " RECEIPT_LOCATOR_PRINT_FORMAT,
      cmdJson, RECEIPT_LOCATOR_TO_PRINT_ARGS(receiptLocator));
  //copy wallet address into caller address
  strcpy(ledgerInputRequest->callerAddress, bnsClient->walletAddress);
#if defined(RECEIPT_TIMESTAMP_IS_LONG)
  ledgerInputRequest->timestamp = get_timestamp();
#else
  ledgerInputRequest->timestamp = get_timestamp_string();
#endif
  //duplicate right to left 
  bns_strdup(&ledgerInputRequest->cmd, cmdJson);
  bns_strdup(&ledgerInputRequest->indexValue, receiptLocator->indexValue);
  bns_strdup(&ledgerInputRequest->metadata, "");
  //put receipt locator clearance order into ledgerinpuRequest clearance Order
  ledgerInputRequest->clearanceOrder = receiptLocator->clearanceOrder;
  LOG_DEBUG(
      "build_ledger_input_request() end, " LEDGER_INPUT_REQUEST_PRINT_FORMAT,
      LEDGER_INPUT_REQUEST_TO_PRINT_ARGS(ledgerInputRequest));
  return exitCode;

build_ledger_input_request_fail:
  ledger_input_request_free(ledgerInputRequest);
  LOG_ERROR("build_ledger_input_request() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//create ledger input request signature
bns_exit_code_t ledger_input_request_sign(
    ledger_input_request_t* const ledgerInputRequest,
    const char* const             privateKey) {
  bns_exit_code_t exitCode;
  char*           buffer = NULL;
  //check exsistence 
  if (!ledgerInputRequest || !ledgerInputRequest->cmd ||
      !ledgerInputRequest->indexValue || !ledgerInputRequest->metadata) {
    exitCode = BNS_LEDGER_INPUT_REQUEST_NULL_ERROR;
    goto ledger_input_request_sign_fail;
  }
  if (!privateKey) {
    exitCode = BNS_PRIVATE_KEY_NULL_ERROR;
    goto ledger_input_request_sign_fail;
  }
  LOG_DEBUG(
      "ledger_input_request_sign() begin, " LEDGER_INPUT_REQUEST_PRINT_FORMAT,
      LEDGER_INPUT_REQUEST_TO_PRINT_ARGS(ledgerInputRequest));
  //get size of LegerInput
  size_t size = 0;
  size += strlen(ledgerInputRequest->callerAddress);
#if defined(RECEIPT_TIMESTAMP_IS_LONG)
  size += bns_digits(ledgerInputRequest->timestamp);
#else
  size += strlen(ledgerInputRequest->timestamp);
#endif
  size += strlen(ledgerInputRequest->cmd);
  size += strlen(ledgerInputRequest->indexValue);
  size += strlen(ledgerInputRequest->metadata);
  size += bns_digits(ledgerInputRequest->clearanceOrder);
  buffer = (char*)malloc(sizeof(char) * (size + 1));
  //put ledgerInput as string into buffer
#if defined(RECEIPT_TIMESTAMP_IS_LONG)
  sprintf(buffer, "%s%lld%s%s%s%lld", ledgerInputRequest->callerAddress,
          ledgerInputRequest->timestamp, ledgerInputRequest->cmd,
          ledgerInputRequest->indexValue, ledgerInputRequest->metadata,
          ledgerInputRequest->clearanceOrder);
#else
  sprintf(buffer, "%s%s%s%s%s%lld", ledgerInputRequest->callerAddress,
          ledgerInputRequest->timestamp, ledgerInputRequest->cmd,
          ledgerInputRequest->indexValue, ledgerInputRequest->metadata,
          ledgerInputRequest->clearanceOrder);
#endif
  // initalize result 
  unsigned char shaResult[SHA3_BYTE_LEN] = {0};
  //encrypted buffer 
  bns_sha3_prefix((unsigned char*)buffer, size, shaResult);
  BNS_FREE(buffer);
  //create signature 
  if ((exitCode = bns_sign(shaResult, privateKey,
                           &ledgerInputRequest->sigClient)) != BNS_OK) {
    goto ledger_input_request_sign_fail;
  }
  LOG_DEBUG(
      "ledger_input_request_sign() end, " LEDGER_INPUT_REQUEST_PRINT_FORMAT,
      LEDGER_INPUT_REQUEST_TO_PRINT_ARGS(ledgerInputRequest));
  return exitCode;

ledger_input_request_sign_fail:
  LOG_ERROR("ledger_input_request_sign() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//get Ledger Input Request as a string
bns_exit_code_t ledger_input_request_to_json(
    const ledger_input_request_t* const ledgerInputRequest, char** const json) {
  LOG_DEBUG("ledger_input_request_to_json() begin");
  bns_exit_code_t exitCode = BNS_OK;
  cJSON*          root     = NULL;
  //check existence
  if (!ledgerInputRequest || !ledgerInputRequest->cmd ||
      !ledgerInputRequest->indexValue || !ledgerInputRequest->metadata) {
    exitCode = BNS_LEDGER_INPUT_REQUEST_NULL_ERROR;
    goto ledger_input_request_to_json_fail;
  }
  if (!json) {
    exitCode = BNS_LEDGER_INPUT_REQUEST_JSON_NULL_ERROR;
    goto ledger_input_request_to_json_fail;
  }
  //create root and signature as JSON Objects
  root       = cJSON_CreateObject();
  cJSON* sig = cJSON_CreateObject();
  //add items to the cJson object
  cJSON_AddItemToObject(root, "callerAddress",
                        cJSON_CreateString(ledgerInputRequest->callerAddress));
#if defined(RECEIPT_TIMESTAMP_IS_LONG)
  cJSON_AddItemToObject(root, "timestamp",
                        cJSON_CreateNumber(ledgerInputRequest->timestamp,
                                           ledgerInputRequest->timestamp));
#else
  cJSON_AddItemToObject(root, "timestamp",
                        cJSON_CreateRaw(ledgerInputRequest->timestamp));
#endif
  cJSON_AddItemToObject(root, "cmd",
                        cJSON_CreateString(ledgerInputRequest->cmd));
  cJSON_AddItemToObject(root, "indexValue",
                        cJSON_CreateString(ledgerInputRequest->indexValue));
  cJSON_AddItemToObject(root, "metadata",
                        cJSON_CreateString(ledgerInputRequest->metadata));
  cJSON_AddItemToObject(
      root, "clearanceOrder",
      cJSON_CreateNumber((double)ledgerInputRequest->clearanceOrder,
                         ledgerInputRequest->clearanceOrder));
  cJSON_AddItemToObject(root, "sigClient", sig);
  cJSON_AddItemToObject(sig, "r",
                        cJSON_CreateString(ledgerInputRequest->sigClient.r));
  cJSON_AddItemToObject(sig, "s",
                        cJSON_CreateString(ledgerInputRequest->sigClient.s));
  cJSON_AddItemToObject(sig, "v",
                        cJSON_CreateString(ledgerInputRequest->sigClient.v));
  if (*json) { BNS_FREE(*json); }
  //turn root into string
  *json = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  LOG_DEBUG("ledger_input_request_to_json() end, json=%s", *json);
  return exitCode;

ledger_input_request_to_json_fail:
  cJSON_Delete(root);
  LOG_ERROR("ledger_input_request_to_json() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//free LedgerInputRequest data
void ledger_input_request_free(
    ledger_input_request_t* const ledgerInputRequest) {
  if (ledgerInputRequest) {
#if !defined(RECEIPT_TIMESTAMP_IS_LONG)
    if (ledgerInputRequest->timestamp) {
      BNS_FREE(ledgerInputRequest->timestamp);
    }
#endif
    if (ledgerInputRequest->cmd) { BNS_FREE(ledgerInputRequest->cmd); }
    if (ledgerInputRequest->indexValue) {
      BNS_FREE(ledgerInputRequest->indexValue);
    }
    if (ledgerInputRequest->metadata) {
      BNS_FREE(ledgerInputRequest->metadata);
    }
  }
}
