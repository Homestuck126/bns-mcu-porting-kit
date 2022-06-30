#ifndef BNS_CLIENT_SRC_BNS_CLIENT_INPUT_LEDGER_INPUT_REQUEST_H_
#define BNS_CLIENT_SRC_BNS_CLIENT_INPUT_LEDGER_INPUT_REQUEST_H_

#include <bns-client/core/core.h>

#if defined(RECEIPT_TIMESTAMP_IS_LONG)
#define LEDGER_INPUT_REQUEST_PRINT_FORMAT                        \
  "ledgerInputRequest=ledger_input_request_t(callerAddress=%s, " \
  "timestamp=%lld, cmd=%s, indexValue=%s, "                      \
  "metadata=%s, clearanceOrder=%lld, sigClient=sig_t(r=%s, s=%s, v=%s))"
#else
#define LEDGER_INPUT_REQUEST_PRINT_FORMAT                                      \
  "ledgerInputRequest=ledger_input_request_t(callerAddress=%s, timestamp=%s, " \
  "cmd=%s, indexValue=%s, "                                                    \
  "metadata=%s, clearanceOrder=%lld, sigClient=sig_t(r=%s, s=%s, v=%s))"
#endif

#define LEDGER_INPUT_REQUEST_TO_PRINT_ARGS(ledgerInputRequest)              \
  ledgerInputRequest->callerAddress, (ledgerInputRequest)->timestamp,       \
      (ledgerInputRequest)->cmd, (ledgerInputRequest)->indexValue,          \
      (ledgerInputRequest)->metadata, (ledgerInputRequest)->clearanceOrder, \
      (ledgerInputRequest)->sigClient.r, (ledgerInputRequest)->sigClient.s, \
      (ledgerInputRequest)->sigClient.v
//built Ledger Input Request
_CHECK_RESULT
bns_exit_code_t build_ledger_input_request_json(
    const bns_client_t*      bnsClient,
    const char*              cmdJson,
    const receipt_locator_t* receiptLocator,
    char**                   reqJson);
//built ledger Input Request(part of LedgerInputRequestJson)
_CHECK_RESULT
bns_exit_code_t build_ledger_input_request(
    const bns_client_t*      bnsClient,
    const char*              cmdJson,
    const receipt_locator_t* receiptLocator,
    ledger_input_request_t*  ledgerInputRequest);
//create ledger input request signature
_CHECK_RESULT
bns_exit_code_t ledger_input_request_sign(
    ledger_input_request_t* ledgerInputRequest, const char* privateKey);
//get Ledger Input Request as a string
_CHECK_RESULT
bns_exit_code_t ledger_input_request_to_json(
    const ledger_input_request_t* ledgerInputRequest, char** json);
//free LedgerInputRequest data
void ledger_input_request_free(ledger_input_request_t* ledgerInputRequest);

#endif  // BNS_CLIENT_SRC_BNS_CLIENT_INPUT_LEDGER_INPUT_REQUEST_H_
