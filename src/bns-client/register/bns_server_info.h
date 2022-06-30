#ifndef BNS_C_CLIENT_BNS_SERVER_INFO_H
#define BNS_C_CLIENT_BNS_SERVER_INFO_H

#include <bns-client/core/core.h>

#define LEDGER_BNS_SERVER_INFO_PATH "/serverInfo"

#define BNS_SERVER_INFO_PRINT_FORMAT                     \
  "bnsServerInfo=bns_server_info_t(contractAddress=%s, " \
  "serverWalletAddress=%s)"
#define BNS_SERVER_INFO_TO_PRINT_ARGS(bnsServerInfo) \
  bnsServerInfo->contractAddress, (bnsServerInfo)->serverWalletAddress
//built serverUrl 
void build_get_bns_server_info_url(char** url, const char* serverUrl);
//get server info from bns
_CHECK_RESULT
bns_exit_code_t bns_get_server_info(const bns_client_t* bnsClient,
                                    bns_server_info_t*  bnsServerInfo);
//save bns ServerInfo and check it works 
_CHECK_RESULT
bns_exit_code_t check_and_parse_bns_server_info_response(
    const char* res, bns_server_info_t* bnsServerInfo);

#endif  // BNS_C_CLIENT_BNS_SERVER_INFO_H
