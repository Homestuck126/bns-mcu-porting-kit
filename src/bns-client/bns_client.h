#ifndef BNS_CLIENT_BNS_CLIENT_H
#define BNS_CLIENT_BNS_CLIENT_H

#include <bns-client/core/core.h>

#define LEDGER_DONE_CLEARANCE_ORDER_PATH "/doneClearanceOrder"

#define BNS_CLIENT_PRINT_FORMAT                                       \
  "bnsClient=bns_client_t(config=bns_client_config_t(privateKey=%s, " \
  "indexValueKey=%s, email=%s"                                        \
  "serverUrl=%s, nodeUrl=%s), publicKey=%s, "                         \
  "walletAddress=%s, " BNS_SERVER_INFO_PRINT_FORMAT                   \
  ", receiptDao=receipt_dao_t(save=%p, "                              \
  "findPageByClearanceOrderEqualOrLessThan=%p, delete=%p), "          \
  "httpClient=http_client_t(get=%p, post=%p, eth_post=%p), "          \
  "callback=%p)"
#define BNS_CLIENT_TO_PRINT_ARGS(bnsClient)                            \
  bnsClient->config.privateKey, (bnsClient)->config.indexValueKey,     \
      (bnsClient)->config.email, (bnsClient)->config.serverUrl,        \
      (bnsClient)->config.nodeUrl, (bnsClient)->publicKey,             \
      (bnsClient)->walletAddress,                                      \
      BNS_SERVER_INFO_TO_PRINT_ARGS((&(bnsClient)->bnsServerInfo)),    \
      (bnsClient)->receiptDao.save,                                    \
      (bnsClient)->receiptDao.findPageByClearanceOrderEqualOrLessThan, \
      (bnsClient)->receiptDao.delete, (bnsClient)->httpClient.get,     \
      (bnsClient)->httpClient.post, (bnsClient)->httpClient.eth_post,  \
      &(bnsClient)->callback

#define DEFAULT_VERIFY_AFTER_LEDGER_INPUT_COUNT 2

#define DEFAULT_MAX_RETRY_COUNT 5

#define DEFAULT_RETRY_DELAY_SEC 5

#define VERIFY_BATCH_SIZE 1
//check initalized arguments and they all exist 
_CHECK_RESULT
bns_exit_code_t bns_client_init(bns_client_t*                bnsClient,
                                const char*                  privateKey,
                                const char*                  indexValueKey,
                                const char*                  email,
                                const char*                  serverUrl,
                                const char*                  nodeUrl,
                                const receipt_dao_t*         receiptDao,
                                const http_client_t*         httpClient,
                                const bns_client_callback_t* callback);
//enable post_multi
_CHECK_RESULT
bns_exit_code_t bns_client_enable_binary_ledger_input(
    bns_client_t* bnsClient,
    char* (*post_multi)(const char*, const bns_form_t*, const bns_form_t*));

_CHECK_RESULT
bns_exit_code_t bns_client_ledger_input(const bns_client_t* bnsClient,
                                        const char*         cmdJson);

_CHECK_RESULT
bns_exit_code_t bns_client_binary_ledger_input(const bns_client_t*  bnsClient,
                                               const char*          cmdJson,
                                               const binary_info_t* binaryInfo);

_CHECK_RESULT
bns_exit_code_t bns_client_verify_now(const bns_client_t* bnsClient,
                                      size_t              verifyCount);

_CHECK_RESULT
bns_exit_code_t bns_get_done_clearance_order(const bns_client_t* bnsClient,
                                             clearance_order_t*  doneCO);

_CHECK_RESULT
bns_exit_code_t bns_client_verify_by_done_co(const bns_client_t* bnsClient,
                                             size_t              verifyCount,
                                             clearance_order_t   doneCO);
//Set the count of verify After Ledger Input
_CHECK_RESULT
bns_exit_code_t bns_client_set_verify_after_ledger_input_count(
    bns_client_t* bnsClient, size_t count);
//Set retryDelaysize and if BNS client exists for size
_CHECK_RESULT
bns_exit_code_t bns_client_set_retry_count(bns_client_t* bnsClient,
                                           size_t        count);
//Set retryDelaysize and if BNS client exists for seconds
_CHECK_RESULT
bns_exit_code_t bns_client_set_retry_delay_sec(bns_client_t* bnsClient,
                                               size_t        sec);
//call relogin
_CHECK_RESULT
bns_exit_code_t bns_relogin(const bns_client_t* bnsClient);
// Frees up data used by BNS_Client
void bns_client_free(bns_client_t* bnsClient);

#endif  // BNS_CLIENT_BNS_CLIENT_H
