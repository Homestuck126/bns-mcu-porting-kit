#include "callback.h"
#include "config.h"
#include <bns-client/input/ledger_input.h>
#include <bns-client/input/receipt_locator.h>
#include <bns-client/util/log.h>
#include <bns-client/util/string_util.h>
#include <bns-client/verify/verify.h>
#include <string.h>

/** 1.
 * When initialize the BNS Client
 * BNS Client will send registerRequest to BNS Server
 * and receive registerResult from BNS Server
 * Developers can implement the code in register_callback function
 * to callback the informations in registerRequest and registerResult
 */
void register_callback(_UNUSED const register_request_t* registerRequest,
                       _UNUSED bool                      result) {
  LOG_DEBUG("register_callback begin()");
      printf("ITM_BNS_C_CLIENT_EXAMPLE\n\n\n\n\n\n\n\n\n\n");
  LOG_DEBUG("register_callback end()");
}

/** 2.
 * After successfully initializing the BNS Client
 * BNS Client will store CMD and other data in
 * ledgerInputRequest and do ledgerInput
 * to send ledgerInputRequest to the BNS Server
 * Developers can implement the code in create_ledger_input_by_cmd_callback
 * function to callback the information in ledgerInputRequest
 */
void create_ledger_input_by_cmd_callback(
    _UNUSED const receipt_locator_t*      receiptLocator,
    _UNUSED const ledger_input_request_t* ledgerInputRequest) {
  LOG_DEBUG("create_ledger_input_by_cmd_callback begin()");
  LOG_DEBUG("create_ledger_input_by_cmd_callback end()");
}

/** 3.
 * BNS Client will receive ledgerInputResult from BNS Server
 * after sending ledgerInputRequest
 * Developers can implement the code in ledger_input_response_callback function
 * to callback the informations in ledgerInputResult
 */
void ledger_input_response_callback(
    _UNUSED const receipt_locator_t* receiptLocator,
    _UNUSED const char*              cmdJson,
    const ledger_input_result_t*     ledgerInputResult) {
  LOG_DEBUG(
      "ledger_input_response_callback "
      "begin() " LEDGER_INPUT_RESULT_PRINT_FORMAT,
      LEDGER_INPUT_RESULT_TO_PRINT_ARGS(ledgerInputResult));
      
  LOG_DEBUG("ledger_input_response_callback end()");
}

/** 4.
 * BNS Client will receive binaryLedgerInputResult from BNS Server
 * after sending ledgerInputRequest
 * Developers can implement the code in binary_ledger_input_response_callback
 * function to callback the informations in binaryLedgerInputResult
 */
void binary_ledger_input_response_callback(
    _UNUSED const receipt_locator_t*            receiptLocator,
    _UNUSED const char*                         cmdJson,
    _UNUSED const binary_info_t*                binaryInfo,
    _UNUSED const binary_ledger_input_result_t* binaryLedgerInputResult) {
  LOG_DEBUG("binary_ledger_input_response_callback begin()");
  LOG_DEBUG("binary_ledger_input_response_callback end()");
}

/** 5.
 * The receipt is contained in ledgerInputResult / binaryLedgerInputResult
 * Developers can implement the code in receipt_event_callback function
 * to callback the informations in receipt
 */
void receipt_event_callback(_UNUSED const receipt_t* receipt) {
  LOG_DEBUG("receipt_event_callback begin()");
  LOG_DEBUG("receipt_event_callback end()");
}

/** 6.
 * The doneClearanceOrder is contained in ledgerInputResult /
 * binaryLedgerInputResult BNS Client will use doneClearanceOrder to find out
 * which receipts need to be verified Developers can implement the code in
 * done_clearance_order_event_callback function to callback the informations in
 * doneClearanceOrder
 */
void done_clearance_order_event_callback(_UNUSED clearance_order_t doneCO) {
  LOG_DEBUG("done_clearance_order_event_callback begin()");
  LOG_DEBUG("done_clearance_order_event_callback end()");
}

/** 7.
 * Before verifying the receipt
 * BNS Client will request the merkleProof of the receipts to be verified from
 * the Server The Merkle Proof is evidence of receipt verification BNS Client
 * will use Merkle proof to verify the receipt whether receipt is in the
 * TP-merkle tree. Developers can implement the code in merkle_proof_callback
 * function to callback the informations in merkleProof
 */

void merkle_proof_callback(_UNUSED const receipt_locator_t* receiptLocator,
                           _UNUSED const merkle_proof_t*    merkleProof) {
  LOG_DEBUG("merkle_proof_callback begin()");
  LOG_DEBUG("merkle_proof_callback end()");
}

/** 8.
 * After receiving the Merkle Proof
 * BNS Client will start to verify the receipt and store the result in
 * verifyReceiptResult Developers can implement the code in
 * verify_receipt_result_callback function to callback the informations in
 * verifyReceiptResult
 */
void verify_receipt_result_callback(
  _UNUSED const receipt_t *receipt, 
  _UNUSED const merkle_proof_t *merkleProof,
  const verify_receipt_result_t *verifyReceiptResult) {
   
    LOG_DEBUG("\nget_verify_receipt_result() begin\n");
    size_t retryCount = 5;

    //send_verify_receipt_result_to_dashboard(verifyReceiptResult, &retryCount);
    LOG_DEBUG("\nget_verify_receipt_result() end\n");
  }

