#ifndef BNS_C_CLIENT_SIGNATURE_UTIL_H
#define BNS_C_CLIENT_SIGNATURE_UTIL_H

#include <bns-client/core/core.h>
//encrypt toSignData
void bns_sha3(const unsigned char* toSignData,
              size_t               size,
              unsigned char*       sha3Result);
//encrypted toSignData and message. 
void bns_sha3_prefix(const unsigned char* toSignData,
                     size_t               size,
                     unsigned char*       sha3Result);
//create signature
_CHECK_RESULT
bns_exit_code_t bns_sign(const unsigned char* sha3Result,
                         const char*          privateKey,
                         sig_t*               sig);
//verify signature via address
_CHECK_RESULT
bns_exit_code_t verify_signature(const char*  address,
                                 const char*  toSignData,
                                 const sig_t* sig);
//gets public key from signature
_CHECK_RESULT
bns_exit_code_t recover_public_key(const unsigned char* shaResult,
                                   const sig_t*         sig,
                                   char*                publicKey);
//get address from publicKey
void recover_address(const char* publicKey, char* address);

#endif  // BNS_C_CLIENT_SIGNATURE_UTIL_H
