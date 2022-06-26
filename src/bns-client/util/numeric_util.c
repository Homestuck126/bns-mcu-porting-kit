#include <bns-client/util/numeric_util.h>
#include <stdio.h>
//number of digits for the int
size_t bns_digits(long long int num) {
  size_t count = 0;
  do {
    count++;
    num /= 10;
  } while (num != 0);
  return count;
}
//change long long to int
char* bns_lltos(const long long int num) {
  size_t size = bns_digits(num);
  if (num < 0) { size += 1; }
  char* buffer = (char*)malloc(sizeof(char) * (size + 1));
  if (buffer) { sprintf(buffer, "%lld", num); }
  return buffer;
}
