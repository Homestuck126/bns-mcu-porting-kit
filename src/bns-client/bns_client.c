#include <bns-client/bns_client.h>
#include <bns-client/input/binary_ledger_input.h>
#include <bns-client/input/receipt_locator.h>
#include <bns-client/login/login.h>
#include <bns-client/register/bns_server_info.h>
#include <bns-client/util/log.h>
#include <bns-client/util/signature_util.h>
#include <bns-client/util/string_util.h>
#include <bns-client/verify/merkle_proof.h>
#include <bns-client/verify/verify.h>
#include <bns-client/verify/verify_receipt_result.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

_CHECK_RESULT
//check inital arguments and they all exist 
bns_exit_code_t check_init_args(const bns_client_t* const  bnsClient,
                                const char* const          privateKey,
                                const char* const          indexValueKey,
                                const char* const          email,
                                const char* const          serverUrl,
                                const char* const          nodeUrl,
                                const receipt_dao_t* const receiptDao,
                                const http_client_t* const httpClient) {
  
  LOG_DEBUG("check_init_args() begin");
    //check existence 
  if (!bnsClient) { return BNS_CLIENT_NULL_ERROR; }
  if (!privateKey) { return BNS_PRIVATE_KEY_NULL_ERROR; }
  if (!indexValueKey) { return BNS_INDEX_VALUE_KEY_NULL_ERROR; }
  if (strlen(indexValueKey) >= INDEX_VALUE_LEN) {
    return BNS_INDEX_VALUE_KEY_LEN_OUT_OF_RANGE_ERROR;
  }
  if (!email) { return BNS_EMAIL_NULL_ERROR; }
  if (!serverUrl) { return BNS_SERVER_URL_NULL_ERROR; }
  if (!nodeUrl) { return BNS_NODE_URL_NULL_ERROR; }
  if (!receiptDao) { return BNS_RECEIPT_DAO_NULL_ERROR; }
  if (!receiptDao->save) { return BNS_RECEIPT_DAO_SAVE_NULL_ERROR; }
  if (!receiptDao->findPageByClearanceOrderEqualOrLessThan) {
    return BNS_RECEIPT_DAO_FIND_NULL_ERROR;
  }
  if (!receiptDao->delete) { return BNS_RECEIPT_DAO_DELETE_NULL_ERROR; }
  if (!httpClient) { return BNS_CLIENT_HTTP_CLIENT_NULL_ERROR; }
  if (!httpClient->get) { return BNS_CLIENT_HTTP_CLIENT_BNS_GET_NULL_ERROR; }
  if (!httpClient->post) { return BNS_CLIENT_HTTP_CLIENT_BNS_POST_NULL_ERROR; }
  if (!httpClient->eth_post) {
    return BNS_CLIENT_HTTP_CLIENT_ETH_POST_NULL_ERROR;
  }
  return BNS_OK;
}
//
bns_exit_code_t bns_client_init(bns_client_t* const        bnsClient,
                                const char* const          privateKey,
                                const char* const          indexValueKey,
                                const char* const          email,
                                const char* const          serverUrl,
                                const char* const          nodeUrl,
                                const receipt_dao_t* const receiptDao,
                                const http_client_t* const httpClient,
                                const bns_client_callback_t* const callback) {
  LOG_INFO(
      "bns_client_init() begin, privateKey=%s, indexValueKey=%s, serverUrl=%s, "
      "nodeUrl=%s",
      privateKey, indexValueKey, serverUrl, nodeUrl);
  bns_exit_code_t exitCode;
  //check existence of initals 
  if ((exitCode = check_init_args(bnsClient, privateKey, indexValueKey, email,
                                  serverUrl, nodeUrl, receiptDao,
                                  httpClient)) != BNS_OK) {
    goto bns_client_init_fail;
  }
  //copy things into bnsClient config
  strncpy(bnsClient->config.privateKey, privateKey, PRIVATE_KEY_STR_LEN - 1);

  bns_strdup(&bnsClient->config.indexValueKey, indexValueKey);

  bns_strdup(&bnsClient->config.email, email);

  bns_strdup(&bnsClient->config.serverUrl, serverUrl);
  remove_end_slash(bnsClient->config.serverUrl);

  bns_strdup(&bnsClient->config.nodeUrl, nodeUrl);
  //initalize bnsClient
  bnsClient->receiptDao = *receiptDao;
  bnsClient->httpClient = *httpClient;
  if (callback) { bnsClient->callback = *callback; }
  //initalize walletAddress
  bnsClient->walletAddress[0] = '0';
  bnsClient->walletAddress[1] = 'x';
  //encrypt t which is empty
  char          t[]                       = " ";
  unsigned char sha3Result[SHA3_BYTE_LEN] = {0};
  bns_sha3((unsigned char*)t, (int)strlen(t), sha3Result);
  sig_t sig = {0};
  //sign sha3Result
  if ((exitCode = bns_sign(sha3Result, privateKey, &sig)) != BNS_OK) {
    goto bns_client_init_fail;
  }
  //get publicKey
  char publicKey[PUBLIC_KEY_STR_LEN] = {0};
  if ((exitCode = recover_public_key(sha3Result, &sig, publicKey)) != BNS_OK) {
    goto bns_client_init_fail;
  }
  strcpy(bnsClient->publicKey, publicKey);

  strcpy(bnsClient->walletAddress, "0x");
  //recover address as publicKey
  recover_address(publicKey, (bnsClient->walletAddress + 2));
  //set count of verify
  if (!bnsClient->verifyAfterLedgerInputCount) {
    if ((exitCode = bns_client_set_verify_after_ledger_input_count(
             bnsClient, DEFAULT_VERIFY_AFTER_LEDGER_INPUT_COUNT)) != BNS_OK) {
      goto bns_client_init_fail;
    }
  }
  //set retry count
  if (!bnsClient->maxRetryCount) {
    if ((exitCode = bns_client_set_retry_count(
             bnsClient, DEFAULT_MAX_RETRY_COUNT)) != BNS_OK) {
      goto bns_client_init_fail;
    }
  }
  //set client retry delay
  if (!bnsClient->retryDelaySec) {
    if ((exitCode = bns_client_set_retry_delay_sec(
             bnsClient, DEFAULT_RETRY_DELAY_SEC)) != BNS_OK) {
      goto bns_client_init_fail;
    }
  }
  //relogin
  if ((exitCode = bns_relogin(bnsClient)) != BNS_OK) {
    goto bns_client_init_fail;
  }

  if ((strlen(bnsClient->bnsServerInfo.contractAddress) == 0) ||
      (strlen(bnsClient->bnsServerInfo.serverWalletAddress) == 0)) {
    if ((exitCode = bns_get_server_info(bnsClient,
                                        &bnsClient->bnsServerInfo)) != BNS_OK) {
      goto bns_client_init_fail;
    }
  }

  LOG_INFO("bns_client_init() end, " BNS_CLIENT_PRINT_FORMAT,
           BNS_CLIENT_TO_PRINT_ARGS(bnsClient));
  return exitCode;

bns_client_init_fail:
  bns_client_free(bnsClient);
  LOG_ERROR("bns_client_init() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//enable post_multi
bns_exit_code_t bns_client_enable_binary_ledger_input(
    bns_client_t* const bnsClient,
    char* (*post_multi)(const char*, const bns_form_t*, const bns_form_t*)) {
      //enable post_multi?
  if (!bnsClient) { return BNS_CLIENT_NULL_ERROR; }
  if (!post_multi) { return BNS_CLIENT_HTTP_CLIENT_BNS_POST_MULTI_NULL_ERROR; }
  bnsClient->httpClient.post_multi = post_multi;
  return BNS_OK;
}
//bns client callbacks and 
bns_exit_code_t bns_client_ledger_input(const bns_client_t* const bnsClient,
                                        const char* const         cmdJson) {
  LOG_INFO("bns_client_ledger_input() begin, cmdJson=%s", cmdJson);
  bns_exit_code_t       exitCode;
  ledger_input_result_t ledgerInputResult = {0};
  receipt_locator_t     receiptLocator    = {0};
  //check existence
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_client_ledger_input_fail;
  }
  if (!cmdJson) {
    exitCode = BNS_CMD_JSON_NULL_ERROR;
    goto bns_client_ledger_input_fail;
  }
  size_t retryCount = bnsClient->maxRetryCount ? *bnsClient->maxRetryCount : 0;
  //while retryCount > 0
  do {
    if (retryCount > 0) { retryCount--; }
    //cleanup
    receipt_locator_free(&receiptLocator);
    ledger_input_result_free(&ledgerInputResult);
    //get receipt locator
    exitCode = bns_get_receipt_locator(bnsClient, &receiptLocator);
    if (exitCode != BNS_OK) { goto bns_client_ledger_input_fail; }
    //call bns post for ledgerInput
    exitCode = bns_post_ledger_input(bnsClient, cmdJson, &receiptLocator,
                                     &ledgerInputResult);
    if (exitCode == BNS_OK) { break; }
    //check indexValue and clearenceOrder for ledgerInput
    if (is_ledger_input_resend_error(exitCode)) {
      LOG_WARN("bns_client_ledger_input() resend, " BNS_EXIT_CODE_PRINT_FORMAT,
               bns_strerror(exitCode));
      if (bnsClient->retryDelaySec) { sleep(*bnsClient->retryDelaySec); }
      continue;
    }
    LOG_ERROR("bns_client_ledger_input() error, " BNS_EXIT_CODE_PRINT_FORMAT,
              bns_strerror(exitCode));
    break;
  } while (retryCount > 0);
  //check ok
  if ((exitCode == BNS_OK) || is_ledger_input_error(exitCode)) {
    //insert callback 
    if (bnsClient->callback.obtain_ledger_input_response) {
      bnsClient->callback.obtain_ledger_input_response(&receiptLocator, cmdJson,
                                                       &ledgerInputResult);
    }
  }
  if (exitCode != BNS_OK) { goto bns_client_ledger_input_fail; }
  //insert bnsClientCallbacks
  if (bnsClient->callback.obtain_receipt_event) {
    bnsClient->callback.obtain_receipt_event(ledgerInputResult.receipt);
  }
  if (bnsClient->callback.obtain_done_clearance_order_event) {
    bnsClient->callback.obtain_done_clearance_order_event(
        ledgerInputResult.doneClearanceOrder);
  }
  //cleanup
  receipt_locator_free(&receiptLocator);
  bool verifyAfterLedgerInput =
      bnsClient->verifyAfterLedgerInputCount
          ? *bnsClient->verifyAfterLedgerInputCount > 0
          : false;
  if (verifyAfterLedgerInput) {
    exitCode = bns_client_verify_by_done_co(
        bnsClient, *bnsClient->verifyAfterLedgerInputCount,
        ledgerInputResult.doneClearanceOrder);
  }
  ledger_input_result_free(&ledgerInputResult);
  LOG_INFO("bns_client_ledger_input() end");
  return exitCode;

bns_client_ledger_input_fail:
  receipt_locator_free(&receiptLocator);
  ledger_input_result_free(&ledgerInputResult);
  LOG_ERROR("bns_client_ledger_input() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//post binary ledger and check then save into receipt then insert callbacks
bns_exit_code_t bns_client_binary_ledger_input(
    const bns_client_t* const  bnsClient,
    const char* const          cmdJson,
    const binary_info_t* const binaryInfo) {
  LOG_INFO("bns_client_binary_ledger_input() begin, cmdJson=%s", cmdJson);
  bns_exit_code_t              exitCode;
  binary_ledger_input_result_t binaryLedgerInputResult = {0};
  receipt_locator_t            receiptLocator          = {0};
  //check existence 
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_client_ledger_input_fail;
  }
  if (!cmdJson) {
    exitCode = BNS_CMD_JSON_NULL_ERROR;
    goto bns_client_ledger_input_fail;
  }
  if (!bnsClient->httpClient.post_multi) {
    exitCode = BNS_BINARY_LEDGER_INPUT_IS_DISABLE_ERROR;
    goto bns_client_ledger_input_fail;
  }
  size_t retryCount = bnsClient->maxRetryCount ? *bnsClient->maxRetryCount : 0;
  do {
    //cleanup
    if (retryCount > 0) { retryCount--; }
    receipt_locator_free(&receiptLocator);
    binary_ledger_input_result_free(&binaryLedgerInputResult);
    //get receipt locator
    exitCode = bns_get_receipt_locator(bnsClient, &receiptLocator);
    if (exitCode != BNS_OK) { goto bns_client_ledger_input_fail; }
    //POST binary
    exitCode =
        bns_post_binary_ledger_input(bnsClient, cmdJson, &receiptLocator,
                                     binaryInfo, &binaryLedgerInputResult);
    //check errors
    if (exitCode == BNS_OK) { break; }
    if (is_ledger_input_resend_error(exitCode)) {
      LOG_WARN(
          "bns_client_binary_ledger_input() "
          "resend, " BNS_EXIT_CODE_PRINT_FORMAT,
          bns_strerror(exitCode));
      if (bnsClient->retryDelaySec) { sleep(*bnsClient->retryDelaySec); }
      continue;
    }
    LOG_ERROR(
        "bns_client_binary_ledger_input() error, " BNS_EXIT_CODE_PRINT_FORMAT,
        bns_strerror(exitCode));
    break;
  } while (retryCount > 0);
  //ensure ok
  if ((exitCode == BNS_OK) || is_ledger_input_error(exitCode)) {
    //insert spoClientCallbacks 
    if (bnsClient->callback.obtain_binary_ledger_input_response) {
      bnsClient->callback.obtain_binary_ledger_input_response(
          &receiptLocator, cmdJson, binaryInfo, &binaryLedgerInputResult);
    }
  }
  if (exitCode != BNS_OK) { goto bns_client_ledger_input_fail; }
  //insert callbacks 
  if (bnsClient->callback.obtain_receipt_event) {
    bnsClient->callback.obtain_receipt_event(binaryLedgerInputResult.receipt);
  }
  if (bnsClient->callback.obtain_done_clearance_order_event) {
    bnsClient->callback.obtain_done_clearance_order_event(
        binaryLedgerInputResult.doneClearanceOrder);
  }
  //clean up
  receipt_locator_free(&receiptLocator);
  //if verifyAfterLedgerInputCount < 0 or does not exist, false
  bool verifyAfterLedgerInput =
      bnsClient->verifyAfterLedgerInputCount
          ? *bnsClient->verifyAfterLedgerInputCount > 0
          : false;
  //verify by done Clearance Order 
  if (verifyAfterLedgerInput) {
    exitCode = bns_client_verify_by_done_co(
        bnsClient, *bnsClient->verifyAfterLedgerInputCount,
        binaryLedgerInputResult.doneClearanceOrder);
  }
  //cleanup
  binary_ledger_input_result_free(&binaryLedgerInputResult);
  LOG_INFO("bns_client_binary_ledger_input() end");
  return exitCode;

bns_client_ledger_input_fail:
  receipt_locator_free(&receiptLocator);
  binary_ledger_input_result_free(&binaryLedgerInputResult);
  LOG_ERROR(
      "bns_client_binary_ledger_input() error, " BNS_EXIT_CODE_PRINT_FORMAT,
      bns_strerror(exitCode));
  return exitCode;
}
//call relogin
bns_exit_code_t bns_relogin(const bns_client_t* const bnsClient) {
  LOG_INFO("bns_relogin() begin");
  bns_exit_code_t exitCode;
  //check existence 
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_relogin_fail;
  }
  //build login
  if ((exitCode = bns_login(bnsClient)) != BNS_OK) { goto bns_relogin_fail; }
  return exitCode;

bns_relogin_fail:
  LOG_ERROR("bns_relogin_fail() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}

bns_exit_code_t bns_client_verify_now(const bns_client_t* const bnsClient,
                                      const size_t              verifyCount) {
  LOG_INFO("bns_client_verify_now() begin");
  bns_exit_code_t exitCode;
  //check existence 
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_client_verify_now_fail;
  }
  clearance_order_t doneClearanceOrder = 0;
  //get done clearanceOrder 
  if ((exitCode = bns_get_done_clearance_order(
           bnsClient, &doneClearanceOrder)) != BNS_OK) {
    goto bns_client_verify_now_fail;
  }
  //verify by done CO
  if ((exitCode = bns_client_verify_by_done_co(bnsClient, verifyCount,
                                               doneClearanceOrder)) != BNS_OK) {
    goto bns_client_verify_now_fail;
  }
  LOG_INFO("bns_client_verify_now() end");
  return exitCode;

bns_client_verify_now_fail:
  LOG_ERROR("bns_client_verify_now() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}

bns_exit_code_t bns_client_verify_by_done_co(
    const bns_client_t* const bnsClient,
    const size_t              verifyCount,
    const clearance_order_t   doneCO) {
  LOG_DEBUG("bns_client_verify_by_done_co() begin");
  bns_exit_code_t exitCode = BNS_OK;
  receipt_t*      receipt  = NULL;
  //check existence
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_client_verify_by_done_co_fail;
  }
  size_t toVerifyCount = verifyCount;
  size_t receiptCount  = 0;
  //
  do {
    //count is equal to less than toVerifyCount 
    size_t count =
        toVerifyCount <= VERIFY_BATCH_SIZE ? toVerifyCount : VERIFY_BATCH_SIZE;
    //reallocate space for receipt
    receipt = (receipt_t*)malloc(sizeof(receipt_t) * count);
    memset(receipt, 0, sizeof(receipt_t) * count);
    //put info into findPageByClearanceOrder
    bnsClient->receiptDao.findPageByClearanceOrderEqualOrLessThan(
        doneCO, 0, count, receipt, &receiptCount);
    for (size_t i = 0; i < receiptCount; i++) {
      merkle_proof_t          merkleProof         = {0};
      verify_receipt_result_t verifyReceiptResult = {0};
      //verify data
      exitCode =
          verify(bnsClient, &receipt[i], &merkleProof, &verifyReceiptResult);
          //
      if (bnsClient->callback.get_verify_receipt_result) {
        bnsClient->callback.get_verify_receipt_result
        (&receipt[i], &merkleProof, &verifyReceiptResult);
      }
      //frees up the memory used in the receipt and merkle proof
      bnsClient->receiptDao.delete(&receipt[i]);
      merkle_proof_free(&merkleProof);
      verify_receipt_result_free(&verifyReceiptResult);
    }
    BNS_FREE(receipt);
    toVerifyCount -= receiptCount;
  } while (toVerifyCount > 0 && receiptCount != 0);
  LOG_DEBUG("bns_client_verify_by_done_co() end");
  return exitCode;

bns_client_verify_by_done_co_fail:
  LOG_ERROR("bns_client_verify_by_done_co() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}
//get done clearanceOrder from bns client server 
bns_exit_code_t bns_get_done_clearance_order(
    const bns_client_t* const bnsClient, clearance_order_t* const doneCO) {
  size_t count = 0;
bns_get_done_clearance_order_beg:
  LOG_INFO("bns_get_done_clearance_order() begin");
  bns_exit_code_t exitCode = BNS_OK;
  cJSON*          root     = NULL;
  char*           url      = NULL;
  char*           res      = NULL;
  //check existence 
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_get_done_clearance_order_fail;
  }
  if (!bnsClient->config.serverUrl) {
    exitCode = BNS_SERVER_URL_NULL_ERROR;
    goto bns_get_done_clearance_order_fail;
  }
  if (!doneCO) {
    exitCode = BNS_DONE_CO_NULL_ERROR;
    goto bns_get_done_clearance_order_fail;
  }
  //allocate space for url 
  url = (char*)malloc(sizeof(char) *
                      (strlen(bnsClient->config.serverUrl) +
                       strlen(LEDGER_DONE_CLEARANCE_ORDER_PATH) + 1));
  strcpy(url, bnsClient->config.serverUrl);
  strcat(url, LEDGER_DONE_CLEARANCE_ORDER_PATH);
  //get url using bnsClient
  res = bnsClient->httpClient.get(url);
  if (!res) {
    exitCode = BNS_GET_DONE_CO_RESPONSE_NULL_ERROR;
    goto bns_get_done_clearance_order_fail;
  }
  BNS_FREE(url);
  //turns res into a decimal
  *doneCO = strtoll(res, NULL, 10);

  BNS_FREE(res);

  LOG_INFO("bns_get_done_clearance_order() end, doneCO=%lld", *doneCO);
  return exitCode;
bns_get_done_clearance_order_fail:
  if (url) { BNS_FREE(url); }

  cJSON_Delete(root);
  LOG_ERROR("bns_get_done_clearance_order() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  if (bnsClient && bnsClient->maxRetryCount) {
    if (count++ < *bnsClient->maxRetryCount) {
      LOG_DEBUG("bns_get_done_clearance_order() retry, count=%ld", count);
      if (bnsClient->retryDelaySec) 
      { sleep(*bnsClient->retryDelaySec); }
      goto bns_get_done_clearance_order_beg;
    }
  }
  return exitCode;
}
//Set the count of verify After Ledger Input
bns_exit_code_t bns_client_set_verify_after_ledger_input_count(
    bns_client_t* const bnsClient, const size_t count) {
  if (!bnsClient) { return BNS_CLIENT_NULL_ERROR; }
  if (!bnsClient->verifyAfterLedgerInputCount) {
    bnsClient->verifyAfterLedgerInputCount = (size_t*)malloc(sizeof(size_t));
  }
  *bnsClient->verifyAfterLedgerInputCount = count;
  return BNS_OK;
}
//Set retryDelaysize and if BNS client exists for size
bns_exit_code_t bns_client_set_retry_count(bns_client_t* const bnsClient,
                                           const size_t        count) {
  if (!bnsClient) { return BNS_CLIENT_NULL_ERROR; }
  if (!bnsClient->maxRetryCount) {
    bnsClient->maxRetryCount = (size_t*)malloc(sizeof(size_t));
  }
  *bnsClient->maxRetryCount = count;
  return BNS_OK;
}
//Set retryDelaysize and if BNS client exists for seconds
bns_exit_code_t bns_client_set_retry_delay_sec(bns_client_t* const bnsClient,
                                               const size_t        sec) {
  if (!bnsClient) { return BNS_CLIENT_NULL_ERROR; }
  if (!bnsClient->retryDelaySec) {
    bnsClient->retryDelaySec = (size_t*)malloc(sizeof(size_t));
  }
  *bnsClient->retryDelaySec = sec;
  return BNS_OK;
}
// Frees up data used by BNS_Client
void bns_client_free(bns_client_t* const bnsClient) {
  if (bnsClient) {
    if (bnsClient->config.indexValueKey) {
      BNS_FREE(bnsClient->config.indexValueKey);
    }
    if (bnsClient->config.email) { BNS_FREE(bnsClient->config.email); }
    if (bnsClient->config.serverUrl) { BNS_FREE(bnsClient->config.serverUrl); }
    if (bnsClient->config.nodeUrl) { BNS_FREE(bnsClient->config.nodeUrl); }
    if (bnsClient->verifyAfterLedgerInputCount) {
      BNS_FREE(bnsClient->verifyAfterLedgerInputCount);
    }
    if (bnsClient->maxRetryCount) { BNS_FREE(bnsClient->maxRetryCount); }
    if (bnsClient->retryDelaySec) { BNS_FREE(bnsClient->retryDelaySec); }
  }
}
