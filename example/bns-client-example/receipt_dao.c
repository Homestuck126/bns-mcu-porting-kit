#include "receipt_dao.h"

#include <bns-client/util/log.h>
#include <string.h>

/**
 * declare the static variable to store receipt in memory
 */
static receipt_t* receiptPtr[RECEIPT_CACHE_SIZE] = {0};

/**
 * store receipt in memory
 */
void receipt_cache_save(const receipt_t* receipt) {
  LOG_DEBUG("receipt_cache_save() begin");
  // for i less than the max size
  for (int i = 0; i < RECEIPT_CACHE_SIZE; i++) {
    //store receipts 
    if (!receiptPtr[i]) {
      receipt_t* _receipt = (receipt_t*)malloc(sizeof(receipt_t));
      strcpy(_receipt->callerAddress, receipt->callerAddress);
#if defined(RECEIPT_TIMESTAMP_IS_LONG)
      _receipt->timestamp = receipt->timestamp;
#else
      strcpy(_receipt->timestamp, receipt->timestamp);
#endif
      strcpy(_receipt->cmd, receipt->cmd);
      strcpy(_receipt->indexValue, receipt->indexValue);
      strcpy(_receipt->metadata, receipt->metadata);
      _receipt->clearanceOrder = receipt->clearanceOrder;
#if defined(RECEIPT_TIMESTAMP_IS_LONG)
      _receipt->timestampSPO = receipt->timestampSPO;
#else
      strcpy(_receipt->timestampSPO, receipt->timestampSPO);
#endif
      strcpy(_receipt->result, receipt->result);
      strcpy(_receipt->sigClient.r, receipt->sigClient.r);
      strcpy(_receipt->sigClient.s, receipt->sigClient.s);
      strcpy(_receipt->sigClient.v, receipt->sigClient.v);
      strcpy(_receipt->sigServer.r, receipt->sigServer.r);
      strcpy(_receipt->sigServer.s, receipt->sigServer.s);
      strcpy(_receipt->sigServer.v, receipt->sigServer.v);
      receiptPtr[i] = _receipt;
      LOG_DEBUG("receipt_cache_save() end, index=%d", i);
      return;
    }
  }
  LOG_ERROR("receipt_cache_save() error, cache out of space");
}

/**
 * use clearanceOrder to find out which receipts to be verified
 */
void receipt_cache_findPageByClearanceOrderEqualOrLessThan(
    const clearance_order_t clearanceOrder,
    const size_t            page,
    const size_t            pageSize,
    receipt_t*              outputReceipt,
    size_t*                 outputSize) {
  LOG_DEBUG("receipt_cache_findPageByClearanceOrderEqualOrLessThan() begin");
  if (!outputReceipt) {
    LOG_ERROR(
        "receipt_cache_findPageByClearanceOrderEqualOrLessThan() error, "
        "outputReceipt=NULL");
    return;
  }
  if (!outputSize) {
    LOG_ERROR(
        "receipt_cache_findPageByClearanceOrderEqualOrLessThan() error, "
        "outputSize=NULL");
    return;
  }
  //calculate offset
  size_t       size          = 0;
  size_t       currentOffset = 0;
  const size_t offset        = page * pageSize;

  int i;
  //for all receipts in receipt cache
  for (i = 0; i < RECEIPT_CACHE_SIZE; i++) {
    if (size >= pageSize) { break; }
    if (!receiptPtr[i]) { break; }
    //if this clearanceOrder is not the latest one
    if (receiptPtr[i]->clearanceOrder <= clearanceOrder) {
      //if currentOffset is not greater
      if (currentOffset >= offset) {
        //copy receipt into outputReceipt
        strcpy(outputReceipt[size].callerAddress, receiptPtr[i]->callerAddress);
#if defined(RECEIPT_TIMESTAMP_IS_LONG)
        outputReceipt[size].timestamp = receiptPtr[i]->timestamp;
#else
        strcpy(outputReceipt[size].timestamp, receiptPtr[i]->timestamp);
#endif
        strcpy(outputReceipt[size].cmd, receiptPtr[i]->cmd);
        strcpy(outputReceipt[size].indexValue, receiptPtr[i]->indexValue);
        strcpy(outputReceipt[size].metadata, receiptPtr[i]->metadata);
        outputReceipt[size].clearanceOrder = receiptPtr[i]->clearanceOrder;
#if defined(RECEIPT_TIMESTAMP_IS_LONG)
        outputReceipt[size].timestampSPO = receiptPtr[i]->timestampSPO;
#else
        strcpy(outputReceipt[size].timestampSPO, receiptPtr[i]->timestampSPO);
#endif
        strcpy(outputReceipt[size].result, receiptPtr[i]->result);
        strcpy(outputReceipt[size].sigClient.r, receiptPtr[i]->sigClient.r);
        strcpy(outputReceipt[size].sigClient.s, receiptPtr[i]->sigClient.s);
        strcpy(outputReceipt[size].sigClient.v, receiptPtr[i]->sigClient.v);

        strcpy(outputReceipt[size].sigServer.r, receiptPtr[i]->sigServer.r);
        strcpy(outputReceipt[size].sigServer.s, receiptPtr[i]->sigServer.s);
        strcpy(outputReceipt[size].sigServer.v, receiptPtr[i]->sigServer.v);
        size++;
      } else {
        currentOffset++;
      }
    }
  }
  *outputSize = size;
  if (*outputSize == 1) {
    if (outputReceipt[0].clearanceOrder == 0) { LOG_ERROR("error"); }
  }
  LOG_DEBUG(
      "receipt_cache_findPageByClearanceOrderEqualOrLessThan() end, "
      "outputSize=%ld",
      *outputSize);
}

/**
 * delete the verified receipts
 */
void receipt_cache_delete(const receipt_t* receipt) {
  LOG_DEBUG("receipt_cache_delete() begin");
  //for all receipts 
  for (int i = 0; i < RECEIPT_CACHE_SIZE; i++) {
    if (!receiptPtr[i]) { continue; }
    //if clearanceOrder is the same as current and indexValue is the same, deleate current,  
    if ((receiptPtr[i]->clearanceOrder == receipt->clearanceOrder) &&
        (strcmp(receiptPtr[i]->indexValue, receipt->indexValue) == 0)) {
      // deallocated and delete recept ptr
      free(receiptPtr[i]);
      receiptPtr[i] = NULL;
      LOG_DEBUG("receipt_cache_delete() delete index=%d", i);
      // shift everything to the left 1
      for (int j = i; j < (RECEIPT_CACHE_SIZE - 1); j++) {
        receiptPtr[j] = receiptPtr[j + 1];
        if (!receiptPtr[j + 1]) { break; }
      }
      receiptPtr[RECEIPT_CACHE_SIZE - 1] = NULL;
    }
  }
  LOG_DEBUG("receipt_cache_delete() end");
}

/* Get the number of receipt */
int receipt_cache_count() {
  LOG_DEBUG("receipt_cache_count() begin");
  int count = 0;
  for (int i = 0; i < RECEIPT_CACHE_SIZE; i++) {
    if (receiptPtr[i]) { count++; }
  }
  LOG_DEBUG("receipt_cache_count() end");
  return count;
}