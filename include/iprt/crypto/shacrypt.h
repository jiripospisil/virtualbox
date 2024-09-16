/** @file
 * IPRT - Crypto - SHA-crypt.
 */

/*
 * Copyright (C) 2023-2024 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#ifndef IPRT_INCLUDED_crypto_shacrypt_h
#define IPRT_INCLUDED_crypto_shacrypt_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/sha.h>


RT_C_DECLS_BEGIN

/** @defgroup grp_rt_crshacrypt   RTCrShaCrypt - SHA-crypt
 * @ingroup grp_rt_crypto
 *
 * This implements SHA-crypt.txt v0.6 (2016-08-31), which is a scheme to encrypt
 * passwords using SHA-256 and SHA-512.
 *
 * @{
 */

/** Minimum salt string length for SHA-crypt (inclusive). */
#define RT_SHACRYPT_SALT_MIN_LEN        8
/** Maximum salt string length for SHA-crypt (inclusive). */
#define RT_SHACRYPT_SALT_MAX_LEN        16


/** Minimum number of rounds for SHA-crypt (inclusive). */
#define RT_SHACRYPT_ROUNDS_MIN          1000
/** Default number of rounds for SHA-crypt. */
#define RT_SHACRYPT_ROUNDS_DEFAULT      5000
/** Maximum number of rounds for SHA-crypt (inclusive). */
#define RT_SHACRYPT_ROUNDS_MAX          999999999


/** The maximum string length of a password encrypted with SHA-256
 * (including the terminator). */
#define RT_SHACRYPT_256_MAX_SIZE        80
/** The maximum string length of a password encrypted with SHA-512.
 * (including the terminator). */
#define RT_SHACRYPT_512_MAX_SIZE        123


/** The extended password '$ID$' part for SHA-256 encrypted passwords. */
#define RT_SHACRYPT_ID_STR_256          "$5$"
/** The extended password '$ID$' part for SHA-512 encrypted passwords. */
#define RT_SHACRYPT_ID_STR_512          "$6$"


/**
 * Creates a randomized salt for the RTCrShaCryptXXX functions.
 *
 * @returns IPRT status code.
 * @param   pszSalt     Where to return the generated salt string.
 * @param   cchSalt     The length of the salt to generate.  The buffer size is
 *                      this value plus 1 (for the terminator character)!
 *
 *                      Must be in the range RT_SHACRYPT_MIN_SALT_LEN to
 *                      RT_SHACRYPT_MAX_SALT_LEN (both inclusive).
 *
 * @warning Be very careful with the @a cchSalt parameter, as it must be at
 *          least one less than the actual buffer size!
 */
RTDECL(int) RTCrShaCryptGenerateSalt(char *pszSalt, size_t cchSalt);

/**
 * Encrypts @a pszPhrase using SHA-256, @a pszSalt and @a cRounds.
 *
 * @returns IPRT status code.
 * @param   pszPhrase   The passphrase (password) to encrypt.
 * @param   pszSalt     Salt to use.  This can either be a pure salt, like from
 *                      RTCrShaCryptGenerateSalt(), or a 'password' string as
 *                      generated by RTCrShaCrypt256ToString().
 *
 *                      The latter allows for easy password validation by
 *                      comparing the encrypted string with the one stored in
 *                      the passwd file.
 *
 *                      The length of the actual salt portion of the string must
 *                      be within RT_SHACRYPT_MIN_SALT_LEN to
 *                      RT_SHACRYPT_MAX_SALT_LEN, both inclusive.
 *
 * @param   cRounds     Number of rounds to use, must be in the range
 *                      RT_SHACRYPT_ROUNDS_MIN thru RT_SHACRYPT_ROUNDS_MAX.
 *                      If unsure, use RT_SHACRYPT_ROUNDS_DEFAULT.  This is
 *                      ignored if the salt includes a 'rounds=xxx$' part.
 * @param   pszString   Where to store the string on success.
 * @param   cbString    The size of the buffer pointed to by @a pszString.
 *
 *                      The minimum length is 3 + salt + 1 + 43 + zero
 *                      terminator.  If a non-default @a cRounds value is used,
 *                      add 8 + strlen(cRounds as decimal). The max buffer
 *                      needed is 80 bytes (RT_SHACRYPT_256_MAX_SIZE).
 */
RTDECL(int) RTCrShaCrypt256(const char *pszPhrase, const char *pszSalt, uint32_t cRounds, char *pszString, size_t cbString);

/**
 * Encrypts @a pszPhrase using SHA-512, @a pszSalt and @a cRounds.
 *
 * @returns IPRT status code.
 * @param   pszPhrase   The passphrase (password) to encrypt.
 * @param   pszSalt     Salt to use.  This can either be a pure salt, like from
 *                      RTCrShaCryptGenerateSalt(), or a 'password' string as
 *                      generated by RTCrShaCrypt512ToString().
 *
 *                      The latter allows for easy password validation by
 *                      comparing the encrypted string with the one stored in
 *                      the passwd file.
 *
 *                      The length of the actual salt portion of the string must
 *                      be within RT_SHACRYPT_MIN_SALT_LEN to
 *                      RT_SHACRYPT_MAX_SALT_LEN, both inclusive.
 *
 * @param   cRounds     Number of rounds to use, must be in the range
 *                      RT_SHACRYPT_ROUNDS_MIN thru RT_SHACRYPT_ROUNDS_MAX.
 *                      If unsure, use RT_SHACRYPT_ROUNDS_DEFAULT.  This is
 *                      ignored if the salt includes a 'rounds=xxx$' part.
 * @param   pszString   Where to store the string on success.
 * @param   cbString    The size of the buffer pointed to by @a pszString.
 *
 *                      The minimum length is 3 + salt + 1 + 86 + zero
 *                      terminator. If a non-default @a cRounds value is used,
 *                      add 8 + strlen(cRounds as decimal). The max buffer
 *                      needed is 123 bytes (RT_SHACRYPT_512_MAX_SIZE).
 */
