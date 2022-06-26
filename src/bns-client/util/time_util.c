#include <bns-client/util/log.h>
#include <bns-client/util/numeric_util.h>
#include <bns-client/util/time_util.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
// get timestamp as string
char* get_timestamp_string() {
  LOG_DEBUG("get_timestamp_string() begin");
  long int timestampLong = time(NULL);
  char*    timestamp =
      (char*)malloc(sizeof(char) * (bns_digits(timestampLong) + 3 + 1));
  sprintf(timestamp, "%ld000", timestampLong);
  LOG_DEBUG("get_timestamp_string() end, timestamp=%s", timestamp);
  return timestamp;
}
//get timestamp and return in form of long template
bns_long_t get_timestamp() {
  LOG_DEBUG("get_timestamp_string() begin");
  bns_long_t timestamp = time(NULL);
  timestamp *= 1000L;
  LOG_DEBUG("get_timestamp_string() end, timestamp=%lld", timestamp);
  return timestamp;
}
