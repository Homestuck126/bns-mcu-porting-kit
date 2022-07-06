#include <bns-client/contract/clearance_record_cache.h>
#include <string.h>
#include <time.h>

static clearance_record_cache_t
    clearanceRecordCache[CLEARANCE_RECORD_CACHE_SIZE] = {0};
// put the clearanceRecord of the corresponding order found in the cache in the clearanceRecord variable
void get_clearance_record_cache(const clearance_order_t   clearanceOrder,
                                clearance_record_t* const clearanceRecord) {
  //if clearanceOrder is 0, error
  if (clearanceOrder == 0) { return; }
  //if clearance Record exists
  if (!clearanceRecord) { return; }
  //find the clearanceorder
  for (size_t i = 0; i < CLEARANCE_RECORD_CACHE_SIZE; i++) {
    //if clearanceCache locations clearanceOrder is the same is the given clearance Order, 
    if (clearanceRecordCache[i].clearanceRecord.clearanceOrder ==
        clearanceOrder) {
          //copies the clearencerecord of the corresponding clearance order into clearanceRecord from cache
      memcpy(clearanceRecord, &clearanceRecordCache[i].clearanceRecord,
             sizeof(clearance_record_t));
             // removes timestamp of the cache
      clearanceRecordCache[i].timestamp = time(NULL);
    }
  }
}
// insert clearance or replace earliest clearance
void set_clearance_record_cache(
    const clearance_record_t* const clearanceRecord) {
      // if clearanceRecord does not exists
  if (!clearanceRecord) { return; }
      //if clearance order has nothing, return
  if (clearanceRecord->clearanceOrder == 0) { return; }
//find an open space for the new clearanceRecord in cache and set it
  bool placed = false;
  for (size_t i = 0; i < CLEARANCE_RECORD_CACHE_SIZE; i++) {
    //if the clearanceorder at location is empty, 
    if (clearanceRecordCache[i].clearanceRecord.clearanceOrder == 0) {
      //copy into clearance record, clearance record 
      memcpy(&clearanceRecordCache[i].clearanceRecord, clearanceRecord,
             sizeof(clearance_record_t));
      //timestap set to null
      clearanceRecordCache[i].timestamp = time(NULL);
      placed                            = true;
      break;
    }
  }
  // if no more space,
  if (!placed) {
    //set min timestamp to the first timestamp and minIndex to 0 
    size_t     minIndex     = 0;
    bns_long_t minTimestamp = clearanceRecordCache[0].timestamp;
    //find min timestamp
    for (size_t i = 1; i < CLEARANCE_RECORD_CACHE_SIZE; i++) {
      if (clearanceRecordCache[i].timestamp < minTimestamp) {
        minIndex     = i;
        minTimestamp = clearanceRecordCache[i].timestamp;
      }
    }
    //copy clearance record into the minimum index
    memcpy(&clearanceRecordCache[minIndex].clearanceRecord, clearanceRecord,
           sizeof(clearance_record_t));
    //set timestamp at min index to current time
    clearanceRecordCache[minIndex].timestamp = time(NULL);
  }
}
//reset cache
void reset_clearance_record_cache() {
  memset(&clearanceRecordCache, 0, sizeof(clearanceRecordCache));
}
