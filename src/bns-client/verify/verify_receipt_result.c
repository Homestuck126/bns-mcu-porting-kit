#include <bns-client/verify/verify_receipt_result.h>
//frees data
void verify_receipt_result_free(
    verify_receipt_result_t* const verifyReceiptResult) {
  if (verifyReceiptResult) {
    if (verifyReceiptResult->indexValue) {
      BNS_FREE(verifyReceiptResult->indexValue);
    }
    if (verifyReceiptResult->timestamp) {
      BNS_FREE(verifyReceiptResult->timestamp);
    }
    if (verifyReceiptResult->status) { BNS_FREE(verifyReceiptResult->status); }
    if (verifyReceiptResult->description) {
      BNS_FREE(verifyReceiptResult->description);
    }
  }
}
