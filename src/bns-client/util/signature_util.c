#include <bns-client/util/byte_util.h>
#include <bns-client/util/crypto/keccak256.h>
#include <bns-client/util/log.h>
#include <bns-client/util/numeric_util.h>
#include <bns-client/util/signature_util.h>
#include <bns-client/util/string_util.h>
#include <secp256k1_recovery.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MESSAGE_PREFIX_1 0x19
#define MESSAGE_PREFIX_2 "Ethereum Signed Message:\n"

void bns_sha3(const unsigned char* toSignData,
              size_t               size,
              unsigned char*       sha3Result) {
  if (!toSignData || (size == 0) || !sha3Result) { return; }
  SHA3_CTX ctx;
  keccak_init(&ctx);
  keccak_update(&ctx, toSignData, size);
  keccak_final(&ctx, sha3Result);
}

void bns_sha3_prefix(const unsigned char* toSignData,
                     size_t               size,
                     unsigned char*       sha3Result) {
  //if toSignData does not exist, size = 0, sha3Result does not exist, return
  if (!toSignData || (size == 0) || !sha3Result) { return; }
  //length is size of tosigndata and prefix
  size_t newSize = 1 + strlen(MESSAGE_PREFIX_2) +
                   bns_digits(strlen((char*)toSignData)) +
                   strlen((char*)toSignData);
  char* message = (char*)malloc(sizeof(char) * (newSize + 1));
  //put "0x19Ethereum Signed Message:"\ntoSignData lengthtoSigndata
  sprintf(message, "%c%s%lu%s", MESSAGE_PREFIX_1, MESSAGE_PREFIX_2,
          strlen((char*)toSignData), (char*)toSignData);
  SHA3_CTX ctx;
  keccak_init(&ctx);
  keccak_update(&ctx, (unsigned char*)message, newSize);
  keccak_final(&ctx, sha3Result);
  if (message) { BNS_FREE(message); }
}

bns_exit_code_t bns_sign(const unsigned char* sha3Result,
                         const char*          privateKey,
                         sig_t*               sig) {
  if (!sha3Result || !privateKey || !sig) { return BNS_SIGN_ERROR; }
  char          tempPrivateKey[PRIVATE_KEY_STR_LEN] = {0};
  unsigned char bytePrivateKey[PRIVATE_KEY_BYTES]   = {0};

  size_t privateKeyLen = strlen(privateKey);
  if (privateKeyLen <= (PRIVATE_KEY_STR_LEN - 1)) {
    memset(tempPrivateKey, '0', (PRIVATE_KEY_STR_LEN - 1 - privateKeyLen));
    strcat(tempPrivateKey, privateKey);
  } else if (privateKeyLen > (PRIVATE_KEY_STR_LEN - 1)) {
    strncpy(tempPrivateKey, privateKey, PRIVATE_KEY_STR_LEN - 1);
  }

  bns_hex_to_byte(tempPrivateKey, PRIVATE_KEY_STR_LEN - 1, bytePrivateKey);

  secp256k1_context* context;
  context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN |
                                     SECP256K1_CONTEXT_VERIFY);  // NOLINT
  secp256k1_ecdsa_recoverable_signature signature;
  if (secp256k1_ecdsa_sign_recoverable(context, &signature, sha3Result,
                                       bytePrivateKey, NULL, NULL) == 0) {
    secp256k1_context_destroy(context);
    return BNS_SIGN_ERROR;
  }
  unsigned char sigByte[BYTES_64] = {0};
  int           v                 = 0;
  secp256k1_ecdsa_recoverable_signature_serialize_compact(context, sigByte, &v,
                                                          &signature);
  v += 27;

  bns_byte_to_hex(sigByte, BYTES_32, sig->r);
  bns_byte_to_hex(sigByte + BYTES_32, BYTES_32, sig->s);
  sprintf(sig->v, "%02x", v);

  LOG_DEBUG("bns_sign() R=%s, S=%s, V=%s", sig->r, sig->s, sig->v);
  secp256k1_context_destroy(context);

  return BNS_OK;
}

