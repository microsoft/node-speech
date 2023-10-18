/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>

bool encrypt(const std::string &plainText, const std::string &key,
             std::string &cipherText, std::string &iv, std::string &tag);

bool decrypt(const std::string &cipherText, const std::string &key, const std::string &iv,
             const std::string &tag, std::string &plainText);

std::string sha256(const std::string &input);

std::string fromHexString(const std::string &input);
std::string toHexString(const std::string &input);

bool getKey(std::string &key);

#endif // CRYPTO_H