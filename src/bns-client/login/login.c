#include <bns-client/login/login.h>
#include <bns-client/util/log.h>
#include <bns-client/util/signature_util.h>
#include <bns-client/util/string_util.h>
#include <bns-client/util/time_util.h>
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//build post login url from serverUrl and LedgerLogin
void build_post_login_url(char** url, const char* const serverUrl) {
  if (!url) { return; }
  //get size
  size_t size = strlen(serverUrl) + strlen(LEDGER_LOGIN_PATH);
  //build url
  *url        = (char*)malloc(sizeof(char) * (size + 1));
  if (*url) { sprintf(*url, "%s%s", serverUrl, LEDGER_LOGIN_PATH); }
}
//build and call login as well as verify login
bns_exit_code_t bns_login(const bns_client_t* const bnsClient) {
  bns_exit_code_t exitCode;
  char*           url = NULL;
  char*           res = NULL;
  char*           req = NULL;
  LOG_DEBUG("bns_login() start");
  //check existence and build url
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_login_fail;
  }
  build_post_login_url(&url, bnsClient->config.serverUrl);
  if (!url) {
    exitCode = BNS_LOGIN_URL_NULL_ERROR;
    goto bns_login_fail;
  }
  //built login cJSON
  if ((exitCode = build_login_request_json(bnsClient, &req)) != BNS_OK) {
    goto bns_login_fail;
  }
  //call post on client with url and request
  res = bnsClient->httpClient.post(url, req);
  if (!res) {
    exitCode = BNS_LOGIN_RESPONSE_NULL_ERROR;
    goto bns_login_fail;
  }
  //check response 
  if ((exitCode = check_login_response(res, bnsClient->walletAddress)) !=
      BNS_OK) {
    goto bns_login_fail;
  }
  BNS_FREE(url);
  BNS_FREE(req);
  BNS_FREE(res);

  LOG_DEBUG("bns_login() end");
  return exitCode;

