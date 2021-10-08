#include <bns-client/bns_client.h>
#include <bns-client/input/binary_ledger_input.h>
#include <bns-client/input/ledger_input.h>
#include <bns-client/input/receipt_locator.h>
#include <bns-client/register/bns_server_info.h>
#include <bns-client/register/register.h>
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
bns_exit_code_t check_init_args(const bns_client_t *const bnsClient,
                                const char *const privateKey,
                                const char *const indexValueKey,
                                const char *const email,
                                const char *const serverUrl,
                                const char *const nodeUrl,
                                const receipt_dao_t *const receiptDao,
                                const http_client_t *const httpClient) {
  LOG_DEBUG("check_init_args() begin");
  if (!bnsClient) {
    return BNS_CLIENT_NULL_ERROR;
  }
  if (!privateKey) {
    return BNS_PRIVATE_KEY_NULL_ERROR;
  }
  if (!indexValueKey) {
    return BNS_INDEX_VALUE_KEY_NULL_ERROR;
  }
  if (strlen(indexValueKey) >= INDEX_VALUE_LEN) {
    return BNS_INDEX_VALUE_KEY_LEN_OUT_OF_RANGE_ERROR;
  }
  if (!email) {
    return BNS_EMAIL_NULL_ERROR;
  }
  if (!serverUrl) {
    return BNS_SERVER_URL_NULL_ERROR;
  }
  if (!nodeUrl) {
    return BNS_NODE_URL_NULL_ERROR;
  }
  if (!receiptDao) {
    return BNS_RECEIPT_DAO_NULL_ERROR;
  }
  if (!receiptDao->save) {
    return BNS_RECEIPT_DAO_SAVE_NULL_ERROR;
  }
  if (!receiptDao->findPageByClearanceOrderEqualOrLessThan) {
    return BNS_RECEIPT_DAO_FIND_NULL_ERROR;
  }
  if (!receiptDao->delete) {
    return BNS_RECEIPT_DAO_DELETE_NULL_ERROR;
  }
  if (!httpClient) {
    return BNS_CLIENT_HTTP_CLIENT_NULL_ERROR;
  }
  if (!httpClient->get) {
    return BNS_CLIENT_HTTP_CLIENT_BNS_GET_NULL_ERROR;
  }
  if (!httpClient->post) {
    return BNS_CLIENT_HTTP_CLIENT_BNS_POST_NULL_ERROR;
  }
  if (!httpClient->eth_post) {
    return BNS_CLIENT_HTTP_CLIENT_ETH_POST_NULL_ERROR;
  }
  return BNS_OK;
}

