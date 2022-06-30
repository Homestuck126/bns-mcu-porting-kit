#ifndef BNS_C_CLIENT_LEDGER_INPUT_H
#define BNS_C_CLIENT_LEDGER_INPUT_H

#include <bns-client/core/core.h>
#include <bns-client/input/receipt.h>
#include <cJSON.h>

#define LEDGER_INPUT_PATH "/input"

#define CLEARANCE_ORDER_ERROR "CLEARANCE_ORDER_ERROR"
#define INDEX_VALUE_ERROR "INDEX_VALUE_ERROR"

#define CLIENT_SIGNATURE_ERROR "CLIENT_SIGNATURE_ERROR"
#define AUTHENTICATION_ERROR "AUTHENTICATION_ERROR"
#define CMD_ERROR "CMD_ERROR"
#define TX_COUNT_ERROR "TX_COUNT_ERROR"

#define LEDGER_INPUT_RESULT_PRINT_FORMAT                                \
  "ledgerInputResult=ledger_input_result_t(status=%s, description=%s, " \
  "" RECEIPT_PRINT_FORMAT ", doneClearanceOrder=%lld)"
#define LEDGER_INPUT_RESULT_TO_PRINT_ARGS(ledgerInputResult)   \
  ledgerInputResult->status, (ledgerInputResult)->description, \
      RECEIPT_TO_PRINT_ARGS((ledgerInputResult)->receipt),     \
      (ledgerInputResult)->doneClearanceOrder
//get url for post ledgerInputUrl
void build_post_ledger_input_url(char** url, const char* serverUrl);
//call bns for post with ledgerInput
_CHECK_RESULT
bns_exit_code_t bns_post_ledger_input(const bns_client_t*      bnsClient,
                                      const char*              cmdJson,
                                      const receipt_locator_t* receiptLocator,
                                      ledger_input_result_t* ledgerInputResult);
//if clearanceOrder or IndexValue error, return true
_CHECK_RESULT
bool is_ledger_input_resend_error(bns_exit_code_t exitCode);
//if clearanceOrder. indexValue, ClientSignature, Authentication, CMD, are error, return true
_CHECK_RESULT
bool is_ledger_input_error(bns_exit_code_t exitCode);
//go through ledgerInputresponse and ensure it all works
_CHECK_RESULT
bns_exit_code_t check_and_parse_ledger_input_response(
    const char* res, ledger_input_result_t* ledgerInputResult);
//check that ClearanceOrder from roots 0th position works
_CHECK_RESULT
bns_exit_code_t parse_done_clearance_order_list_from_cjson(
    const cJSON* root, clearance_order_t* doneCO);
//free ledgerInputResult's status description and receipt
void ledger_input_result_free(ledger_input_result_t* ledgerInputResult);

#endif  // BNS_C_CLIENT_LEDGER_INPUT_H