bns_exit_code_t recover_public_key(const unsigned char* const shaResult,
                                   const sig_t* const         sig,
                                   char* const                publicKey) {
  if (!shaResult || !sig || !publicKey) { return BNS_RECOVER_PUBLIC_KEY_ERROR; }
  int v;
  v = (int)strtol(sig->v, NULL, 16);
  v -= 27;

  unsigned char sigRAndV[HASH_BYTES * 2] = {0};
  bns_hex_to_byte(sig->r, SIG_R_STR_LEN - 1, sigRAndV);
  bns_hex_to_byte(sig->s, SIG_S_STR_LEN - 1, sigRAndV + HASH_BYTES);
  secp256k1_context* context;
  context = secp256k1_context_create(SECP256K1_CONTEXT_SIGN |
                                     SECP256K1_CONTEXT_VERIFY);  // NOLINT
  secp256k1_ecdsa_recoverable_signature rawSig;
  if (!secp256k1_ecdsa_recoverable_signature_parse_compact(context, &rawSig,
                                                           sigRAndV, v)) {
    secp256k1_context_destroy(context);
    return BNS_RECOVER_PUBLIC_KEY_ERROR;
  }
  secp256k1_pubkey rawPubkey;
  if (!secp256k1_ecdsa_recover(context, &rawPubkey, &rawSig, shaResult)) {
    secp256k1_context_destroy(context);
    return BNS_RECOVER_PUBLIC_KEY_ERROR;
  }
  size_t        public_key_len = PUBLIC_KEY_BYTES + 1;
  unsigned char serializedPubkey[PUBLIC_KEY_BYTES + 1];
  secp256k1_ec_pubkey_serialize(context, serializedPubkey, &public_key_len,
                                &rawPubkey,
                                SECP256K1_EC_UNCOMPRESSED);  // NOLINT

  bns_byte_to_hex(serializedPubkey + 1, PUBLIC_KEY_BYTES, publicKey);
  LOG_DEBUG("recover_public_key() end, publicKey={%s}", publicKey);
  secp256k1_context_destroy(context);
  return BNS_OK;
}

void recover_address(const char* const publicKey, char* const address) {
  if (!publicKey || !address) { return; }
  unsigned char publicKeyBytes[PUBLIC_KEY_BYTES]   = {0};
  unsigned char sha3Result[HASH_BYTES]             = {0};
  unsigned char recoverAddressBytes[ADDRESS_BYTES] = {0};

  bns_hex_to_byte(publicKey, PUBLIC_KEY_STR_LEN - 1, publicKeyBytes);
  bns_sha3(publicKeyBytes, PUBLIC_KEY_BYTES, sha3Result);
  for (int i = 0; i < ADDRESS_BYTES; i++) {
    recoverAddressBytes[i] = sha3Result[i + 12];
  }
  bns_byte_to_hex(recoverAddressBytes, ADDRESS_BYTES, address);
  LOG_DEBUG("recover_address() end, address=%s", address);
}

bns_exit_code_t verify_signature(const char* const  address,
                                 const char* const  toSignData,
                                 const sig_t* const sig) {
  LOG_DEBUG("verify_signature() begin, address=%s, toSignData=%s, ", address,
            toSignData);
  //if address, toSignData or sig do not exist
  if (!address || !toSignData || !sig) { return BNS_VERIFY_SIGNATURE_ERROR; }
  bns_exit_code_t exitCode = BNS_OK;
  {
    //initalize public key and recover address
    char publicKey[PUBLIC_KEY_STR_LEN]   = {0};
    char recoverAddress[ADDRESS_STR_LEN] = {0};
    //intalize shaSignData
    unsigned char shaSignData[SHA3_STR_LEN] = {0};
    bns_sha3_prefix((unsigned char*)toSignData, strlen(toSignData),
                    shaSignData);

    if (recover_public_key(shaSignData, sig, publicKey) != BNS_OK) {
      return BNS_VERIFY_SIGNATURE_ERROR;
    }

    recover_address(publicKey, recoverAddress);
    bool result;
    if (strncmp(address, "0x", 2) == 0) {
      result = bns_equals_ignore_case(&address[2], recoverAddress);
    } else {
      result = bns_equals_ignore_case(address, recoverAddress);
    }
    LOG_DEBUG("verify_signature() end, result=%s", result ? "true" : "false");
    if (result == true) {
      exitCode = BNS_OK;
    } else {
      exitCode = BNS_VERIFY_SIGNATURE_ERROR;
    }
  }
  if (exitCode == BNS_VERIFY_SIGNATURE_ERROR) {
    char publicKey[PUBLIC_KEY_STR_LEN]   = {0};
    char recoverAddress[ADDRESS_STR_LEN] = {0};

    unsigned char shaSignData[SHA3_STR_LEN] = {0};
    bns_sha3((unsigned char*)toSignData, strlen(toSignData), shaSignData);

    if (recover_public_key(shaSignData, sig, publicKey) != BNS_OK) {
      return BNS_VERIFY_SIGNATURE_ERROR;
    }

    recover_address(publicKey, recoverAddress);
    bool result;
    if (strncmp(address, "0x", 2) == 0) {
      result = bns_equals_ignore_case(&address[2], recoverAddress);
    } else {
      result = bns_equals_ignore_case(address, recoverAddress);
    }
    LOG_DEBUG("verify_signature() end, result=%s", result ? "true" : "false");
    if (result == true) {
      exitCode = BNS_OK;
    } else {
      exitCode = BNS_VERIFY_SIGNATURE_ERROR;
    }
  }
  return exitCode;
}