bns_exit_code_t bns_client_init(bns_client_t *const bnsClient,
                                const char *const privateKey,
                                const char *const indexValueKey,
                                const char *const email,
                                const char *const serverUrl,
                                const char *const nodeUrl,
                                const receipt_dao_t *const receiptDao,
                                const http_client_t *const httpClient,
                                const bns_client_callback_t *const callback) {
  LOG_INFO(
      "bns_client_init() begin, privateKey=%s, indexValueKey=%s, serverUrl=%s, "
      "nodeUrl=%s",
      privateKey, indexValueKey, serverUrl, nodeUrl);
  bns_exit_code_t exitCode;
  if ((exitCode = check_init_args(bnsClient, privateKey, indexValueKey, email,
                                  serverUrl, nodeUrl, receiptDao,
                                  httpClient)) != BNS_OK) {
    goto bns_client_init_fail;
  }

  strncpy(bnsClient->config.privateKey, privateKey, PRIVATE_KEY_STR_LEN - 1);

  bns_strdup(&bnsClient->config.indexValueKey, indexValueKey);

  bns_strdup(&bnsClient->config.email, email);

  bns_strdup(&bnsClient->config.serverUrl, serverUrl);
  remove_end_slash(bnsClient->config.serverUrl);

  bns_strdup(&bnsClient->config.nodeUrl, nodeUrl);

  bnsClient->receiptDao = *receiptDao;
  bnsClient->httpClient = *httpClient;
  if (callback) {
    bnsClient->callback = *callback;
  }

  bnsClient->walletAddress[0] = '0';
  bnsClient->walletAddress[1] = 'x';

  char t[] = " ";
  unsigned char sha3Result[SHA3_BYTE_LEN] = {0};
  bns_sha3((unsigned char *)t, (int)strlen(t), sha3Result);
  sig_t sig = {0};
  if ((exitCode = bns_sign(sha3Result, privateKey, &sig)) != BNS_OK) {
    goto bns_client_init_fail;
  }
  char publicKey[PUBLIC_KEY_STR_LEN] = {0};
  if ((exitCode = recover_public_key(sha3Result, &sig, publicKey)) != BNS_OK) {
    goto bns_client_init_fail;
  }
  strcpy(bnsClient->publicKey, publicKey);

  strcpy(bnsClient->walletAddress, "0x");
  recover_address(publicKey, (bnsClient->walletAddress + 2));

  if (!bnsClient->verifyAfterLedgerInputCount) {
    if ((exitCode = bns_client_set_verify_after_ledger_input_count(
             bnsClient, DEFAULT_VERIFY_AFTER_LEDGER_INPUT_COUNT)) != BNS_OK) {
      goto bns_client_init_fail;
    }
  }

  if (!bnsClient->maxRetryCount) {
    if ((exitCode = bns_client_set_retry_count(
             bnsClient, DEFAULT_MAX_RETRY_COUNT)) != BNS_OK) {
      goto bns_client_init_fail;
    }
  }

  if (!bnsClient->retryDelaySec) {
    if ((exitCode = bns_client_set_retry_delay_sec(
             bnsClient, DEFAULT_RETRY_DELAY_SEC)) != BNS_OK) {
      goto bns_client_init_fail;
    }
  }

  bool result = false;

  if ((exitCode = bns_check_register(bnsClient, &result)) != BNS_OK) {
    goto bns_client_init_fail;
  }

  if ((strlen(bnsClient->bnsServerInfo.contractAddress) == 0) ||
      (strlen(bnsClient->bnsServerInfo.serverWalletAddress) == 0)) {
    if ((exitCode = bns_post_server_info(
             bnsClient, &bnsClient->bnsServerInfo)) != BNS_OK) {
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

bns_exit_code_t bns_client_enable_binary_ledger_input(
    bns_client_t *const bnsClient,
    char *(*post_multi)(const char *, const bns_form_t *, const bns_form_t *)) {
  if (!bnsClient) {
    return BNS_CLIENT_NULL_ERROR;
  }
  if (!post_multi) {
    return BNS_CLIENT_HTTP_CLIENT_BNS_POST_MULTI_NULL_ERROR;
  }
  bnsClient->httpClient.post_multi = post_multi;
  return BNS_OK;
}

bns_exit_code_t bns_client_ledger_input(const bns_client_t *const bnsClient,
                                        const char *const cmdJson) {
  LOG_INFO("bns_client_ledger_input() begin, cmdJson=%s", cmdJson);
  bns_exit_code_t exitCode;
  ledger_input_result_t ledgerInputResult = {0};
  receipt_locator_t receiptLocator = {0};
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_client_ledger_input_fail;
  }
  if (!cmdJson) {
    exitCode = BNS_CMD_JSON_NULL_ERROR;
    goto bns_client_ledger_input_fail;
  }
  size_t retryCount = bnsClient->maxRetryCount ? *bnsClient->maxRetryCount : 0;
  do {
    if (retryCount > 0) {
      retryCount--;
    }
    receipt_locator_free(&receiptLocator);
    ledger_input_result_free(&ledgerInputResult);
    exitCode = bns_post_receipt_locator(bnsClient, &receiptLocator);
    if (exitCode != BNS_OK) {
      goto bns_client_ledger_input_fail;
    }
    exitCode = bns_post_ledger_input(bnsClient, cmdJson, &receiptLocator,
                                     &ledgerInputResult);
    if (exitCode == BNS_OK) {
      break;
    }
    if (is_ledger_input_resend_error(exitCode)) {
      LOG_WARN("bns_client_ledger_input() resend, " BNS_EXIT_CODE_PRINT_FORMAT,
               bns_strerror(exitCode));
      if (bnsClient->retryDelaySec) {
        sleep(*bnsClient->retryDelaySec);
      }
      continue;
    }
    LOG_ERROR("bns_client_ledger_input() error, " BNS_EXIT_CODE_PRINT_FORMAT,
              bns_strerror(exitCode));
    break;
  } while (retryCount > 0);
  if ((exitCode == BNS_OK) || is_ledger_input_error(exitCode)) {
    if (bnsClient->callback.obtain_ledger_input_response) {
      bnsClient->callback.obtain_ledger_input_response(&receiptLocator, cmdJson,
                                                       &ledgerInputResult);
    }
  }
  if (exitCode != BNS_OK) {
    goto bns_client_ledger_input_fail;
  }
  if (bnsClient->callback.obtain_receipt_event) {
    bnsClient->callback.obtain_receipt_event(ledgerInputResult.receipt);
  }
  if (bnsClient->callback.obtain_done_clearance_order_event) {
    bnsClient->callback.obtain_done_clearance_order_event(
        ledgerInputResult.doneClearanceOrder);
  }
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

bns_exit_code_t bns_client_binary_ledger_input(
    const bns_client_t *const bnsClient, const char *const cmdJson,
    const binary_info_t *const binaryInfo) {
  LOG_INFO("bns_client_binary_ledger_input() begin, cmdJson=%s", cmdJson);
  bns_exit_code_t exitCode;
  binary_ledger_input_result_t binaryLedgerInputResult = {0};
  receipt_locator_t receiptLocator = {0};
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
    if (retryCount > 0) {
      retryCount--;
    }
    receipt_locator_free(&receiptLocator);
    binary_ledger_input_result_free(&binaryLedgerInputResult);
    exitCode = bns_post_receipt_locator(bnsClient, &receiptLocator);
    if (exitCode != BNS_OK) {
      goto bns_client_ledger_input_fail;
    }
    exitCode =
        bns_post_binary_ledger_input(bnsClient, cmdJson, &receiptLocator,
                                     binaryInfo, &binaryLedgerInputResult);
    if (exitCode == BNS_OK) {
      break;
    }
    if (is_ledger_input_resend_error(exitCode)) {
      LOG_WARN(
          "bns_client_binary_ledger_input() "
          "resend, " BNS_EXIT_CODE_PRINT_FORMAT,
          bns_strerror(exitCode));
      if (bnsClient->retryDelaySec) {
        sleep(*bnsClient->retryDelaySec);
      }
      continue;
    }
    LOG_ERROR(
        "bns_client_binary_ledger_input() error, " BNS_EXIT_CODE_PRINT_FORMAT,
        bns_strerror(exitCode));
    break;
  } while (retryCount > 0);
  if ((exitCode == BNS_OK) || is_ledger_input_error(exitCode)) {
    if (bnsClient->callback.obtain_binary_ledger_input_response) {
      bnsClient->callback.obtain_binary_ledger_input_response(
          &receiptLocator, cmdJson, binaryInfo, &binaryLedgerInputResult);
    }
  }
  if (exitCode != BNS_OK) {
    goto bns_client_ledger_input_fail;
  }
  if (bnsClient->callback.obtain_receipt_event) {
    bnsClient->callback.obtain_receipt_event(binaryLedgerInputResult.receipt);
  }
  if (bnsClient->callback.obtain_done_clearance_order_event) {
    bnsClient->callback.obtain_done_clearance_order_event(
        binaryLedgerInputResult.doneClearanceOrder);
  }
  receipt_locator_free(&receiptLocator);
  bool verifyAfterLedgerInput =
      bnsClient->verifyAfterLedgerInputCount
          ? *bnsClient->verifyAfterLedgerInputCount > 0
          : false;
  if (verifyAfterLedgerInput) {
    exitCode = bns_client_verify_by_done_co(
        bnsClient, *bnsClient->verifyAfterLedgerInputCount,
        binaryLedgerInputResult.doneClearanceOrder);
  }
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

bns_exit_code_t bns_client_verify_now(const bns_client_t *const bnsClient,
                                      const size_t verifyCount) {
  LOG_INFO("bns_client_verify_now() begin");
  bns_exit_code_t exitCode;
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_client_verify_now_fail;
  }
  clearance_order_t doneClearanceOrder = 0;
  if ((exitCode = bns_post_done_clearance_order(
           bnsClient, &doneClearanceOrder)) != BNS_OK) {
    goto bns_client_verify_now_fail;
  }
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
    const bns_client_t *const bnsClient, const size_t verifyCount,
    const clearance_order_t doneCO) {
  LOG_DEBUG("bns_client_verify_by_done_co() begin");
  bns_exit_code_t exitCode = BNS_OK;
  receipt_t *receipt = NULL;
  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_client_verify_by_done_co_fail;
  }
  size_t toVerifyCount = verifyCount;
  size_t receiptCount = 0;
  do {
    size_t count =
        toVerifyCount <= VERIFY_BATCH_SIZE ? toVerifyCount : VERIFY_BATCH_SIZE;
    receipt = (receipt_t *)malloc(sizeof(receipt_t) * count);
    memset(receipt, 0, sizeof(receipt_t) * count);
    bnsClient->receiptDao.findPageByClearanceOrderEqualOrLessThan(
        doneCO, 0, count, receipt, &receiptCount);
    for (size_t i = 0; i < receiptCount; i++) {
      merkle_proof_t merkleProof = {0};
      verify_receipt_result_t verifyReceiptResult = {0};
      exitCode =
          verify(bnsClient, &receipt[i], &merkleProof, &verifyReceiptResult);
      if (bnsClient->callback.get_verify_receipt_result) {
        bnsClient->callback.get_verify_receipt_result(&receipt[i], &merkleProof,
                                                      &verifyReceiptResult);
      }
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
  if (receipt) {
    BNS_FREE(receipt);
  }
  LOG_ERROR("bns_client_verify_by_done_co() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}

bns_exit_code_t bns_post_done_clearance_order(
    const bns_client_t *const bnsClient, clearance_order_t *const doneCO) {
  size_t count = 0;
bns_get_done_clearance_order_beg:
  LOG_INFO("bns_get_done_clearance_order() begin");
  bns_exit_code_t exitCode = BNS_OK;
  cJSON *root = NULL;
  char *reqJson = NULL;
  char *url = NULL;
  char *res = NULL;

  if (!bnsClient) {
    exitCode = BNS_CLIENT_NULL_ERROR;
    goto bns_get_done_clearance_order_fail;
  }
  if (!bnsClient->config.privateKey) {
    exitCode = BNS_PRIVATE_KEY_NULL_ERROR;
    goto bns_get_done_clearance_order_fail;
  }
  if (!bnsClient->config.indexValueKey) {
    exitCode = BNS_INDEX_VALUE_KEY_NULL_ERROR;
    goto bns_get_done_clearance_order_fail;
  }
  if (!bnsClient->config.serverUrl) {
    exitCode = BNS_SERVER_URL_NULL_ERROR;
    goto bns_get_done_clearance_order_fail;
  }
  if (!bnsClient->walletAddress) {
    exitCode = BNS_ADDRESS_NULL_ERROR;
    goto bns_get_done_clearance_order_fail;
  }
  if (!doneCO) {
    exitCode = BNS_DONE_CO_NULL_ERROR;
    goto bns_get_done_clearance_order_fail;
  }

  url = (char *)malloc(sizeof(char) *
                       (strlen(bnsClient->config.serverUrl) +
                        strlen(LEDGER_DONE_CLEARANCE_ORDER_PATH) +
                        strlen(bnsClient->config.indexValueKey) + 1));
  strcpy(url, bnsClient->config.serverUrl);
  strcat(url, LEDGER_DONE_CLEARANCE_ORDER_PATH);

  char *toSignMessage = "doneClearanceOrder";

  unsigned char shaResult[SHA3_BYTE_LEN] = {0};
  bns_sha3((unsigned char *)toSignMessage, strlen(toSignMessage), shaResult);
  sig_t sig = {0};
  exitCode = bns_sign(shaResult, bnsClient->config.privateKey, &sig);
  if (exitCode != BNS_OK) {
    goto bns_get_done_clearance_order_fail;
  }

  root = cJSON_CreateObject();
  cJSON *sig_item = cJSON_CreateObject();
  cJSON_AddItemToObject(root, "address",
                        cJSON_CreateString(bnsClient->walletAddress));
  cJSON_AddItemToObject(root, "toSignMessage",
                        cJSON_CreateString(toSignMessage));
  cJSON_AddItemToObject(root, "sig", sig_item);
  cJSON_AddItemToObject(sig_item, "r", cJSON_CreateString(sig.r));
  cJSON_AddItemToObject(sig_item, "s", cJSON_CreateString(sig.s));
  cJSON_AddItemToObject(sig_item, "v", cJSON_CreateString(sig.v));
  reqJson = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);

  res = bnsClient->httpClient.post(url, reqJson);
  if (!res) {
    exitCode = BNS_GET_DONE_CO_RESPONSE_NULL_ERROR;
    goto bns_get_done_clearance_order_fail;
  }
  BNS_FREE(url);
  BNS_FREE(reqJson);

  *doneCO = strtoll(res, NULL, 10);

  BNS_FREE(res);

  LOG_INFO("bns_get_done_clearance_order() end, doneCO=%lld", *doneCO);
  return exitCode;
bns_get_done_clearance_order_fail:
  if (res) {
    BNS_FREE(res);
  }
  if (url) {
    BNS_FREE(url);
  }
  if (reqJson) {
    BNS_FREE(reqJson);
  }
  cJSON_Delete(root);
  LOG_ERROR("bns_get_done_clearance_order() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  if (bnsClient && bnsClient->maxRetryCount) {
    if (count++ < *bnsClient->maxRetryCount) {
      LOG_DEBUG("bns_get_done_clearance_order() retry, count=%ld", count);
      if (bnsClient->retryDelaySec) {
        sleep(*bnsClient->retryDelaySec);
      }
      goto bns_get_done_clearance_order_beg;
    }
  }
  return exitCode;
}

bns_exit_code_t bns_client_set_verify_after_ledger_input_count(
    bns_client_t *const bnsClient, const size_t count) {
  if (!bnsClient) {
    return BNS_CLIENT_NULL_ERROR;
  }
  if (!bnsClient->verifyAfterLedgerInputCount) {
    bnsClient->verifyAfterLedgerInputCount = (size_t *)malloc(sizeof(size_t));
  }
  *bnsClient->verifyAfterLedgerInputCount = count;
  return BNS_OK;
}

bns_exit_code_t bns_client_set_retry_count(bns_client_t *const bnsClient,
                                           const size_t count) {
  if (!bnsClient) {
    return BNS_CLIENT_NULL_ERROR;
  }
  if (!bnsClient->maxRetryCount) {
    bnsClient->maxRetryCount = (size_t *)malloc(sizeof(size_t));
  }
  *bnsClient->maxRetryCount = count;
  return BNS_OK;
}

bns_exit_code_t bns_client_set_retry_delay_sec(bns_client_t *const bnsClient,
                                               const size_t sec) {
  if (!bnsClient) {
    return BNS_CLIENT_NULL_ERROR;
  }
  if (!bnsClient->retryDelaySec) {
    bnsClient->retryDelaySec = (size_t *)malloc(sizeof(size_t));
  }
  *bnsClient->retryDelaySec = sec;
  return BNS_OK;
}

void bns_client_free(bns_client_t *const bnsClient) {
  if (bnsClient) {
    if (bnsClient->config.indexValueKey) {
      BNS_FREE(bnsClient->config.indexValueKey);
    }
    if (bnsClient->config.email) {
      BNS_FREE(bnsClient->config.email);
    }
    if (bnsClient->config.serverUrl) {
      BNS_FREE(bnsClient->config.serverUrl);
    }
    if (bnsClient->config.nodeUrl) {
      BNS_FREE(bnsClient->config.nodeUrl);
    }
    if (bnsClient->verifyAfterLedgerInputCount) {
      BNS_FREE(bnsClient->verifyAfterLedgerInputCount);
    }
    if (bnsClient->maxRetryCount) {
      BNS_FREE(bnsClient->maxRetryCount);
    }
    if (bnsClient->retryDelaySec) {
      BNS_FREE(bnsClient->retryDelaySec);
    }
  }
}
