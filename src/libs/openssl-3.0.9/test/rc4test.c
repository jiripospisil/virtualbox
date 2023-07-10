/*
 * Copyright 1995-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/*
 * RC4 and SHA-1 low level APIs are deprecated for public use, but still ok for
 * internal use.
 */
#include "internal/deprecated.h"

#include <string.h>

#include "internal/nelem.h"
#include "testutil.h"

#ifndef OPENSSL_NO_RC4
# include <openssl/rc4.h>
# include <openssl/sha.h>

static unsigned char keys[6][30] = {
    {8, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef},
    {8, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef},
    {8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {4, 0xef, 0x01, 0x23, 0x45},
    {8, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef},
    {4, 0xef, 0x01, 0x23, 0x45},
};

static unsigned char data_len[6] = { 8, 8, 8, 20, 28, 10 };

static unsigned char data[6][30] = {
    {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xff},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
     0x00, 0x00, 0x00, 0x00, 0xff},
    {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
     0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
     0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
     0x12, 0x34, 0x56, 0x78, 0xff},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff},
};

static unsigned char output[6][30] = {
    {0x75, 0xb7, 0x87, 0x80, 0x99, 0xe0, 0xc5, 0x96, 0x00},
    {0x74, 0x94, 0xc2, 0xe7, 0x10, 0x4b, 0x08, 0x79, 0x00},
    {0xde, 0x18, 0x89, 0x41, 0xa3, 0x37, 0x5d, 0x3a, 0x00},
    {0xd6, 0xa1, 0x41, 0xa7, 0xec, 0x3c, 0x38, 0xdf,
     0xbd, 0x61, 0x5a, 0x11, 0x62, 0xe1, 0xc7, 0xba,
     0x36, 0xb6, 0x78, 0x58, 0x00},
    {0x66, 0xa0, 0x94, 0x9f, 0x8a, 0xf7, 0xd6, 0x89,
     0x1f, 0x7f, 0x83, 0x2b, 0xa8, 0x33, 0xc0, 0x0c,
     0x89, 0x2e, 0xbe, 0x30, 0x14, 0x3c, 0xe2, 0x87,
     0x40, 0x01, 0x1e, 0xcf, 0x00},
    {0xd6, 0xa1, 0x41, 0xa7, 0xec, 0x3c, 0x38, 0xdf, 0xbd, 0x61, 0x00},
};

static int test_rc4_encrypt(const int i)
{
    unsigned char obuf[512];
    RC4_KEY key;

    RC4_set_key(&key, keys[i][0], &(keys[i][1]));
    memset(obuf, 0, sizeof(obuf));
    RC4(&key, data_len[i], &(data[i][0]), obuf);
    return TEST_mem_eq(obuf, data_len[i] + 1, output[i], data_len[i] + 1);
}

static int test_rc4_end_processing(const int i)
{
    unsigned char obuf[512];
    RC4_KEY key;

    RC4_set_key(&key, keys[3][0], &(keys[3][1]));
    memset(obuf, 0, sizeof(obuf));
    RC4(&key, i, &(data[3][0]), obuf);
    if (!TEST_mem_eq(obuf, i, output[3], i))
        return 0;
    return TEST_uchar_eq(obuf[i], 0);
}

static int test_rc4_multi_call(const int i)
{
    unsigned char obuf[512];
    RC4_KEY key;

    RC4_set_key(&key, keys[3][0], &(keys[3][1]));
    memset(obuf, 0, sizeof(obuf));
    RC4(&key, i, &(data[3][0]), obuf);
    RC4(&key, data_len[3] - i, &(data[3][i]), &(obuf[i]));
    return TEST_mem_eq(obuf, data_len[3] + 1, output[3], data_len[3] + 1);
}

static int test_rc_bulk(void)
{
    RC4_KEY key;
    unsigned char buf[513];
    SHA_CTX c;
    unsigned char md[SHA_DIGEST_LENGTH];
    int i;
    static unsigned char expected[] = {
        0xa4, 0x7b, 0xcc, 0x00, 0x3d, 0xd0, 0xbd, 0xe1, 0xac, 0x5f,
        0x12, 0x1e, 0x45, 0xbc, 0xfb, 0x1a, 0xa1, 0xf2, 0x7f, 0xc5
    };

    RC4_set_key(&key, keys[0][0], &(keys[3][1]));
    memset(buf, 0, sizeof(buf));
    SHA1_Init(&c);
    for (i = 0; i < 2571; i++) {
        RC4(&key, sizeof(buf), buf, buf);
        SHA1_Update(&c, buf, sizeof(buf));
    }
    SHA1_Final(md, &c);

    return TEST_mem_eq(md, sizeof(md), expected, sizeof(expected));
}
#endif

int setup_tests(void)
{
#ifndef OPENSSL_NO_RC4
    ADD_ALL_TESTS(test_rc4_encrypt, OSSL_NELEM(data_len));
    ADD_ALL_TESTS(test_rc4_end_processing, data_len[3]);
    ADD_ALL_TESTS(test_rc4_multi_call, data_len[3]);
    ADD_TEST(test_rc_bulk);
#endif
    return 1;
}