bns_login_fail:
  if (url) { BNS_FREE(url); }
  if (res) { BNS_FREE(res); }
  if (req) { BNS_FREE(req); }

  LOG_ERROR("bns_login() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//call creation of login request, signature of login design and login design cJSON
bns_exit_code_t build_login_request_json(const bns_client_t* bnsClient,
                                         char**              reqJson) {
  LOG_DEBUG("build_login_request_json() start");
  login_request_t req = {0};
  bns_exit_code_t exitCode;
  //check existence
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto build_login_request_json_fail;
  }
  //get to build login request
  if ((exitCode = build_login_request(bnsClient, &req)) != BNS_OK) {
    goto build_login_request_json_fail;
  }
  //built login request sign
  if ((exitCode = login_request_sign(&req, bnsClient->config.privateKey)) !=
      BNS_OK) {
    goto build_login_request_json_fail;
  }
  //built login request json
  if ((exitCode = login_request_to_json(&req, reqJson)) != BNS_OK) {
    goto build_login_request_json_fail;
  }
  //cleanup
  login_request_free(&req);
  return exitCode;
build_login_request_json_fail:
  login_request_free(&req);
  LOG_ERROR("build_login_request_json() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//built toSignMessage I confirm that ... walletAddress, and I agree that timestamp
bns_exit_code_t build_login_request(const bns_client_t* const bnsClient,
                                    login_request_t* const    req) {
  bns_exit_code_t exitCode = BNS_OK;
  //check existence
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto build_login_request_fail;
  }
  if (!req) {
    exitCode = BNS_LOGIN_REQUEST_NULL_ERROR;
    goto build_login_request_fail;
  }
  LOG_DEBUG("build_LOGIN_request() begin");
  //copy walletAddress to callerAddress
  strcpy(req->callerAddress, bnsClient->walletAddress);
  //get size
  size_t size = strlen(LOGIN_MESSAGE_STR_PART1);
  size += strlen(LOGIN_MESSAGE_STR_PART2);
  req->toSignMessage =
      (char*)malloc(sizeof(char) * (size + ADDRESS_0X_STR_LEN + 16));
  //get timestamp
  char* timestamp = get_timestamp_string();
  //put loginMessage, walletaddress, loginMessage2, and timestamp together
  sprintf(req->toSignMessage, "%s%s%s%s.", LOGIN_MESSAGE_STR_PART1,
          bnsClient->walletAddress, LOGIN_MESSAGE_STR_PART2, timestamp);
  BNS_FREE(timestamp);
  LOG_DEBUG("build_LOGIN_request() end, " LOGIN_REQUEST_PRINT_FORMAT,
            LOGIN_REQUEST_TO_PRINT_ARGS(req));
  return exitCode;

build_login_request_fail:
  login_request_free(req);
  LOG_ERROR("build_LOGIN_request() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//get signature of login request 
bns_exit_code_t login_request_sign(login_request_t* const req,
                                   const char* const      privateKey) {
  bns_exit_code_t exitCode;
  char*           buffer = NULL;
  //check existence
  if (!req || !req->toSignMessage || strlen(req->callerAddress) == 0) {
    exitCode = BNS_LOGIN_REQUEST_NULL_ERROR;
    goto login_request_sign_fail;
  }
  if (!privateKey) {
    exitCode = BNS_PRIVATE_KEY_NULL_ERROR;
    goto login_request_sign_fail;
  }
  LOG_DEBUG("login_request_sign() begin, " LOGIN_REQUEST_PRINT_FORMAT,
            LOGIN_REQUEST_TO_PRINT_ARGS(req));
  //get size
  size_t size = 0;
  size += strlen(req->toSignMessage);
  buffer = (char*)malloc(sizeof(char) * (size + 1));
  //copy toSignMessage to buffer
  strcpy(buffer, req->toSignMessage);
  //encrypt buffer
  unsigned char shaResult[SHA3_BYTE_LEN] = {0};
  bns_sha3_prefix((unsigned char*)buffer, size, shaResult);
  BNS_FREE(buffer);
  //get signature from shaResult
  if ((exitCode = bns_sign(shaResult, privateKey, &req->sig)) != BNS_OK) {
    goto login_request_sign_fail;
  }
  LOG_DEBUG("login_request_sign() end, " LOGIN_REQUEST_PRINT_FORMAT,
            LOGIN_REQUEST_TO_PRINT_ARGS(req));
  return exitCode;

login_request_sign_fail:
  LOG_ERROR("login_request_sign() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//built CJSON login request
bns_exit_code_t login_request_to_json(const login_request_t* const loginRequest,
                                      char** const                 json) {
  LOG_DEBUG("login_request_to_json() begin");
  bns_exit_code_t exitCode = BNS_OK;
  cJSON*          root     = NULL;
  //check existence
  if (!loginRequest || strlen(loginRequest->callerAddress) == 0 ||
      !loginRequest->toSignMessage) {
    exitCode = BNS_LOGIN_REQUEST_NULL_ERROR;
    goto login_request_to_json_fail;
  }
  if (!json) {
    exitCode = BNS_LOGIN_REQUEST_JSON_NULL_ERROR;
    goto login_request_to_json_fail;
  }
  //build cJSON of signature
  root       = cJSON_CreateObject();
  cJSON* sig = cJSON_CreateObject();
  cJSON_AddItemToObject(root, "address",
                        cJSON_CreateString(loginRequest->callerAddress));
  cJSON_AddItemToObject(root, "toSignMessage",
                        cJSON_CreateString(loginRequest->toSignMessage));
  cJSON_AddItemToObject(root, "sig", sig);
  cJSON_AddItemToObject(sig, "r", cJSON_CreateString(loginRequest->sig.r));
  cJSON_AddItemToObject(sig, "s", cJSON_CreateString(loginRequest->sig.s));
  cJSON_AddItemToObject(sig, "v", cJSON_CreateString(loginRequest->sig.v));
  if (*json) { BNS_FREE(*json); }
  //get cJSON as text
  *json = cJSON_PrintUnformatted(root);
  //clean 
  cJSON_Delete(root);
  LOG_DEBUG("login_request_to_json() end, json=%s", *json);
  return exitCode;

login_request_to_json_fail:
  cJSON_Delete(root);
  LOG_ERROR("login_request_to_json() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//free login request
void login_request_free(login_request_t* const loginRequest) {
  if (loginRequest) {
    if (loginRequest->toSignMessage) { BNS_FREE(loginRequest->toSignMessage); }
    //2 of these? 
    if (loginRequest->toSignMessage) { BNS_FREE(loginRequest->toSignMessage); }
  }
}
//check that wallet is the same
bns_exit_code_t check_login_response(const char* const res,
                                     const char* const walletAddress) {
  LOG_DEBUG("check_login_response() begin");
  bns_exit_code_t exitCode = BNS_OK;
  cJSON*          root     = cJSON_Parse(res);
  cJSON*          address;
  //get address and check
  address = cJSON_GetObjectItem(root, "address");
  if (!cJSON_IsString(address)) {
    exitCode = BNS_RESPONSE_STATUS_PARSE_ERROR;
    goto check_login_response_fail;
  }
  //check that address is the same
  if (bns_equals_ignore_case(walletAddress, address->valuestring) == false) {
    exitCode = BNS_LOGIN_WARN;
    LOG_WARN(
        "check_login_response() "
        "warn, " BNS_EXIT_CODE_PRINT_FORMAT,
        bns_strerror(exitCode));

    goto check_login_response_fail;
  }
  //cleanup
  cJSON_Delete(root);
  LOG_DEBUG("check_login_response() end");
  return exitCode;

check_login_response_fail:
  cJSON_Delete(root);
  LOG_ERROR(
      "check_login_response() "
      "error, " BNS_EXIT_CODE_PRINT_FORMAT,
      bns_strerror(exitCode));
  return exitCode;
}
