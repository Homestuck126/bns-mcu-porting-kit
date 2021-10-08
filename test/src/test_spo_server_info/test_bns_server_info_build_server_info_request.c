#include <assert.h>
#include <bns-client/register/bns_server_info.h>
#include <string.h>

#include "../mock_data/mock_ok_data.h"

void test_ok() {
  // then
  char *json = NULL;
  assert(build_server_info_request(MOCK_CALLER_ADDRESS_OK, MOCK_PRIVATE_KEY_OK,
                                   &json) == BNS_OK);
  assert(strcmp(json, MOCK_SERVER_INFO_REQUEST_JSON_OK) == 0);

  // clean
  BNS_FREE(json);
}

void test_BNS_ADDRESS_NULL_ERROR() {
  assert(build_server_info_request(NULL, NULL, NULL) == BNS_ADDRESS_NULL_ERROR);
}

void test_BNS_PRIVATE_KEY_NULL_ERROR() {
  assert(build_server_info_request(MOCK_CALLER_ADDRESS_OK, NULL, NULL) ==
         BNS_PRIVATE_KEY_NULL_ERROR);
}

void test_BNS_SERVER_INFO_REQUEST_JSON_NULL_ERROR() {
  assert(build_server_info_request(MOCK_CALLER_ADDRESS_OK, MOCK_PRIVATE_KEY_OK,
                                   NULL) ==
         BNS_SERVER_INFO_REQUEST_JSON_NULL_ERROR);
}

int main() {
  test_ok();
  test_BNS_ADDRESS_NULL_ERROR();
  test_BNS_PRIVATE_KEY_NULL_ERROR();
  test_BNS_SERVER_INFO_REQUEST_JSON_NULL_ERROR();
  return 0;
}