#include <bns-client/register/register.h>
#include <bns-client/util/log.h>
#include <bns-client/util/string_util.h>
#include <cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//build check register url
char* build_check_register_url(const char* const serverUrl,
                               const char* const callerAddress) {
  if (!serverUrl) { return NULL; }
  if (!callerAddress) { return NULL; }
  const size_t size =
      strlen(serverUrl) + strlen(LEDGER_CHECK_REGISTER) + strlen(callerAddress);
  char* url = (char*)malloc(sizeof(char) * (size + 2));
  if (url) {
    sprintf(url, "%s%s%s", serverUrl, LEDGER_CHECK_REGISTER, callerAddress);
  }
  return url;
}

bns_exit_code_t bns_check_register(const bns_client_t* const bnsClient,
                                   bool* const               result) {
  bns_exit_code_t exitCode = BNS_OK;
  char*           url      = NULL;
  char*           res      = NULL;
  LOG_DEBUG("bns_check_register() start");

  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_check_register_fail;
  }
  url = build_check_register_url(bnsClient->config.serverUrl,
                                 bnsClient->walletAddress);
  if (!url) {
    exitCode = BNS_CHECK_REGISTER_URL_NULL_ERROR;
    goto bns_check_register_fail;
  }

  res = bnsClient->httpClient.get(url);
  if (!res) {
    exitCode = BNS_CHECK_REGISTER_RESPONSE_NULL_ERROR;
    goto bns_check_register_fail;
  }
  BNS_FREE(url);

  *result = strcmp("true", res) == 0;

  BNS_FREE(res);

  LOG_DEBUG("bns_check_register() end");
  return exitCode;

bns_check_register_fail:
  if (url) { BNS_FREE(url); }
  LOG_ERROR("bns_check_register() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//clean check register
void register_request_free(register_request_t* const registerRequest) {
  if (registerRequest) {
    if (registerRequest->email) { BNS_FREE(registerRequest->email); }
    if (registerRequest->toSignMessage) {
      BNS_FREE(registerRequest->toSignMessage);
    }
  }
}
