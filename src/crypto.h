/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>

bool decrypt(const std::string &cipherText, const std::string &key, const std::string &iv,
             const std::string &tag, std::string &plainText);

std::string fromHexString(const std::string &input);
std::string toHexString(const std::string &input);

bool getKey(const std::string &cipher, const std::string &iv, const std::string &authTag, std::string &key);

#endif // CRYPTO_H