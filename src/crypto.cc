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
const int GCM_IV_LEN = 12;

bool encrypt(const std::string &plainText, const std::string &key, std::string &cipherText, std::string &iv, std::string &tag)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        return false;
    }

    if (1 != EVP_EncryptInit(ctx, EVP_aes_256_gcm(), reinterpret_cast<const unsigned char *>(key.data()), nullptr))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    std::vector<unsigned char> iv_buf(GCM_IV_LEN);
    RAND_bytes(iv_buf.data(), GCM_IV_LEN);
    iv = std::string(reinterpret_cast<char *>(iv_buf.data()), GCM_IV_LEN);

    if (1 != EVP_EncryptInit_ex(ctx, nullptr, nullptr, nullptr, iv_buf.data()))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int len;
    std::vector<unsigned char> cipher_buf(plainText.size());

    if (1 != EVP_EncryptUpdate(ctx, cipher_buf.data(), &len, reinterpret_cast<const unsigned char *>(plainText.data()), static_cast<int>(plainText.size())))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    int cipher_len = len;

    if (1 != EVP_EncryptFinal_ex(ctx, cipher_buf.data() + len, &len))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }

    cipher_len += len;
    cipherText = std::string(reinterpret_cast<char *>(cipher_buf.data()), cipher_len);

    std::vector<unsigned char> tag_buf(GCM_TAG_LEN);
    if (1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, GCM_TAG_LEN, tag_buf.data()))
    {
        EVP_CIPHER_CTX_free(ctx);
        return false;
    }
    tag = std::string(reinterpret_cast<char *>(tag_buf.data()), GCM_TAG_LEN);

    EVP_CIPHER_CTX_free(ctx);
    return true;
}

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

bool getKey(std::string &key)
{
    std::string cipher = fromHexString("<AZURE_SPEECH_SDK_KEY>");
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
    std::string iv = fromHexString("5f07a7c61c436c628bd9aaf1");
    std::string tag = fromHexString("0079f977314529d9b86ccb51071d8022");

    if (decrypt(cipher, cipherKey, iv, tag, key))
    {
        return true;
    }

    return false;
}

//
// Instructions how to create a new cipher:
//
// std::string plainText = "<AZURE_SPEECH_SDK_KEY>";
// std::string rawKey =
//     "You may only use the C/C++ Extension for Visual Studio Code and C# "
//     "Extension for Visual Studio Code with Visual Studio Code, Visual Studio "
//     "or Xamarin Studio software to help you develop and test your applications. "
//     "The software is licensed, not sold. This agreement only gives you some "
//     "rights to use the software. Microsoft reserves all other rights. You may "
//     "not work around any technical limitations in the software; reverse engineer, "
//     "decompile or disassemble the software remove, minimize, block or modify any "
//     "notices of Microsoft or its suppliers in the software share, publish, rent, "
//     "or lease the software, or provide the software as a stand-alone hosted as "
//     "solution for others to use.";
// std::string derivedKey = sha256(rawKey);
// std::string cipherText, iv, tag;

// if (encrypt(plainText, derivedKey, cipherText, iv, tag))
// {
//   std::cout << "Encryption successful!" << std::endl;

//   std::string cipherTextHex = toHexString(cipherText);
//   std::cout << "cipherText: " << cipherTextHex << std::endl;

//   std::string ivHex = toHexString(iv);
//   std::cout << "iv: " << ivHex << std::endl;

//   std::string tagHex = toHexString(tag);
//   std::cout << "tag: " << tagHex << std::endl;
// }
// else
// {
//   std::cerr << "Encryption failed!" << std::endl;
// }

// std::string decryptedText;
// if (decrypt(cipherText, derivedKey, iv, tag, decryptedText))
// {
//   std::cout << "Decryption successful: " << decryptedText << std::endl;
// }
// else
// {
//   std::cerr << "Decryption failed!" << std::endl;
// }