void build_get_tx_count_url(char** url) {
  if (!url) { return; }
  char* serverUrl = SERVER_URL;
  char* txCount = "tx/overview";
  size_t size = strlen(serverUrl) + strlen(txCount);
  *url        = (char*)malloc(sizeof(char) * (size + 1));
  if (*url) { sprintf(*url, "%s%s", serverUrl, txCount); }
}
void get_tx_count(http_client_t client )
{
  LOG_DEBUG("\nBNS Get TXCOUNT begin\n");
  char ** url;
  build_get_tx_count_url(&url);
  char * res = client.get(url);
  LOG_DEBUG("\nBNS Get TXCOUNT end\n");
}

void build_get_server_info_url(char** url) {
  if (!url) { return; }
  char* serverUrl = SERVER_URL;
  char* txCount = "serverInfo";
  size_t size = strlen(serverUrl) + strlen(txCount);
  *url        = (char*)malloc(sizeof(char) * (size + 1));
  if (*url) { sprintf(*url, "%s%s", serverUrl, txCount); }
}
void get_server_info(http_client_t client )
{
  LOG_DEBUG("\nBNS Get serverinfo begin\n");
  char ** url;
  build_get_server_info_url(&url);
  char * res = client.get(url);
  LOG_DEBUG("\nBNS Get serverinfo end\n");
}

void build_download_merkleProof_url(clearance_order_t Clear, char* indexValue, char** url)
{
    if (!url) { return; }
  char* serverUrl = SERVER_URL;
  char* temp  = "verify/merkleProof";
  char** merkleurl;
  size_t size = bns_digits(72) + strlen(indexValue) + 4 + strlen(temp); 
  merkleurl        = (char*)malloc(sizeof(char) * (size + 1));
  sprintf(merkleurl, "%s/%lld/%s" , temp , 72,indexValue );
  size = strlen(serverUrl) + strlen(merkleurl);
  *url        = (char*)malloc(sizeof(char) * (size + 1));
  if (*url) {sprintf(*url, "%s%s", serverUrl, merkleurl); }
  merkleurl = NULL;
}
void download_merkleProof(clearance_order_t clearanceOrder, char* indexValue, http_client_t client)
{
  LOG_DEBUG("\nBNS getMerkleProof begin\n");
  char ** url;
  build_download_merkleProof_url(clearanceOrder , indexValue, &url);
  char * res = client.get(url);
  LOG_DEBUG("\nBNS getMerkleProof end\n");
}

void build_Confirm_Wallet_Address_url(char * address ,char** url)
{
    if (!url) { return; }
  char* serverUrl = SERVER_URL;
  char* temp  = "account/register/check/";
  char** confirmurl;
  size_t size = bns_digits(address) +2  + strlen(temp); 
  confirmurl        = (char*)malloc(sizeof(char) * (size + 1));
  sprintf(confirmurl, "%s%s" ,temp, address  );
  size = strlen(serverUrl) + strlen(confirmurl);
  *url        = (char*)malloc(sizeof(char) * (size + 1));
  if (*url) {sprintf(*url, "%s%s", serverUrl, confirmurl); }
  confirmurl = NULL;
  printf(*url);
}
void confirm_Wallet_Address(http_client_t client , char* address)
{
  char ** url;
  build_Confirm_Wallet_Address_url(address,&url);
  char * res = client.get(url);
}

  void build_get_done_Clearance_Order_url(char** url) 
  {
  if (!url) { return; }
  char* serverUrl = SERVER_URL;
  char* txCount = "doneClearanceOrder";
  size_t size = strlen(serverUrl) + strlen(txCount);
  *url        = (char*)malloc(sizeof(char) * (size + 1));
  if (*url) { sprintf(*url, "%s%s", serverUrl, txCount); }
  printf(*url);
}
void get_done_Clearance_Order(http_client_t client )
{
  LOG_DEBUG("\nBNS getDoneClearanceOrder begin\n");
  char ** url;
  build_get_done_Clearance_Order_url(&url);
  char * res = client.get(url);
  LOG_DEBUG("\nBNS getDoneClearanceOrder end\n");
}

void build_download_Verification_Proof_url(clearance_order_t Clear, char* indexValue, char** url)
{
    if (!url) { return; }
  char* serverUrl = SERVER_URL;
  char* temp  = "verify/verificationProof";
  char** merkleurl;
  size_t size = bns_digits(Clear) + strlen(indexValue) + 4 + strlen(temp); 
  merkleurl        = (char*)malloc(sizeof(char) * (size + 1));
  sprintf(merkleurl, "%s/%lld/%s" , temp , Clear,indexValue );
  size = strlen(serverUrl) + strlen(merkleurl);
  *url        = (char*)malloc(sizeof(char) * (size + 1));
  if (*url) {sprintf(*url, "%s%s", serverUrl, merkleurl); }
  merkleurl = NULL;
}
void download_Verification_Proof(clearance_order_t clearanceOrder, char* indexValue, http_client_t client)
{
  char cmdJson[CMD_LEN] = {0};
  sprintf(cmdJson, "{\"clearanceOrder\":\"%d\", \"indexValue\":%s}", clearanceOrder, indexValue );
  char ** url;
  build_download_Verification_Proof_url(clearanceOrder , indexValue, &url);
  char * res = client.get(url);
  FILE* fptr = fopen("practice.txt" , "w");
  if(fptr == NULL)
  {
    LOG_INFO("ERROR");
    return;
  }
  fprintf(fptr ,"%s", res);
  fclose(fptr);
}

  