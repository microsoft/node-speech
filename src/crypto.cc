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

bool decrypt(const std::string &cipherText, const std::string &key, const std::string &iv, const std::string &tag, std::string &plainText)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        return false;
    }

    if (!EVP_DecryptInit(ctx, EVP_aes_256_gcm(), reinterpret_cast<const unsigned char *>(key.data()), nullptr))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (!EVP_DecryptInit_ex(ctx, nullptr, nullptr, nullptr, reinterpret_cast<const unsigned char *>(iv.data())))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, GCM_TAG_LEN, reinterpret_cast<void *>(const_cast<char *>(tag.data()))))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int len;
    std::vector<unsigned char> plain_buf(cipherText.size());

    if (!EVP_DecryptUpdate(ctx, plain_buf.data(), &len, reinterpret_cast<const unsigned char *>(cipherText.data()), static_cast<int>(cipherText.size())))
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

std::string sha256(const std::string &input)
{
    const EVP_MD *md = EVP_sha256();
    unsigned char hash[256 / 8]; // SHA-256 produces a 256-bit or 32-byte hash
    unsigned int len;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, md, nullptr);
    EVP_DigestUpdate(ctx, input.data(), input.size());
    EVP_DigestFinal_ex(ctx, hash, &len);
    EVP_MD_CTX_free(ctx);

    return std::string(reinterpret_cast<char *>(hash), len);
}

std::string toHexString(const std::string &input)
{
    std::ostringstream oss;

    for (unsigned char c : input)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }

    return oss.str();
}

std::string fromHexString(const std::string &input)
{
    if (input.length() % 2 != 0)
    {
        return {}; // or handle error
    }

    std::string output;
    output.reserve(input.length() / 2);

    for (size_t i = 0; i < input.length(); i += 2)
    {
        std::string byteString = input.substr(i, 2);
        char byteChar = static_cast<char>(std::strtol(byteString.c_str(), nullptr, 16));
        output.push_back(byteChar);
    }

    return output;
}

bool getKey(const std::string &cipherHex, const std::string &ivHex, const std::string &authTagHex, std::string &key)
{
    std::string cipher = fromHexString(cipherHex);
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
    std::string cipherKey = sha256(license);
    std::string iv = fromHexString(ivHex);
    std::string authTag = fromHexString(authTagHex);

    if (decrypt(cipher, cipherKey, iv, authTag, key))
    {
        return true;
    }

    return false;
}