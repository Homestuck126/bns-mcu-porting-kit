#ifndef BNS_C_CLIENT_LOGIN_H
#define BNS_C_CLIENT_LOGIN_H

#include <bns-client/core/core.h>

#define LOGIN_MESSAGE_STR_PART1 \
  "I confirm that I am the owner of the following address:"
#define LOGIN_MESSAGE_STR_PART2                                            \
  ", and I agree to login to Blockchain Notary with this wallet address. " \
  "Validate timestamp:"

#define LOGIN_REQUEST_PRINT_FORMAT                                    \
  "loginRequest=login_request_t(callerAddress=%s, toSignMessage=%s, " \
  "sigClient=sig_t(r=%s, s=%s, v=%s))"

#define LOGIN_REQUEST_TO_PRINT_ARGS(loginRequest)             \
  loginRequest->callerAddress, (loginRequest)->toSignMessage, \
      (loginRequest)->sig.r, (loginRequest)->sig.s, (loginRequest)->sig.v

#define LEDGER_LOGIN_PATH "/account/login"
//build post login url from serverUrl and LedgerLogin
void build_post_login_url(char** url, const char* serverUrl);
//build and call login as well as verify login
_CHECK_RESULT
bns_exit_code_t bns_login(const bns_client_t* bnsClient);
//call creation of login request, signature of login design and login design cJSON
_CHECK_RESULT
bns_exit_code_t build_login_request_json(const bns_client_t* bnsClient,
                                         char**              reqJson);
//built toSignMessage I confirm that ... walletAddress, and I agree that timestamp
_CHECK_RESULT
bns_exit_code_t build_login_request(const bns_client_t* bnsClient,
                                    login_request_t*    req);
//get signature of login request 
_CHECK_RESULT
bns_exit_code_t login_request_sign(login_request_t* req,
                                   const char*      privateKey);
//built CJSON login request
_CHECK_RESULT
bns_exit_code_t login_request_to_json(const login_request_t* loginRequest,
                                      char**                 json);
//check that wallet is the same from res and walletAddress
_CHECK_RESULT
bns_exit_code_t check_login_response(const char* res,
                                     const char* walletAddress);
//deleate toSignMessage
void login_request_free(login_request_t* req);

#endif  // BNS_C_CLIENT_LOGIN_H
