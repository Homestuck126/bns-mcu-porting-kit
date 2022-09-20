#include <bns-client/util/string_util.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
//equal strings check
bool bns_equals_ignore_case(const char* const a, const char* const b) {
  if (!a || !b) { return false; }
  if (a == b) { return true; }
  size_t size = strlen(a);
  if (strlen(b) != size) { return false; }
  for (size_t i = 0; i < size; i++) {
    if (tolower(a[i]) != tolower(b[i])) { return false; }
  }
  return true;
}
//equal strings check with a check that their length is < n
bool bns_equals_n_ignore_case(const char* const a,
                              const char* const b,
                              const size_t      n) {
  if (!a || !b) { return false; }
  if (a == b) { return true; }
  size_t size = strlen(a);
  if (size < n) { return false; }
  if (strlen(b) < n) { return false; }
  for (size_t i = 0; i < n; i++) {
    if (tolower(a[i]) != tolower(b[i])) { return false; }
  }
  return true;
}
//copy src to dest 
void bns_strdup(char** const dest, const char* src) {
  if (!dest || !src) { return; }
  size_t len = strlen(src) + 1;
  //allocate/reallocate based on whether dest exists
  if (*dest) {
    *dest = (char*)realloc(*dest, sizeof(char) * len);
  } else {
    *dest = (char*)malloc(sizeof(char) * len);
  }
  memcpy(*dest, src, len);
}
//removes end slash 
void remove_end_slash(char* string) {
  if (!string) { return; }
  size_t len = strlen(string);
  if (string[len - 1] == '/') { string[len - 1] = '\0'; }
}

char* bns_to_lower_case_index_value(char* a , char buffer []) {
  size_t size= strlen(a);
  for (size_t i = 0; i < size-2; i++) {
    buffer[i] =  (tolower(a[i]));
  }
  buffer[size-2] = a[size-2];
  buffer[size-1] = a[size-1];
  buffer[size] = a[size];
  return buffer;
}