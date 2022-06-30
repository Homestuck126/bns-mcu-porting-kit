#include <bns-client/contract/clearance_record_cache.h>
#include <bns-client/contract/contract.h>
#include <bns-client/util/log.h>
#include <cJSON.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//format instructions in the form of 0s 64 and a long long hex
bns_exit_code_t build_clearance_record_contract_instruction(
    const clearance_order_t clearanceOrder, char** const instruction) {
  LOG_DEBUG(
      "build_clearance_record_contract_instruction() begin, "
      "clearanceOrder=%lld",
      clearanceOrder);
  bns_exit_code_t exitCode = BNS_OK;
  //if clearanceOrder is off, then error
  if (clearanceOrder <= 0) {
    exitCode = BNS_CLEARANCE_ORDER_LESS_THAN_OR_EQUAL_TO_ZERO_ERROR;
    goto build_clearance_record_contract_instruction_fail;
  }
  //if instruction does not exist, error
  if (!instruction) {
    exitCode = BNS_CLEARANCE_RECORD_INSTRUCTION_NULL_ERROR;
    goto build_clearance_record_contract_instruction_fail;
  }
  //if there is something in instruction but first dereference, reallocate
  if (*instruction) {
    *instruction =
        (char*)realloc(*instruction, sizeof(char) * INSTRUCTION_STR_LEN);
  } else {
    // else, allocate new
    *instruction = (char*)malloc(sizeof(char) * INSTRUCTION_STR_LEN);
  }
  // format instruction in the form of padded with 0s 64 long long hex
  sprintf(*instruction, INSTRUCTION_PREFIX "%064llx", clearanceOrder);
  LOG_DEBUG("build_clearance_record_contract_instruction() end");
  return exitCode;

build_clearance_record_contract_instruction_fail:
  LOG_ERROR(
      "build_clearance_record_contract_instruction() "
      "error, " BNS_EXIT_CODE_PRINT_FORMAT,
      bns_strerror(exitCode));
  return exitCode;
}
//put information into request json and make it into text 
bns_exit_code_t build_contract_request_json(const char* const contractAddress,
                                            const char* const instruction,
                                            char** const      requestJson) {
  LOG_DEBUG(
      "build_contract_request_json() begin, contractAddress=%s, instruction=%s",
      contractAddress, instruction);
  bns_exit_code_t exitCode = BNS_OK;
  cJSON*          root     = NULL;
  cJSON*          params   = NULL;
  cJSON*          item     = NULL;
  //check that everything exists
  if (!contractAddress) {
    exitCode = BNS_CONTRACT_ADDRESS_NULL_ERROR;
    goto build_contract_request_json_fail;
  }
  if (!instruction) { exitCode = BNS_CLEARANCE_RECORD_INSTRUCTION_NULL_ERROR; }
  if (!requestJson) {
    exitCode = BNS_CONTRACT_REQUEST_JSON_NULL_ERROR;
    goto build_contract_request_json_fail;
  }

  item   = cJSON_CreateObject();
  params = cJSON_CreateArray();
  root   = cJSON_CreateObject();
  //create object of to and data related to contract Address and instruction
  cJSON_AddStringToObject(item, "to", contractAddress);
  cJSON_AddStringToObject(item, "data", instruction);

  //create array of item with items inside params and params with latest
  cJSON_AddItemToArray(params, item);
  cJSON_AddItemToArray(params, cJSON_CreateString("latest"));

  //add into root json jsonrpc version, method of call, parameters, and id
  cJSON_AddStringToObject(root, "jsonrpc", "2.0");
  cJSON_AddStringToObject(root, "method", "eth_call");
  cJSON_AddItemToObject(root, "params", params);
  cJSON_AddNumberToObject(root, "id", 1, 1);
// if requestJson still exists, free it
  if (*requestJson) { BNS_FREE(*requestJson); }
  //convert root to text in requestJson
  *requestJson = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  LOG_DEBUG("build_contract_request_json() end, requestJson=%s", *requestJson);
  return exitCode;

build_contract_request_json_fail:
  cJSON_Delete(root);
  LOG_ERROR("build_contract_request_json() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//Call bns server with requestMessage and create clearanceRecord with clearanceOrder also set clearaceRecord in cache
bns_exit_code_t contract_post_clearance_record(
    const bns_client_t* const bnsClient,
    const clearance_order_t   clearanceOrder,
    clearance_record_t* const clearanceRecord) {
  size_t count = 0;
contract_post_clearance_record_beg:
  LOG_INFO("contract_post_clearance_record() begin, clearanceOrder=%lld",
           clearanceOrder);
  bns_exit_code_t exitCode       = BNS_OK;
  char*           requestMessage = NULL;
  char*           res            = NULL;
  char*           coInstruction  = NULL;
  //check parameters
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto contract_post_clearance_record_fail;
  }
  if (!bnsClient->config.nodeUrl) {
    exitCode = BNS_CLIENT_CONFIG_NODE_URL_NULL_ERROR;
    goto contract_post_clearance_record_fail;
  }
  if (!bnsClient->httpClient.eth_post) {
    exitCode = BNS_CLIENT_HTTP_CLIENT_ETH_POST_NULL_ERROR;
    goto contract_post_clearance_record_fail;
  }
  if (clearanceOrder <= 0) {
    exitCode = BNS_CLEARANCE_ORDER_LESS_THAN_OR_EQUAL_TO_ZERO_ERROR;
    goto contract_post_clearance_record_fail;
  }
  if (!clearanceRecord) {
    exitCode = BNS_CLEARANCE_RECORD_NULL_ERROR;
    goto contract_post_clearance_record_fail;
  }
  //find clearance record from clearance order
  get_clearance_record_cache(clearanceOrder, clearanceRecord);
  //if clearanceorder is not 0, exit.
  if (clearanceRecord->clearanceOrder != 0) {
    LOG_INFO(
        "contract_post_clearance_record() end, from "
        "cache " CLEARANCE_RECORD_PRINT_FORMAT,
        CLEARANCE_RECORD_TO_PRINT_ARGS(clearanceRecord));
    return exitCode;
  }
  //if instruction is not ok, error
  if ((exitCode = build_clearance_record_contract_instruction(
           clearanceOrder, &coInstruction)) != BNS_OK) {
    goto contract_post_clearance_record_fail;
  }
  //makes contract into test
  if ((exitCode = build_contract_request_json(
           bnsClient->bnsServerInfo.contractAddress, coInstruction,
           &requestMessage)) != BNS_OK) {
    goto contract_post_clearance_record_fail;
  }
  BNS_FREE(coInstruction);
  //eth post the command and information in requestMessage
  res =
      bnsClient->httpClient.eth_post(bnsClient->config.nodeUrl, requestMessage);
  if (!res) {
    exitCode = BNS_ETH_POST_CLEARANCE_RECORD_RESPONSE_NULL_ERROR;
    goto contract_post_clearance_record_fail;
  }
  //create clearanceRecord
  BNS_FREE(requestMessage);
  if ((exitCode = check_and_parse_contract_clearance_record_response(
           res, clearanceRecord)) != BNS_OK) {
    goto contract_post_clearance_record_fail;
  }
  BNS_FREE(res);
  //set the clearanceRecord 
  set_clearance_record_cache(clearanceRecord);
  LOG_INFO(
      "contract_post_clearance_record() end, " CLEARANCE_RECORD_PRINT_FORMAT,
      CLEARANCE_RECORD_TO_PRINT_ARGS(clearanceRecord));
  return exitCode;

contract_post_clearance_record_fail:
  if (coInstruction) { BNS_FREE(coInstruction); }
  if (requestMessage) { BNS_FREE(requestMessage); }
  if (res) { BNS_FREE(res); }
  LOG_ERROR(
      "contract_post_clearance_record() error, " BNS_EXIT_CODE_PRINT_FORMAT,
      bns_strerror(exitCode));
  if (bnsClient && bnsClient->maxRetryCount) {
    if (count++ < *bnsClient->maxRetryCount) {
      LOG_DEBUG("contract_post_clearance_record() retry, count=%ld", count);
      if (bnsClient->retryDelaySec) { sleep(*bnsClient->retryDelaySec); }
      goto contract_post_clearance_record_beg;
    }
  }
  return exitCode;
}
//check the response from server and parse the response also create clearanceRecord
bns_exit_code_t check_and_parse_contract_clearance_record_response(
    const char* const res, clearance_record_t* const clearanceRecord) {
  LOG_DEBUG("check_and_parse_contract_clearance_record_response() begin");
  bns_exit_code_t exitCode = BNS_OK;
  cJSON*          root     = NULL;
  cJSON*          item;
  //check existence of clearanceREcord
  if (!clearanceRecord) {
    exitCode = BNS_CLEARANCE_RECORD_NULL_ERROR;
    goto check_contract_clearance_record_response_fail;
  }
  //add error into root
  root = cJSON_Parse(res);
  item = cJSON_GetObjectItem(root, "error");
  if (item) {
    exitCode = BNS_RESPONSE_STATUS_ERROR;
    goto check_contract_clearance_record_response_fail;
  }
  //add result into root 
  item = cJSON_GetObjectItem(root, "result");
  if (!cJSON_IsString(item)) {
    exitCode = BNS_RESPONSE_CLEARANCE_RECORD_PARSE_ERROR;
    goto check_contract_clearance_record_response_fail;
  }
  //copy value string into hash after some modification
  strncpy(clearanceRecord->rootHash, item->valuestring + 2 + 64, 64);
  strncpy(clearanceRecord->chainHash, item->valuestring + 2 + 64 * 2, 64);
  clearanceRecord->clearanceOrder = 0;
  //build clearanceOrder 
  for (int i = 2; i < 2 + 64; i++) {
    clearanceRecord->clearanceOrder *= 16;
    if (isdigit(item->valuestring[i])) {
      clearanceRecord->clearanceOrder += item->valuestring[i] - '0';
    } else if (isalpha(item->valuestring[i])) {
      clearanceRecord->clearanceOrder +=
          tolower(item->valuestring[i]) - 'a' + 10;
    } else {
      exitCode = BNS_RESPONSE_CLEARANCE_RECORD_PARSE_ERROR;
      goto check_contract_clearance_record_response_fail;
    }
  }
//cleanup
  cJSON_Delete(root);
  LOG_DEBUG(
      "check_and_parse_contract_clearance_record_response() "
      "end" CLEARANCE_RECORD_PRINT_FORMAT,
      CLEARANCE_RECORD_TO_PRINT_ARGS(clearanceRecord));
  return exitCode;

check_contract_clearance_record_response_fail:
  cJSON_Delete(root);
  LOG_ERROR(
      "check_and_parse_contract_clearance_record_response() "
      "error, " BNS_EXIT_CODE_PRINT_FORMAT,
      bns_strerror(exitCode));
  return exitCode;
}
