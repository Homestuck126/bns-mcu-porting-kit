#include "binary_ledger_input.h"

#include <bns-client/input/ledger_input_request.h>
#include <bns-client/input/receipt_locator.h>
#include <bns-client/util/log.h>
#include <bns-client/util/string_util.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//build url for post in the form of serverUrl, Binary Ledger
void build_post_binary_ledger_input_url(char**            url,
                                        const char* const serverUrl) {
  if (!url) { return; }
  size_t size = strlen(serverUrl) + strlen(BINARY_LEDGER_INPUT_PATH);
  *url        = (char*)malloc(sizeof(char) * (size + 1));
  if (*url) { sprintf(*url, "%s%s", serverUrl, BINARY_LEDGER_INPUT_PATH); }
}
//call bns server and check ledger input
bns_exit_code_t bns_post_binary_ledger_input(
    const bns_client_t* const           bnsClient,
    const char* const                   cmdJson,
    const receipt_locator_t* const      receiptLocator,
    const binary_info_t* const          binaryInfo,
    binary_ledger_input_result_t* const binaryLedgerInputResult) {
  size_t count = 0;
bns_post_binary_ledger_input_beg : {
  bns_exit_code_t exitCode;
  char*           reqJson = NULL;
  char*           url     = NULL;
  char*           res     = NULL;
  //check that everything exists
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_post_binary_ledger_input_fail;
  }
  if (!bnsClient->config.serverUrl) {
    exitCode = BNS_CLIENT_CONFIG_SERVER_URL_NULL_ERROR;
    goto bns_post_binary_ledger_input_fail;
  }
  if (!bnsClient->httpClient.post) {
    exitCode = BNS_CLIENT_HTTP_CLIENT_BNS_POST_NULL_ERROR;
    goto bns_post_binary_ledger_input_fail;
  }
  if (!bnsClient->receiptDao.save) {
    exitCode = BNS_CLIENT_RECEIPT_DAO_SAVE_NULL_ERROR;
    goto bns_post_binary_ledger_input_fail;
  }
  if (!cmdJson) {
    exitCode = BNS_CMD_JSON_NULL_ERROR;
    goto bns_post_binary_ledger_input_fail;
  }
  if (!receiptLocator) {
    exitCode = BNS_RECEIPT_LOCATOR_NULL_ERROR;
    goto bns_post_binary_ledger_input_fail;
  }
  if (!binaryInfo) {
    exitCode = BNS_BINARY_INFO_NULL_ERROR;
    goto bns_post_binary_ledger_input_fail;
  }
  if (!binaryInfo->filename) {
    exitCode = BNS_BINARY_INFO_FILENAME_NULL_ERROR;
    goto bns_post_binary_ledger_input_fail;
  }
  if (!binaryInfo->data) {
    exitCode = BNS_BINARY_INFO_DATA_NULL_ERROR;
    goto bns_post_binary_ledger_input_fail;
  }
  if (binaryInfo->len == 0) {
    exitCode = BNS_BINARY_LEN_ZERO_ERROR;
    goto bns_post_binary_ledger_input_fail;
  }
  if (!binaryLedgerInputResult) {
    exitCode = BNS_BINARY_LEDGER_INPUT_RESULT_NULL_ERROR;
    goto bns_post_binary_ledger_input_fail;
  }
  
  LOG_INFO(
      "bns_post_binary_ledger_input() begin, "
      "cmdJson=%s, " RECEIPT_LOCATOR_PRINT_FORMAT,
      cmdJson, RECEIPT_LOCATOR_TO_PRINT_ARGS(receiptLocator));
  //built input request as json
  if ((exitCode = build_ledger_input_request_json(
           bnsClient, cmdJson, receiptLocator, &reqJson)) != BNS_OK) {
    goto bns_post_binary_ledger_input_fail;
  }
  //get ledgerInpputUrl
  build_post_binary_ledger_input_url(&url, bnsClient->config.serverUrl);
  //insert info into BNS_forms
  bns_form_t form1 = {.key         = MULTI_PART_LEDGER_INPUT_KEY,
                      .contentType = BNS_APPLICATION_JSON,
                      .filename    = MULTI_PART_LEDGER_INPUT_KEY,
                      .value       = reqJson,
                      .valueLen    = strlen(reqJson)};

  bns_form_t form2 = {.key         = MULTI_PART_BINARY_KEY,
                      .contentType = BNS_APPLICATION_OCTET_STREAM,
                      .filename    = binaryInfo->filename,
                      .value       = binaryInfo->data,
                      .valueLen    = binaryInfo->len};
  //call bnsClient with post
  res              = bnsClient->httpClient.post_multi(url, &form1, &form2);
  BNS_FREE(url);
  BNS_FREE(reqJson);
  if (!res) {
    exitCode = BNS_POST_LEDGER_INPUT_RESPONSE_NULL_ERROR;
    goto bns_post_binary_ledger_input_fail;
  }
  //parse the json and ensure that everything works 
  if ((exitCode = check_and_parse_binary_ledger_input_response(
           res, binaryLedgerInputResult)) != BNS_OK) {
    goto bns_post_binary_ledger_input_fail;
  }
  BNS_FREE(res);
  //check signature
  if ((exitCode =
           receipt_check_sig(bnsClient->bnsServerInfo.serverWalletAddress,
                             binaryLedgerInputResult->receipt)) != BNS_OK) {
    goto bns_post_binary_ledger_input_fail;
  }
  bnsClient->receiptDao.save(binaryLedgerInputResult->receipt);
  LOG_INFO(
      "bns_post_binary_ledger_input() end, " LEDGER_INPUT_RESULT_PRINT_FORMAT,
      LEDGER_INPUT_RESULT_TO_PRINT_ARGS(binaryLedgerInputResult));
  return exitCode;

bns_post_binary_ledger_input_fail:
  if (reqJson) { BNS_FREE(reqJson); }
  if (url) { BNS_FREE(url); }
  if (res) { BNS_FREE(res); }
  LOG_ERROR("bns_post_binary_ledger_input() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
            //if both Client and retry count are the same
  if (bnsClient && bnsClient->maxRetryCount) {
    //if clearanceOrder, indexValue, ClientSig, athentication or CMD error, return exit
    if (is_ledger_input_error(exitCode)) { return exitCode; }
    //if not over max retry count, retry 
    if (count++ < *bnsClient->maxRetryCount) {
      LOG_DEBUG("bns_post_binary_ledger_input() retry, count=%ld", count);
      if (bnsClient->retryDelaySec) { sleep(*bnsClient->retryDelaySec); }
      goto bns_post_binary_ledger_input_beg;
    }
  }
  return exitCode;
}
}
//go over binary Ledger Input Response and ensure content is expected 
bns_exit_code_t check_and_parse_binary_ledger_input_response(
    const char* res, binary_ledger_input_result_t* binaryLedgerInputResult) {
  LOG_DEBUG("check_and_parse_binary_ledger_input_response() begin");
  bns_exit_code_t exitCode;
  cJSON*          root = cJSON_Parse(res);
  cJSON *         status, *description;
  //get status
  status = cJSON_GetObjectItem(root, "status");
  //check existence
  if (!cJSON_IsString(status)) {
    exitCode = BNS_RESPONSE_STATUS_PARSE_ERROR;
    goto check_and_parse_binary_ledger_input_response_fail;
  }
  //reallocate space
  binaryLedgerInputResult->status =
      (char*)malloc(sizeof(char) * (strlen(status->valuestring) + 1));
  //copy status into valueString
  strcpy(binaryLedgerInputResult->status, status->valuestring);
  //deatch status from root
  cJSON_DetachItemViaPointer(root, status);
  cJSON_Delete(status);
  //check description
  description = cJSON_GetObjectItem(root, "description");
  if (!cJSON_IsString(description)) {
    exitCode = BNS_RESPONSE_DESCRIPTION_PARSE_ERROR;
    goto check_and_parse_binary_ledger_input_response_fail;
  }
  //reallocate space
  binaryLedgerInputResult->description =
      (char*)malloc(sizeof(char) * (strlen(description->valuestring) + 1));
      //copy status into valueString
  strcpy(binaryLedgerInputResult->description, description->valuestring);
  //detatch description from root 
  cJSON_DetachItemViaPointer(root, description);
  cJSON_Delete(description);
//check status not ok
  if (strcmp(BNS_STATUS_OK, binaryLedgerInputResult->status) != 0) {
    //check status warning
    if (strcmp(BNS_STATUS_WARN, binaryLedgerInputResult->status) == 0) {
      exitCode = BNS_LEDGER_INPUT_WARN;
      LOG_WARN(
          "check_and_parse_binary_ledger_input_response() "
          "warn, " BNS_EXIT_CODE_PRINT_FORMAT,
          bns_strerror(exitCode));
    } else {
      //find error
      if (strcmp(CLEARANCE_ORDER_ERROR, binaryLedgerInputResult->description) ==
          0) {
        exitCode = BNS_LEDGER_INPUT_CLEARANCE_ORDER_ERROR;
      } else if (strcmp(INDEX_VALUE_ERROR,
                        binaryLedgerInputResult->description) == 0) {
        exitCode = BNS_LEDGER_INPUT_INDEX_VALUE_ERROR;
      } else if (strcmp(CLIENT_SIGNATURE_ERROR,
                        binaryLedgerInputResult->description) == 0) {
        exitCode = BNS_LEDGER_INPUT_CLIENT_SIGNATURE_ERROR;
      } else if (strcmp(AUTHENTICATION_ERROR,
                        binaryLedgerInputResult->description) == 0) {
        exitCode = BNS_LEDGER_INPUT_AUTHENTICATION_ERROR;
      } else if (strcmp(CMD_ERROR, binaryLedgerInputResult->description) == 0) {
        exitCode = BNS_LEDGER_INPUT_CMD_ERROR;
      } else if (strcmp(TX_COUNT_ERROR, binaryLedgerInputResult->description) ==
                 0) {
        exitCode = BNS_LEDGER_INPUT_TX_COUNT_ERROR;
      } else {
        exitCode = BNS_RESPONSE_STATUS_ERROR;
      }
      goto check_and_parse_binary_ledger_input_response_fail;
    }
  }
  cJSON *receipt, *doneClearanceOrderList, *binaryFileMetadata, *binaryFileUrl;
  //set receipt to receipt from root
  receipt                          = cJSON_GetObjectItem(root, "receipt");
  binaryLedgerInputResult->receipt = (receipt_t*)malloc(sizeof(receipt_t));
  //check that receipt json is correct
  if ((exitCode = parse_receipt_from_cjson(
           receipt, binaryLedgerInputResult->receipt)) != BNS_OK) {
    goto check_and_parse_binary_ledger_input_response_fail;
  }
  //detach pointer
  cJSON_DetachItemViaPointer(root, receipt);
  cJSON_Delete(receipt);
  //check clearanceOrderList
  doneClearanceOrderList = cJSON_GetObjectItem(root, "doneClearanceOrderList");
  exitCode               = parse_done_clearance_order_list_from_cjson(
                    doneClearanceOrderList, &binaryLedgerInputResult->doneClearanceOrder);
  if (exitCode != BNS_OK) {
    goto check_and_parse_binary_ledger_input_response_fail;
  }
  //detach pointer
  cJSON_DetachItemViaPointer(root, doneClearanceOrderList);
  cJSON_Delete(doneClearanceOrderList);
  //get BinaryFileUrl and metaData and check it
  binaryFileMetadata = cJSON_GetObjectItem(root, "binaryFileMetadata");
  binaryFileUrl = cJSON_GetObjectItem(binaryFileMetadata, "BINARY_FILE_URL");
  //if binaryFileUrl is string returns false, error
  if (!cJSON_IsString(binaryFileUrl)) {
    exitCode = BNS_RESPONSE_BINARY_FILE_URL_PARSE_ERROR;
    goto check_and_parse_binary_ledger_input_response_fail;
  }
  //duplicate valueString to binaryFileUrl
  bns_strdup(&binaryLedgerInputResult->binaryFileUrl,
             binaryFileUrl->valuestring);
  //check existence of binaryFileUrl
  if (!binaryLedgerInputResult->binaryFileUrl) {
    exitCode = BNS_BINARY_FILE_URL_NULL_ERROR;
    goto check_and_parse_binary_ledger_input_response_fail;
  }
  //cleanup
  cJSON_DetachItemViaPointer(root, binaryFileMetadata);
  cJSON_Delete(binaryFileMetadata);
  cJSON_Delete(root);
  LOG_DEBUG(
      "check_and_parse_binary_ledger_input_response() "
      "end, " LEDGER_INPUT_RESULT_PRINT_FORMAT,
      LEDGER_INPUT_RESULT_TO_PRINT_ARGS(binaryLedgerInputResult));
  return exitCode;

check_and_parse_binary_ledger_input_response_fail:
  cJSON_Delete(root);
  LOG_ERROR(
      "check_and_parse_binary_ledger_input_response() "
      "error, " BNS_EXIT_CODE_PRINT_FORMAT,
      bns_strerror(exitCode));
  return exitCode;
}
//free binaryLedgerInputResult
void binary_ledger_input_result_free(
    binary_ledger_input_result_t* const binaryLedgerInputResult) {
      // free binary ledger input
  if (binaryLedgerInputResult) {
    if (binaryLedgerInputResult->status) {
      BNS_FREE(binaryLedgerInputResult->status);
    }
    if (binaryLedgerInputResult->description) {
      BNS_FREE(binaryLedgerInputResult->description);
    }
    if (binaryLedgerInputResult->receipt) {
      BNS_FREE(binaryLedgerInputResult->receipt);
    }
    if (binaryLedgerInputResult->binaryFileUrl) {
      BNS_FREE(binaryLedgerInputResult->binaryFileUrl);
    }
  }
}
