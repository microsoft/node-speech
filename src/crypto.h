/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>

std::string getKey(const unsigned char *cipher, const unsigned int cipherSize, const unsigned char *iv, const unsigned char *authTag);

#endif // CRYPTO_H