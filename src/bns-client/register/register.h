#ifndef BNS_C_CLIENT_REGISTER_H
#define BNS_C_CLIENT_REGISTER_H

#include <bns-client/core/core.h>

#define CAN_REGISTER_STR "CAN_REGISTER"
#define ALREADY_REGISTER_STR "ALREADY_REGISTER"
#define SIGNATURE_ERROR_STR "SIGNATURE_ERROR"
#define ETHEREUM_ADDRESS_ERROR_STR "ETHEREUM_ADDRESS_ERROR"
#define PUBLIC_KEY_ERROR_STR "PUBLIC_KEY_ERROR"
#define TXCOUNT_NOT_ENOUGH_ERROR_STR "TXCOUNT_NOT_ENOUGH_ERROR"
#define OUT_OF_REGISTER_COUNT_STR "OUT_OF_REGISTER_COUNT"

#define LEDGER_CHECK_REGISTER "/account/register/check"
//build check register url
_CHECK_RESULT
char* build_check_register_url(const char* serverUrl,
                               const char* callerAddress);

_CHECK_RESULT
bns_exit_code_t bns_check_register(const bns_client_t* bnsClient, bool* result);
//clean check register
void register_request_free(register_request_t* registerRequest);

#endif  // BNS_C_CLIENT_REGISTER_H
