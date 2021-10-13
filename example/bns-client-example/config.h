#ifndef BNS_C_CLIENT_EXAMPLE_CONFIG_H
#define BNS_C_CLIENT_EXAMPLE_CONFIG_H

/**
 * Please fill in the 128 bits (32 bytes) Hex encoded
 * private key which generated by VANITY-ETH
 */
#define PRIVATE_KEY "eed0efbcaf0a1e7cbbe3bee16ccad5de288e245a5474f0d02cdd72ae40125f32"

 /**
  * Please fill in the index value of data source.
  * The index value is the index of clearance.
  * You can use device ID instead, if data source is coming from device.
  * But make sure the index value must be unique.
  */
#define INDEX_VALUE_KEY "ITM_BNS_SDK_C_EXAMPLE"

  /** Please fill in the EMAIL of BNS Account */
#define EMAIL ""

/**
 * Please fill in the URL of BNS Server
 */
#define SERVER_URL "https://bns.itrustmachines.com/"  // TODO use prod server

 /**
  * Please fill in the Node URL of Rinkey which generated by Infura
  */
#define NODE_URL "https://mainnet.infura.io/v3/4b3c9e58870344cea4907f747454da5b"

#endif  // BNS_C_CLIENT_EXAMPLE_CONFIG_H

  /** Check the quickstarts document for more informations
   * PRIVATE_KEY : Generated by VANITY-ETH.
   * INDEX_VALUE_KEY : The index value is the index of clearance, please ensure
   * index value is unique NODE_URL : In order to get the onchain proof, we use
   * Rinkeby blockchain for our testing enviroment.
   */
