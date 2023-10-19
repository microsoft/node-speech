/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "crypto.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>
#include <vector>

const int GCM_TAG_LEN = 16;

bool decrypt(const unsigned char *cipher, const unsigned int cipherSize, const unsigned char *key, const unsigned char *iv, const unsigned char *authTag, std::string &plainText)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        return false;
    }

    if (!EVP_DecryptInit(ctx, EVP_aes_256_gcm(), key, nullptr))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (!EVP_DecryptInit_ex(ctx, nullptr, nullptr, nullptr, iv))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, GCM_TAG_LEN, reinterpret_cast<void *>(const_cast<unsigned char *>(authTag))))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int len;
    std::vector<unsigned char> plain_buf(cipherSize);

    if (!EVP_DecryptUpdate(ctx, plain_buf.data(), &len, cipher, cipherSize))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int plain_len = len;

    if (EVP_DecryptFinal_ex(ctx, plain_buf.data() + len, &len) <= 0)
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    plain_len += len;
    plainText = std::string(reinterpret_cast<char *>(plain_buf.data()), plain_len);

    EVP_CIPHER_CTX_free(ctx);
    return true;
}

bool getKey(const unsigned char *cipher, const unsigned int cipherSize, const unsigned char *iv, const unsigned char *authTag, std::string &key)
{
    std::string license =
        "You may only use the C/C++ Extension for Visual Studio Code and C# "
        "Extension for Visual Studio Code with Visual Studio Code, Visual Studio "
        "or Xamarin Studio software to help you develop and test your applications. "
        "The software is licensed, not sold. This agreement only gives you some "
        "rights to use the software. Microsoft reserves all other rights. You may "
        "not work around any technical limitations in the software; reverse engineer, "
        "decompile or disassemble the software remove, minimize, block or modify any "
        "notices of Microsoft or its suppliers in the software share, publish, rent, "
        "or lease the software, or provide the software as a stand-alone hosted as "
        "solution for others to use.";

    const EVP_MD *md = EVP_sha256();
    unsigned char hash[32]; // SHA-256 produces a 256-bit or 32-byte hash
    unsigned int len;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, md, nullptr);
    EVP_DigestUpdate(ctx, license.data(), license.size());
    EVP_DigestFinal_ex(ctx, hash, &len);
    EVP_MD_CTX_free(ctx);

    if (decrypt(cipher, cipherSize, hash, iv, authTag, key))
    {
        return true;
    }

    return false;
}