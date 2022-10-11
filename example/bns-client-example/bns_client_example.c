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
  if ((exitCode = ssl_init()) != 0) { return exitCode; }

  /**
   * BNS Client initialize the bns_client_callback_t struct with each callback
   * function. Check Build the Callback Applications documents for more
   * informations
   */
  bns_client_callback_t callback = {
      .register_callback            = register_callback,
      .create_ledger_input_by_cmd   = create_ledger_input_by_cmd_callback,
      .obtain_ledger_input_response = ledger_input_response_callback,
      .obtain_binary_ledger_input_response =
          binary_ledger_input_response_callback,
      .obtain_receipt_event              = receipt_event_callback,
      .obtain_done_clearance_order_event = done_clearance_order_event_callback,
      .obtain_merkle_proof               = merkle_proof_callback,
      .get_verify_receipt_result         = verify_receipt_result_callback};

  /**
   * BNS Client initialize the receipt_dao_t struct with each receiptDao
   * function. Check Build the ReceiptDao Applications documents for more
   * informations
   */
  receipt_dao_t receiptDao = {
      .save   = receipt_cache_save,
      .delete = receipt_cache_delete,
      .findPageByClearanceOrderEqualOrLessThan =
          receipt_cache_findPageByClearanceOrderEqualOrLessThan};
  receipt_locator_t  receiptLocator = {0};
  receipt_locator_free(&receiptLocator);
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
  if ((exitCode = bns_client_init(&bnsClient, PRIVATE_KEY, ADDRESS, EMAIL,
                                  SERVER_URL, NODE_URL, &receiptDao,
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

  bns_long_t time = get_timestamp();
  printf ("\n\n\n\n\n\n");
   LOG_INFO("IT HATH BEGUN");
  int count = 0;
while(count < 1){
                LOG_INFO("am I the problem");
    if (receipt_cache_count() < RECEIPT_CACHE_SIZE) {
      /*
       * After successfully initializing the BNS Client
       * BNS Client will convert CMD to JSON data type, cmdJSON,
       * and call bns_client_ledger_input function to do the ledgerInput.
       * Check Build the CMD document for more informations
       */
      
/*

      char cmdJson[CMD_LEN] = {0};

      char *timestamp = get_timestamp_string();
      float voltage = 1.0;
      float current = 1.0;
      float power = 1.0;

      sprintf(cmdJson, "{\"deviceId\":\"%s\", \"timestamp\":%s, \"voltage\":%.6f, \"current\":%.6f, \"power\":%.6f}"
      , ADDRESS, timestamp, voltage, current, power);
      BNS_FREE(timestamp);
      
      /**
       * Call bns_client_ledger_input to do ledgerInput
       */

/*
     if ((exitCode = bns_client_ledger_input(&bnsClient, cmdJson)) != BNS_OK) {
    goto bns_client_example_fail;
      }
*/
    
    //(STUFF THAT WORKS)
    LOG_INFO("/tx/overview\n\n");
    get_tx_count(httpClient);
    LOG_INFO("\n\n");
    LOG_INFO("/serverInfo\n\n");
    get_server_info(httpClient);
    LOG_INFO("\n\n");
    confirm_Wallet_Address(httpClient, ADDRESS);
    LOG_INFO("\n\n");
    clearance_order_t co;
    LOG_INFO("GET DONE CLEARANCE ORDER");
    bns_get_done_clearance_order(&bnsClient, &co);
    LOG_INFO("\n\n");
    LOG_INFO("GET RECEIPT LOCATOR");
    bns_get_receipt_locator(&bnsClient, &receiptLocator);
    LOG_INFO("\n\n"); 
    LOG_INFO("GET MERKLEPROOF /verify/merkleProof"); 
    download_merkleProof(co, receiptLocator.indexValue,httpClient);
    LOG_INFO("\n\n"); 
    LOG_INFO("Start of verificationProof"); 
    download_Verification_Proof(35, receiptLocator.indexValue,httpClient);
    LOG_INFO("GET_DONE_CLEARANCE ORDER");
    get_done_Clearance_Order(httpClient);
    
    LOG_INFO("begin Verify Verification Proof");
    CURLVERIFY();
    //CURLVERIFYWithoutFile();
    LOG_INFO("What's the problem here?");
    count++;
    } else {
      /**
       * BNS Client verify the receipt after ledgerInput.
       * In addition, BNS Client also verify the receipt
       * when the number of receipt reaches the receipt_cache_size
       */
      LOG_INFO("is it here?");
      if ((exitCode = bns_client_verify_now(&bnsClient, 5)) != BNS_OK) {
        goto bns_client_example_fail;
      }
      count++;
      nanosleep((const struct timespec[]){{1L, 0L}}, NULL);
    }
    // different than ITM There is a login time required with BNS
    /*
    bns_long_t temp_time = get_timestamp();

    if (temp_time - time > 3600000) {

      bns_relogin(&bnsClient);
      time = temp_time;
    }
    */

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
