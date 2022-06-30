#include <bns-client/input/ledger_input.h>
#include <bns-client/input/ledger_input_request.h>
#include <bns-client/input/receipt_locator.h>
#include <bns-client/util/log.h>
#include <bns-client/util/string_util.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//get url for post ledgerInputUrl
void build_post_ledger_input_url(char** url, const char* const serverUrl) {
  if (!url) { return; }
  //get size of serverUrl and "\input"
  size_t size = strlen(serverUrl) + strlen(LEDGER_INPUT_PATH);
  //allocate size of server
  *url        = (char*)malloc(sizeof(char) * (size + 1));
  //get string of serverUrl and "\input"
  if (*url) { sprintf(*url, "%s%s", serverUrl, LEDGER_INPUT_PATH); }
}
//call bns for post with ledgerInput
bns_exit_code_t bns_post_ledger_input(
    const bns_client_t* const      bnsClient,
    const char* const              cmdJson,
    const receipt_locator_t* const receiptLocator,
    ledger_input_result_t* const   ledgerInputResult) {
  size_t count = 0;
bns_post_ledger_input_beg : {
  bns_exit_code_t exitCode;
  char*           reqJson = NULL;
  char*           url     = NULL;
  char*           res     = NULL;
  //check existence 
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_post_ledger_input_fail;
  }
  if (!bnsClient->config.serverUrl) {
    exitCode = BNS_CLIENT_CONFIG_SERVER_URL_NULL_ERROR;
    goto bns_post_ledger_input_fail;
  }
  if (!bnsClient->httpClient.post) {
    exitCode = BNS_CLIENT_HTTP_CLIENT_BNS_POST_NULL_ERROR;
    goto bns_post_ledger_input_fail;
  }
  if (!bnsClient->receiptDao.save) {
    exitCode = BNS_CLIENT_RECEIPT_DAO_SAVE_NULL_ERROR;
    goto bns_post_ledger_input_fail;
  }
  if (!cmdJson) {
    exitCode = BNS_CMD_JSON_NULL_ERROR;
    goto bns_post_ledger_input_fail;
  }
  if (!receiptLocator) {
    exitCode = BNS_RECEIPT_LOCATOR_NULL_ERROR;
    goto bns_post_ledger_input_fail;
  }
  if (!ledgerInputResult) {
    exitCode = BNS_LEDGER_INPUT_RESULT_NULL_ERROR;
    goto bns_post_ledger_input_fail;
  }

  LOG_INFO(
      "bns_post_ledger_input() begin, "
      "cmdJson=%s, " RECEIPT_LOCATOR_PRINT_FORMAT,
      cmdJson, RECEIPT_LOCATOR_TO_PRINT_ARGS(receiptLocator));
  //build ledger input Request
  if ((exitCode = build_ledger_input_request_json(
           bnsClient, cmdJson, receiptLocator, &reqJson)) != BNS_OK) {
    goto bns_post_ledger_input_fail;
  }
  //build post ledger input url
  build_post_ledger_input_url(&url, bnsClient->config.serverUrl);
  res = bnsClient->httpClient.post(url, reqJson);
  BNS_FREE(url);
  BNS_FREE(reqJson);
  if (!res) {
    exitCode = BNS_POST_LEDGER_INPUT_RESPONSE_NULL_ERROR;
    goto bns_post_ledger_input_fail;
  }
  //ensure ledgerInputResult works
  if ((exitCode = check_and_parse_ledger_input_response(
           res, ledgerInputResult)) != BNS_OK) {
    goto bns_post_ledger_input_fail;
  }
  BNS_FREE(res);
//check existence
  if ((exitCode =
           receipt_check_sig(bnsClient->bnsServerInfo.serverWalletAddress,
                             ledgerInputResult->receipt)) != BNS_OK) {
    goto bns_post_ledger_input_fail;
  }
  //save receipt in receiptDao
  bnsClient->receiptDao.save(ledgerInputResult->receipt);
  LOG_INFO("bns_post_ledger_input() end, " LEDGER_INPUT_RESULT_PRINT_FORMAT,
           LEDGER_INPUT_RESULT_TO_PRINT_ARGS(ledgerInputResult));
  return exitCode;

bns_post_ledger_input_fail:
  if (reqJson) { BNS_FREE(reqJson); }
  if (url) { BNS_FREE(url); }
  if (res) { BNS_FREE(res); }
  LOG_ERROR("bns_post_ledger_input() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  if (bnsClient && bnsClient->maxRetryCount) {
    if (is_ledger_input_error(exitCode)) { return exitCode; }
    if (count++ < *bnsClient->maxRetryCount) {
      LOG_DEBUG("bns_post_ledger_input() retry, count=%ld", count);
      if (bnsClient->retryDelaySec) { sleep(*bnsClient->retryDelaySec); }
      goto bns_post_ledger_input_beg;
    }
  }
  return exitCode;
}
}
//if clearanceOrder or IndexValue error, return true
bool is_ledger_input_resend_error(const bns_exit_code_t exitCode) {
  switch (exitCode) {
    case BNS_LEDGER_INPUT_CLEARANCE_ORDER_ERROR:
    case BNS_LEDGER_INPUT_INDEX_VALUE_ERROR: return true;
    default: return false;
  }
}
//if clearanceOrder. indexValue, ClientSignature, Authentication, CMD, are error, return true
bool is_ledger_input_error(const bns_exit_code_t exitCode) {
  switch (exitCode) {
    case BNS_LEDGER_INPUT_CLEARANCE_ORDER_ERROR:
    case BNS_LEDGER_INPUT_INDEX_VALUE_ERROR:
    case BNS_LEDGER_INPUT_CLIENT_SIGNATURE_ERROR:
    case BNS_LEDGER_INPUT_AUTHENTICATION_ERROR:
    case BNS_LEDGER_INPUT_CMD_ERROR:
    case BNS_LEDGER_INPUT_TX_COUNT_ERROR: return true;
    default: return false;
  }
}
//go through ledgerInputresponse and ensure it all works
bns_exit_code_t check_and_parse_ledger_input_response(
    const char* const res, ledger_input_result_t* const ledgerInputResult) {
  LOG_DEBUG("check_and_parse_ledger_input_response() begin");
  bns_exit_code_t exitCode;
  cJSON*          root = cJSON_Parse(res);
  cJSON *         status, *description;
  //get status and check
  status = cJSON_GetObjectItem(root, "status");
  if (!cJSON_IsString(status)) {
    exitCode = BNS_RESPONSE_STATUS_PARSE_ERROR;
    goto check_and_parse_ledger_input_response_fail;
  }
  //copy status into valueString and cleanup
  ledgerInputResult->status =
      (char*)malloc(sizeof(char) * (strlen(status->valuestring) + 1));
  strcpy(ledgerInputResult->status, status->valuestring);
  cJSON_DetachItemViaPointer(root, status);
  cJSON_Delete(status);
//get description and check
  description = cJSON_GetObjectItem(root, "description");
  if (!cJSON_IsString(description)) {
    exitCode = BNS_RESPONSE_DESCRIPTION_PARSE_ERROR;
    goto check_and_parse_ledger_input_response_fail;
  }
  //copy description and cleanup
  ledgerInputResult->description =
      (char*)malloc(sizeof(char) * (strlen(description->valuestring) + 1));
  strcpy(ledgerInputResult->description, description->valuestring);
  cJSON_DetachItemViaPointer(root, description);
  cJSON_Delete(description);
  //if status is not ok,
  if (strcmp(BNS_STATUS_OK, ledgerInputResult->status) != 0) {
    //check if it is warning,
    if (strcmp(BNS_STATUS_WARN, ledgerInputResult->status) == 0) {
      exitCode = BNS_LEDGER_INPUT_WARN;
      LOG_WARN(
          "check_and_parse_ledger_input_response() "
          "warn, " BNS_EXIT_CODE_PRINT_FORMAT,
          bns_strerror(exitCode));
          //else error
    } else {
      if (strcmp(CLEARANCE_ORDER_ERROR, ledgerInputResult->description) == 0) {
        exitCode = BNS_LEDGER_INPUT_CLEARANCE_ORDER_ERROR;
      } else if (strcmp(INDEX_VALUE_ERROR, ledgerInputResult->description) ==
                 0) {
        exitCode = BNS_LEDGER_INPUT_INDEX_VALUE_ERROR;
      } else if (strcmp(CLIENT_SIGNATURE_ERROR,
                        ledgerInputResult->description) == 0) {
        exitCode = BNS_LEDGER_INPUT_CLIENT_SIGNATURE_ERROR;
      } else if (strcmp(AUTHENTICATION_ERROR, ledgerInputResult->description) ==
                 0) {
        exitCode = BNS_LEDGER_INPUT_AUTHENTICATION_ERROR;
      } else if (strcmp(CMD_ERROR, ledgerInputResult->description) == 0) {
        exitCode = BNS_LEDGER_INPUT_CMD_ERROR;
      } else if (strcmp(TX_COUNT_ERROR, ledgerInputResult->description) == 0) {
        exitCode = BNS_LEDGER_INPUT_TX_COUNT_ERROR;
      } else {
        exitCode = BNS_RESPONSE_STATUS_ERROR;
      }
      goto check_and_parse_ledger_input_response_fail;
    }
  }
  //get receipt and check it then cleanup
  cJSON *receipt, *doneClearanceOrderList;
  receipt                    = cJSON_GetObjectItem(root, "receipt");
  ledgerInputResult->receipt = (receipt_t*)malloc(sizeof(receipt_t));
  if ((exitCode = parse_receipt_from_cjson(
           receipt, ledgerInputResult->receipt)) != BNS_OK) {
    goto check_and_parse_ledger_input_response_fail;
  }
  cJSON_DetachItemViaPointer(root, receipt);
  cJSON_Delete(receipt);
  //get clearanceOrder and check it then cleanup
  doneClearanceOrderList = cJSON_GetObjectItem(root, "doneClearanceOrderList");
  exitCode               = parse_done_clearance_order_list_from_cjson(
                    doneClearanceOrderList, &ledgerInputResult->doneClearanceOrder);
  if (exitCode != BNS_OK) { goto check_and_parse_ledger_input_response_fail; }
  cJSON_DetachItemViaPointer(root, doneClearanceOrderList);
  cJSON_Delete(doneClearanceOrderList);
//final cleanup
  cJSON_Delete(root);
  LOG_DEBUG(
      "check_and_parse_ledger_input_response() "
      "end, " LEDGER_INPUT_RESULT_PRINT_FORMAT,
      LEDGER_INPUT_RESULT_TO_PRINT_ARGS(ledgerInputResult));
  return exitCode;

check_and_parse_ledger_input_response_fail:
  cJSON_Delete(root);
  LOG_ERROR(
      "check_and_parse_ledger_input_response() "
      "error, " BNS_EXIT_CODE_PRINT_FORMAT,
      bns_strerror(exitCode));
  return exitCode;
}
//check that ClearanceOrder from roots 0th position works
bns_exit_code_t parse_done_clearance_order_list_from_cjson(
    const cJSON* const root, clearance_order_t* const doneCO) {
  LOG_DEBUG("parse_done_clearance_order_list_from_cjson() begin");
  bns_exit_code_t exitCode = BNS_OK;
  cJSON*          temp;
  //check existence
  if (!doneCO) {
    exitCode = BNS_DONE_CO_NULL_ERROR;
    goto parse_done_clearance_order_list_from_cjson_fail;
  }
  //get item root 
  temp = cJSON_GetArrayItem(root, 0);
  //check that cJSON at 0 is number
  if (!cJSON_IsNumber(temp)) {
    exitCode = BNS_RESPONSE_DONE_CO_PARSE_ERROR;
    goto parse_done_clearance_order_list_from_cjson_fail;
  }
  //check the done ClearanceOrder
  *doneCO = (clearance_order_t)temp->valueint;
  LOG_DEBUG(
      "parse_done_clearance_order_list_from_cjson() end, "
      "doneClearanceOrder=%lld",
      *doneCO);
  return exitCode;

parse_done_clearance_order_list_from_cjson_fail:
  LOG_ERROR(
      "parse_done_clearance_order_list_from_cjson() "
      "error, " BNS_EXIT_CODE_PRINT_FORMAT,
      bns_strerror(exitCode));
  return exitCode;
}
//free ledgerInputResult's status description and receipt
void ledger_input_result_free(ledger_input_result_t* const ledgerInputResult) {
  if (ledgerInputResult) {
    if (ledgerInputResult->status) { BNS_FREE(ledgerInputResult->status); }
    if (ledgerInputResult->description) {
      BNS_FREE(ledgerInputResult->description);
    }
    if (ledgerInputResult->receipt) { BNS_FREE(ledgerInputResult->receipt); }
  }
}
