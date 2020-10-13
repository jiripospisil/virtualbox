/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2017 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/err.h>
#include "e_dasync_err.h"

#ifndef OPENSSL_NO_ERR

static ERR_STRING_DATA DASYNC_str_functs[] = {
    {ERR_PACK(0, DASYNC_F_BIND_DASYNC, 0), "bind_dasync"},
    {ERR_PACK(0, DASYNC_F_CIPHER_AES_128_CBC_CODE, 0), ""},
    {ERR_PACK(0, DASYNC_F_DASYNC_AES128_CBC_HMAC_SHA1_INIT_KEY, 0), ""},
    {ERR_PACK(0, DASYNC_F_DASYNC_AES128_INIT_KEY, 0), ""},
    {ERR_PACK(0, DASYNC_F_DASYNC_BN_MOD_EXP, 0), ""},
    {ERR_PACK(0, DASYNC_F_DASYNC_CIPHER_INIT_KEY_HELPER, 0),
     "dasync_cipher_init_key_helper"},
    {ERR_PACK(0, DASYNC_F_DASYNC_MOD_EXP, 0), ""},
    {ERR_PACK(0, DASYNC_F_DASYNC_PRIVATE_DECRYPT, 0), ""},
    {ERR_PACK(0, DASYNC_F_DASYNC_PRIVATE_ENCRYPT, 0), ""},
    {ERR_PACK(0, DASYNC_F_DASYNC_PUBLIC_DECRYPT, 0), ""},
    {ERR_PACK(0, DASYNC_F_DASYNC_PUBLIC_ENCRYPT, 0), ""},
    {0, NULL}
};

static ERR_STRING_DATA DASYNC_str_reasons[] = {
    {ERR_PACK(0, 0, DASYNC_R_INIT_FAILED), "init failed"},
    {0, NULL}
};

#endif

static int lib_code = 0;
static int error_loaded = 0;

static int ERR_load_DASYNC_strings(void)
{
    if (lib_code == 0)
        lib_code = ERR_get_next_error_library();

    if (!error_loaded) {
#ifndef OPENSSL_NO_ERR
        ERR_load_strings(lib_code, DASYNC_str_functs);
        ERR_load_strings(lib_code, DASYNC_str_reasons);
#endif
        error_loaded = 1;
    }
    return 1;
}

static void ERR_unload_DASYNC_strings(void)
{
    if (error_loaded) {
#ifndef OPENSSL_NO_ERR
        ERR_unload_strings(lib_code, DASYNC_str_functs);
        ERR_unload_strings(lib_code, DASYNC_str_reasons);
#endif
        error_loaded = 0;
    }
}

static void ERR_DASYNC_error(int function, int reason, char *file, int line)
{
    if (lib_code == 0)
        lib_code = ERR_get_next_error_library();
    ERR_PUT_error(lib_code, function, reason, file, line);
}