RTDECL(int) RTCrShaCrypt512(const char *pszPhrase, const char *pszSalt, uint32_t cRounds, char *pszString, size_t cbString);



/**
 * Encrypts @a pszPhrase using SHA-256, @a pszSalt and @a cRounds, returning raw
 * bytes.
 *
 * @returns IPRT status code.
 * @param   pszPhrase   The passphrase (password) to encrypt.
 * @param   pszSalt     Salt to use.  This can either be a pure salt, like from
 *                      RTCrShaCryptGenerateSalt(), or a 'password' string as
 *                      generated by RTCrShaCrypt256ToString().
 *
 *                      The latter allows for easy password validation by
 *                      comparing the encrypted string with the one stored in
 *                      the passwd file.
 *
 *                      The length of the actual salt portion of the string must
 *                      be within RT_SHACRYPT_MIN_SALT_LEN to
 *                      RT_SHACRYPT_MAX_SALT_LEN, both inclusive.
 *
 * @param   cRounds     Number of rounds to use, must be in the range
 *                      RT_SHACRYPT_ROUNDS_MIN thru RT_SHACRYPT_ROUNDS_MAX.
 *                      If unsure, use RT_SHACRYPT_ROUNDS_DEFAULT.  This is
 *                      ignored if the salt includes a 'rounds=xxx$' part.
 * @param   pabHash     Where to return the hash on success.
 * @see     RTCrShaCrypt256, RTCrShaCrypt256ToString
 */
RTDECL(int) RTCrShaCrypt256Ex(const char *pszPhrase, const char *pszSalt, uint32_t cRounds, uint8_t pabHash[RTSHA256_HASH_SIZE]);


/**
 * Encrypts @a pszPhrase using SHA-512, @a pszSalt and @a cRounds, returning raw
 * bytes.
 *
 * @returns IPRT status code.
 * @param   pszPhrase   The passphrase (password) to encrypt.
 * @param   pszSalt     Salt to use.  This can either be a pure salt, like from
 *                      RTCrShaCryptGenerateSalt(), or a 'password' string as
 *                      generated by RTCrShaCrypt512ToString().
 *
 *                      The latter allows for easy password validation by
 *                      comparing the encrypted string with the one stored in
 *                      the passwd file.
 *
 *                      The length of the actual salt portion of the string must
 *                      be within RT_SHACRYPT_MIN_SALT_LEN to
 *                      RT_SHACRYPT_MAX_SALT_LEN, both inclusive.
 *
 * @param   cRounds     Number of rounds to use, must be in the range
 *                      RT_SHACRYPT_ROUNDS_MIN thru RT_SHACRYPT_ROUNDS_MAX.
 *                      If unsure, use RT_SHACRYPT_ROUNDS_DEFAULT.  This is
 *                      ignored if the salt includes a 'rounds=xxx$' part.
 * @param   pabHash     Where to return the hash on success.
 * @see     RTCrShaCrypt512, RTCrShaCrypt512ToString
 */
RTDECL(int) RTCrShaCrypt512Ex(const char *pszPhrase, const char *pszSalt, uint32_t cRounds, uint8_t pabHash[RTSHA512_HASH_SIZE]);

/**
 * Formats the RTCrShaCrypt256Ex() result and non-secret inputs using the
 * extended password format.
 *
 * @returns IPRT status code.
 * @param   pabHash     The result from RTCrShaCrypt256().
 * @param   pszSalt     The salt used when producing @a pabHash.
 * @param   cRounds     Number of rounds used for producing @a pabHash.
 * @param   pszString   Where to store the string on success.
 * @param   cbString    The size of the buffer pointed to by @a pszString.
 *
 *                      The minimum length is 3 + salt + 1 + 43 + zero
 *                      terminator.  If a non-default @a cRounds value is used,
 *                      add 8 + strlen(cRounds as decimal). The max buffer
 *                      needed is 80 bytes (RT_SHACRYPT_256_MAX_SIZE).
 *
 * @note    This implements step 22 of SHA-crypt.txt v0.6.
 * @see     RTCrShaCrypt256Ex
 */
RTDECL(int) RTCrShaCrypt256ToString(uint8_t const pabHash[RTSHA256_HASH_SIZE], const char *pszSalt, uint32_t cRounds,
                                    char *pszString, size_t cbString);

/**
 * Formats the RTCrShaCrypt512Ex() result and non-secret inputs using the
 * extended password format.
 *
 * @returns IPRT status code.
 * @param   pabHash     The result from RTCrShaCrypt512().
 * @param   pszSalt     The salt used when producing @a pabHash.
 * @param   cRounds     Number of rounds used for producing @a pabHash.
 * @param   pszString   Where to store the string on success.
 * @param   cbString    The size of the buffer pointed to by @a pszString.
 *
 *                      The minimum length is 3 + salt + 1 + 86 + zero
 *                      terminator. If a non-default @a cRounds value is used,
 *                      add 8 + strlen(cRounds as decimal). The max buffer
 *                      needed is 123 bytes (RT_SHACRYPT_512_MAX_SIZE).
 *
 * @note    This implements step 22 of SHA-crypt.txt v0.6.
 * @see     RTCrShaCrypt512Ex
 */
RTDECL(int) RTCrShaCrypt512ToString(uint8_t const pabHash[RTSHA512_HASH_SIZE], const char *pszSalt, uint32_t cRounds,
                                    char *pszString, size_t cbString);

/** @} */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_crypto_shacrypt_h */

