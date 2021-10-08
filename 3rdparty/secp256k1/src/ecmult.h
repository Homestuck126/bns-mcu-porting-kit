/***********************************************************************
 * Copyright (c) 2013, 2014, 2017 Pieter Wuille, Andrew Poelstra       *
 * Distributed under the MIT software license, see the accompanying    *
 * file COPYING or https://www.opensource.org/licenses/mit-license.php.*
 ***********************************************************************/

#ifndef SECP256K1_ECMULT_H
#define SECP256K1_ECMULT_H

#include "group.h"
#include "scalar.h"

typedef struct {
  /* For accelerating the computation of a*P + b*G: */
  secp256k1_ge_storage (*pre_g)[];    /* odd multiples of the generator */
  secp256k1_ge_storage (*pre_g_128)[]; /* odd multiples of 2^128*generator */
} secp256k1_ecmult_context;

static const size_t SECP256K1_ECMULT_CONTEXT_PREALLOCATED_SIZE;
static void secp256k1_ecmult_context_init(secp256k1_ecmult_context *ctx);
static void secp256k1_ecmult_context_build(secp256k1_ecmult_context *ctx, void **prealloc);
static void secp256k1_ecmult_context_clear(secp256k1_ecmult_context *ctx);
static int secp256k1_ecmult_context_is_built(const secp256k1_ecmult_context *ctx);

/** Double multiply: R = na*A + ng*G */
static void secp256k1_ecmult(
    const secp256k1_ecmult_context *ctx,
    secp256k1_gej *r,
    const secp256k1_gej *a,
    const secp256k1_scalar *na,
    const secp256k1_scalar *ng);

#endif /* SECP256K1_ECMULT_H */
