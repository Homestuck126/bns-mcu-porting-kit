#include <bns-client/register/bns_server_info.h>
#include <bns-client/util/log.h>
#include <cJSON.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
//built serverUrl 
void build_get_bns_server_info_url(char** url, const char* const serverUrl) {
  if (!url) { return; }
  const size_t size = strlen(serverUrl) + strlen(LEDGER_BNS_SERVER_INFO_PATH);
  *url              = (char*)malloc(sizeof(char) * (size + 1));
  if (*url) { sprintf(*url, "%s%s", serverUrl, LEDGER_BNS_SERVER_INFO_PATH); }
}
//get server info from bns
bns_exit_code_t bns_get_server_info(const bns_client_t* const bnsClient,
                                    bns_server_info_t* const  bnsServerInfo) {
  size_t count = 0;
bns_get_server_info_beg:
  LOG_INFO("bns_get_server_info() begin");
  bns_exit_code_t exitCode;
  char*           url = NULL;
  char*           res = NULL;
  //check existence 
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_get_server_info_fail;
  }
  if (!bnsClient->config.serverUrl) {
    exitCode = BNS_CLIENT_CONFIG_SERVER_URL_NULL_ERROR;
    goto bns_get_server_info_fail;
  }
  if (!bnsClient->httpClient.post) {
    exitCode = BNS_CLIENT_HTTP_CLIENT_BNS_POST_NULL_ERROR;
    goto bns_get_server_info_fail;
  }
  if (!bnsServerInfo) {
    exitCode = BNS_SERVER_INFO_NULL_ERROR;
    goto bns_get_server_info_fail;
  }
  //get server url
  build_get_bns_server_info_url(&url, bnsClient->config.serverUrl);
  if (!url) {
    exitCode = BNS_GET_SERVER_INFO_URL_NULL_ERROR;
    goto bns_get_server_info_fail;
  }
  //call bnsclient with url
  res = bnsClient->httpClient.get(url);
  if (!res) {
    exitCode = BNS_GET_SERVER_INFO_RESPONSE_NULL_ERROR;
    goto bns_get_server_info_fail;
  }
  BNS_FREE(url);
  //
  exitCode = check_and_parse_bns_server_info_response(res, bnsServerInfo);
  if (exitCode != BNS_OK) { goto bns_get_server_info_fail; }

  BNS_FREE(res);

  LOG_INFO("bns_get_server_info() end, " BNS_SERVER_INFO_PRINT_FORMAT,
           BNS_SERVER_INFO_TO_PRINT_ARGS(bnsServerInfo));
  return exitCode;

bns_get_server_info_fail:
  if (url) { BNS_FREE(url); }
  if (res) { BNS_FREE(res); }
  LOG_ERROR("bns_get_server_info() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  if (bnsClient && bnsClient->maxRetryCount) {
    if (count++ < *bnsClient->maxRetryCount) {
      LOG_DEBUG("bns_get_server_info() retry, count=%ld", count);
      if (bnsClient->retryDelaySec) { sleep(*bnsClient->retryDelaySec); }
      goto bns_get_server_info_beg;
    }
  }
  return exitCode;
}
//save bns ServerInfo and check it works 
bns_exit_code_t check_and_parse_bns_server_info_response(
    const char* const res, bns_server_info_t* const bnsServerInfo) {
  LOG_DEBUG("check_and_parse_bns_server_info_response() begin");
  bns_exit_code_t exitCode = BNS_OK;
  cJSON*          root     = NULL;
  cJSON*          temp     = NULL;
  //check server info
  if (!bnsServerInfo) {
    exitCode = BNS_SERVER_INFO_NULL_ERROR;
    goto check_bns_server_info_response_fail;
  }
  //parse cJSON
  root = cJSON_Parse(res);
  //get serverWalletAddress and cleanup
  temp = cJSON_GetObjectItem(root, "serverWalletAddress");
  if (!cJSON_IsString(temp)) {
    exitCode = BNS_RESPONSE_SERVER_WALLET_ADDRESS_PARSE_ERROR;
    goto check_bns_server_info_response_fail;
  }
  //save serverWalletAddress
  strncpy(bnsServerInfo->serverWalletAddress, temp->valuestring,
          ADDRESS_0X_STR_LEN - 1);
  cJSON_DetachItemViaPointer(root, temp);
  cJSON_Delete(temp);
  //get servercontractAddress and cleanup
  temp = cJSON_GetObjectItem(root, "contractAddress");
  if (!cJSON_IsString(temp)) {
    exitCode = BNS_RESPONSE_CONTRACT_ADDRESS_PARSE_ERROR;
    goto check_bns_server_info_response_fail;
  }
  //save contractAddress
  strncpy(bnsServerInfo->contractAddress, temp->valuestring,
          ADDRESS_0X_STR_LEN - 1);
  //cleanup
  cJSON_DetachItemViaPointer(root, temp);
  cJSON_Delete(temp);
  cJSON_Delete(root);
  LOG_DEBUG(
      "check_and_parse_bns_server_info_response() "
      "end, " BNS_SERVER_INFO_PRINT_FORMAT,
      BNS_SERVER_INFO_TO_PRINT_ARGS(bnsServerInfo));
  return exitCode;

check_bns_server_info_response_fail:
  cJSON_Delete(root);
  LOG_ERROR(
      "check_and_parse_bns_server_info_response() "
      "error, " BNS_EXIT_CODE_PRINT_FORMAT,
      bns_strerror(exitCode));
  return exitCode;
}
