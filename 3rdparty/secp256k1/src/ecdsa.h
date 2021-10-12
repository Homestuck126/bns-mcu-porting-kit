/***********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                              *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_ECDSA_H
#define SECP256K1_ECDSA_H

#include <stddef.h>

#include "scalar.h"
#include "group.h"
#include "ecmult.h"

static int secp256k1_ecdsa_sig_sign(
    const secp256k1_ecmult_gen_context *ctx,
    secp256k1_scalar *r,
    secp256k1_scalar *s,
    const secp256k1_scalar *seckey,
    const secp256k1_scalar *message,
    const secp256k1_scalar *nonce,
    int *recid);

#endif /* SECP256K1_ECDSA_H */
