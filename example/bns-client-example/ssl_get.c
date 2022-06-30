#include "ssl_get.h"

#include <bns-client/util/log.h>
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#if !defined(NODE_NEED_AUTH)
#define NODE_NEED_AUTH 0
#endif

static CURL*       curl = NULL;
struct curl_slist* hs   = NULL;
//allocate space and put allocates new data while returning the additonal data
static size_t curlDataCallback(void*  chunks,
                               size_t chunkSize,
                               size_t chunksCount,
                               void*  memoryBlock) {
  //Allocate memory block
  MemoryBlock* block = (MemoryBlock*)memoryBlock;
  //size of data
  size_t additionalDataSize = chunkSize * chunksCount;
  //reallocate memory
  block->data = realloc(block->data, block->size + additionalDataSize + 1);
  //if block data DNE, error
  if (block->data == NULL) {
    LOG_ERROR("curlDataCallback() Out of memory");
    abort();
  }
  //copy chunks into this new destination
  memcpy(block->data + block->size, chunks, additionalDataSize);
  //add additionalDataSize into size
  block->size += additionalDataSize;
  //set the end data to 0
  block->data[block->size] = 0;
  //return new allocated size 
  return additionalDataSize;
}
//initalize curl
int ssl_init() {
  //set up curl
  CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
  // if res is not ok, fail
  if (res != CURLE_OK) { goto ssl_init_fail; }
  //if null, error
  if ((curl = curl_easy_init()) == NULL) { return !CURLE_OK; }
  //add APPLICATION_JSON to start of hs
  hs = curl_slist_append(hs, APPLICATION_JSON);
  //add device-type:sdk to start of hs 
  hs = curl_slist_append(hs, "device-type:sdk");
  //return
  return res;
ssl_init_fail:
  curl_slist_free_all(hs);
  curl_easy_cleanup(curl);
  return res;
}

//reset curl
int ssl_reset() {
  CURLcode res = 0;
  if (!curl) {
    if ((res = ssl_init()) != CURLE_OK) { return res; }
  }
  curl_easy_reset(curl);
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
  if ((res = curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L)) != CURLE_OK) {
    return res;
  }
#endif
  if ((res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs)) != CURLE_OK) {
    return res;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L)) != CURLE_OK) {
    return res;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L)) != CURLE_OK) {
    return res;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L)) != CURLE_OK) {
    return res;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_SSL_SESSIONID_CACHE, 0L)) !=
      CURLE_OK) {
    return res;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L)) != CURLE_OK) {
    return res;
  }
  char* filename = "./bns-cookie.txt";  // store cookie to file
  // char* filename = "";  // store cookie to memory
  if ((res = curl_easy_setopt(curl, CURLOPT_COOKIEJAR, filename)) != CURLE_OK) {
    return res;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlDataCallback)) !=
      CURLE_OK) {
    return res;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0")) !=
      CURLE_OK) {
    return res;
  }
  return res;
}
//curl cleanup
void ssl_clean() {
  curl_slist_free_all(hs);
  curl_easy_cleanup(curl);
  curl_global_cleanup();
}

// Get information from BNS Server
char* bns_get(const char* const url) {
  LOG_INFO("bns_get() begin() url=%s", url);
  CURLcode    res   = 0;
  // initalize memory block as empty
  MemoryBlock block = {.data = NULL, .size = 0};
  //if error, cleanup
  if ((res = ssl_reset()) != CURLE_OK) { goto cleanupLabel; }
  if ((res = curl_easy_setopt(curl, CURLOPT_URL, url)) != CURLE_OK) {
    goto cleanupLabel;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&block)) !=
      CURLE_OK) {
    goto cleanupLabel;
  }
  if ((res = curl_easy_perform(curl)) != CURLE_OK) { goto cleanupLabel; }
  //
  LOG_INFO("bns_get() end, content-length=%zu bytes, content=%s", block.size,
           block.data);
  //return data in block
  return block.data;

cleanupLabel:
  if (block.data) { free(block.data); }
  LOG_ERROR("bns_get() error, %s", curl_easy_strerror(res));
  return NULL;
}

// POST information to BNS Server
char* bns_post(const char* const url, const char* const postData) {
  LOG_INFO("bns_post() begin() url=%s, postData=%s", url, postData);
  CURLcode    res   = 0;
  MemoryBlock block = {.data = NULL, .size = 0};

  if ((res = ssl_reset()) != CURLE_OK) { goto cleanupLabel; }
  if ((res = curl_easy_setopt(curl, CURLOPT_URL, url)) != CURLE_OK) {
    goto cleanupLabel;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_POST, 1L)) != CURLE_OK) {
    goto cleanupLabel;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                              (long)strlen(postData))) != CURLE_OK) {
    goto cleanupLabel;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData)) !=
      CURLE_OK) {
    goto cleanupLabel;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&block)) !=
      CURLE_OK) {
    goto cleanupLabel;
  }
  if ((res = curl_easy_perform(curl)) != CURLE_OK) { goto cleanupLabel; }

  LOG_INFO("bns_post() end, content-length=%zu bytes, content=%s", block.size,
           block.data);
  return block.data;

cleanupLabel:
  if (block.data) { free(block.data); }
  LOG_ERROR("bns_post() error, %s", curl_easy_strerror(res));
  return block.data;
}

// Get information form Blockchain
char* eth_post(const char* const url, const char* const postData) {
  LOG_INFO("eth_post() begin() url=%s, postData=%s", url, postData);
  CURLcode    res   = 0;
  MemoryBlock block = {.data = NULL, .size = 0};

  if ((res = ssl_reset()) != CURLE_OK) { goto cleanupLabel; }
#if NODE_NEED_AUTH == 1
  if ((res = curl_easy_setopt(curl, CURLOPT_USERNAME, NODE_USER_NAME)) !=
      CURLE_OK) {
    goto cleanupLabel;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_PASSWORD, NODE_PASSWORD)) !=
      CURLE_OK) {
    goto cleanupLabel;
  }
#endif
  if ((res = curl_easy_setopt(curl, CURLOPT_URL, url)) != CURLE_OK) {
    goto cleanupLabel;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_POST, 1L)) != CURLE_OK) {
    goto cleanupLabel;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                              (long)strlen(postData))) != CURLE_OK) {
    goto cleanupLabel;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData)) !=
      CURLE_OK) {
    goto cleanupLabel;
  }
  if ((res = curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&block)) !=
      CURLE_OK) {
    goto cleanupLabel;
  }
  if ((res = curl_easy_perform(curl)) != CURLE_OK) { goto cleanupLabel; }

  LOG_INFO("eth_post() end, content-length=%zu bytes, content=%s", block.size,
           block.data);
  return block.data;

cleanupLabel:
  if (block.data) { free(block.data); }
  LOG_ERROR("eth_post() error, %s", curl_easy_strerror(res));
  return block.data;
}
