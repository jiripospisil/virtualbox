/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2021 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef OPENSSL_ESSERR_H
# define OPENSSL_ESSERR_H
# pragma once

# include <openssl/opensslconf.h>
# include <openssl/symhacks.h>
# include <openssl/cryptoerr_legacy.h>

/*
 * ESS reason codes.
 */
# define ESS_R_EMPTY_ESS_CERT_ID_LIST                     107
# define ESS_R_ESS_CERT_DIGEST_ERROR                      103
# define ESS_R_ESS_CERT_ID_NOT_FOUND                      104
# define ESS_R_ESS_CERT_ID_WRONG_ORDER                    105
# define ESS_R_ESS_DIGEST_ALG_UNKNOWN                     106
# define ESS_R_ESS_SIGNING_CERTIFICATE_ERROR              102
# define ESS_R_ESS_SIGNING_CERT_ADD_ERROR                 100
# define ESS_R_ESS_SIGNING_CERT_V2_ADD_ERROR              101
# define ESS_R_MISSING_SIGNING_CERTIFICATE_ATTRIBUTE      108

#endif
