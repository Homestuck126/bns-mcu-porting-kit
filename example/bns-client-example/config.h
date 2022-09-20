#ifndef BNS_C_CLIENT_EXAMPLE_CONFIG_H
#define BNS_C_CLIENT_EXAMPLE_CONFIG_H

/**
 * Please fill in the 128 bits (32 bytes) Hex encoded
 * private key which generated by VANITY-ETH
 */
#define PRIVATE_KEY \
  "8a0f05496f925fde6816e1518f935e1d84b97670853e4f1ac1ed6b9b9d8f11bb"

/**
 * Please fill in the address. (e.g. 0x0000000000000000000000000000000000000000)
 */
#define ADDRESS          "0xF11f103CF88be3010fcEc835F5A973A0d847Fc35"

//#define INDEX_VALUE_KEY "0xF11f103CF88be3010fcEc835F5A973A0d847Fc35"
/** Please fill in the EMAIL of BNS Account */
#define EMAIL "aaron08820@gmail.com"

/**
 * Please fill in the URL of BNS Server
 */
#define SERVER_URL ""  // TODO use prod server

/**
 * Please fill in the Node URL of Rinkey which generated by Infura
 */
#define NODE_URL "https://mainnet.infura.io/v3/2cf7805f13764fe192eb0f02a194f6b1"

#endif  // BNS_C_CLIENT_EXAMPLE_CONFIG_H

/** Check the quickstarts document for more informations
 * PRIVATE_KEY : Generated by VANITY-ETH.
 * INDEX_VALUE_KEY : The index value is the index of clearance, please ensure
 * index value is unique 
 * NODE_URL : In order to get the onchain proof, we use
 * Rinkeby blockchain for our testing enviroment.
 */
