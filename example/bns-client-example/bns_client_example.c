#include <bns-client/bns_client.h>
#include <bns-client/input/receipt_locator.h>
#include <bns-client/util/log.h>
#include <bns-client/util/string_util.h>
#include <bns-client/util/time_util.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "callback.h"
#include "config.h"
#include "receipt_dao.h"
#include "ssl_get.h"

int bns_client_example() {
  LOG_INFO("bns_client_example() begin");
  bns_exit_code_t exitCode;
  if ((exitCode = ssl_init()) != 0) {
    return exitCode;
  }

  /**
   * BNS Client initialize the bns_client_callback_t struct with each callback
   * function. Check Build the Callback Applications documents for more
   * informations
   */
  bns_client_callback_t callback = {
      .register_callback = register_callback,
      .create_ledger_input_by_cmd = create_ledger_input_by_cmd_callback,
      .obtain_ledger_input_response = ledger_input_response_callback,
      .obtain_binary_ledger_input_response =
          binary_ledger_input_response_callback,
      .obtain_receipt_event = receipt_event_callback,
      .obtain_done_clearance_order_event = done_clearance_order_event_callback,
      .obtain_merkle_proof = merkle_proof_callback,
      .get_verify_receipt_result = verify_receipt_result_callback};

  /**
   * BNS Client initialize the receipt_dao_t struct with each receiptDao
   * function. Check Build the ReceiptDao Applications documents for more
   * informations
   */
  receipt_dao_t receiptDao = {
      .save = receipt_cache_save,
      .delete = receipt_cache_delete,
      .findPageByClearanceOrderEqualOrLessThan =
          receipt_cache_findPageByClearanceOrderEqualOrLessThan};

  /**
   * BNS Client initialize http_client_t struct with httpClient function.
   * Check ssl_get.c and ssl_get.h for more httpClient information
   */
  http_client_t httpClient = {
      .get = bns_get, .post = bns_post, .eth_post = eth_post};

  /**
   * To estabish the ledgerInput and Verification service with BNS Server
   * BNS Client will call bns_client_init function to initialize
   * the Callback, ReceiptDao, HttpClient, and configuration file
   */
  bns_client_t bnsClient = {0};
  if ((exitCode = bns_client_init(&bnsClient, PRIVATE_KEY, INDEX_VALUE_KEY,
                                  EMAIL, SERVER_URL, NODE_URL, &receiptDao,
                                  &httpClient, &callback)) != BNS_OK) {
    goto bns_client_example_fail;
  }

  /**
   * set the number of receipts to be verified after doing ledgerInput
   */
  if ((exitCode = bns_client_set_verify_after_ledger_input_count(
           &bnsClient, 2)) != BNS_OK) {
    goto bns_client_example_fail;
  }

  /**
   * set the maximum retry count of failed ledgerInput
   */
  if ((exitCode = bns_client_set_retry_count(&bnsClient, 5)) != BNS_OK) {
    goto bns_client_example_fail;
  }

  /**
   * set the delay time for each retry
   */
  if ((exitCode = bns_client_set_retry_delay_sec(&bnsClient, 5)) != BNS_OK) {
    goto bns_client_example_fail;
  }

  while (true) {
    if (receipt_cache_count() < RECEIPT_CACHE_SIZE) {
      /**
       * After successfully initializing the BNS Client
       * BNS Client will convert CMD to JSON data type, cmdJSON,
       * and call bns_client_ledger_input function to do the ledgerInput.
       * Check Build the CMD document for more informations
       */
      char cmdJson[CMD_LEN] = {0};
      char *timestamp = get_timestamp_string();

      sprintf(cmdJson, "{\"deviceId\":\"%s\",\"timestamp\":%s}",
              INDEX_VALUE_KEY, timestamp);
      BNS_FREE(timestamp);

      /**
       * Call bns_client_ledger_input to do ledgerInput
       */
      if ((exitCode = bns_client_ledger_input(&bnsClient, cmdJson)) != BNS_OK) {
        goto bns_client_example_fail;
      }
    } else {
      /**
       * BNS Client verify the receipt after ledgerInput.
       * In addition, BNS Client also verify the receipt
       * when the number of receipt reaches the receipt_cache_size
       */
      if ((exitCode = bns_client_verify_now(&bnsClient, 5)) != BNS_OK) {
        goto bns_client_example_fail;
      }
      nanosleep((const struct timespec[]){{1L, 0L}}, NULL);
    }
  }
  while (receipt_cache_count() != 0) {
    if ((exitCode = bns_client_verify_now(&bnsClient, 5)) != BNS_OK) {
      goto bns_client_example_fail;
    }
    LOG_INFO("bns_client_example() wait");
    nanosleep((const struct timespec[]){{1L, 0L}}, NULL);
  }

  bns_client_free(&bnsClient);
  ssl_clean();
  LOG_INFO("bns_client_example() end");
  return 0;
bns_client_example_fail:
  bns_client_free(&bnsClient);
  ssl_clean();
  LOG_ERROR("bns_client_example() error, " BNS_EXIT_CODE_PRINT_FORMAT,
            bns_strerror(exitCode));
  return exitCode;
}

int main() {
  LOG_INFO("main() begin");
  bns_exit_code_t exitCode = bns_client_example();
  if (exitCode != BNS_OK) {
    LOG_ERROR("main() error, " BNS_EXIT_CODE_PRINT_FORMAT,
              bns_strerror(exitCode));
  }
  LOG_INFO("main() end");
  return exitCode;
}
