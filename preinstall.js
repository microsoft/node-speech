/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

//@ts-check
'use strict';

const fs = require('fs');
const path = require('path');
const { finished } = require('stream/promises');
const yauzl = require('yauzl');

const SDK_VERSION = '1.41.1';

const packages = {
  'microsoft.cognitiveservices.speech': SDK_VERSION,
  'microsoft.cognitiveservices.speech.extension.embedded.sr': SDK_VERSION,
  'microsoft.cognitiveservices.speech.extension.embedded.tts': SDK_VERSION,
  'microsoft.cognitiveservices.speech.extension.onnx.runtime': SDK_VERSION,
  // 'microsoft.cognitiveservices.speech.extension.telemetry': SDK_VERSION,
};

async function decompress(packagePath, outputPath) {
  return new Promise((resolve, reject) => {
    yauzl.open(packagePath, { lazyEntries: true }, (err, zipfile) => {
      if (err) {
        return reject(err);
      }

      zipfile.on('entry', async entry => {
        if (!/\/$/.test(entry.fileName)) {
          await new Promise((resolve, reject) => {
            zipfile.openReadStream(entry, async (err, istream) => {
              if (err) {
                return reject(err);
              }

              try {
                const filePath = path.join(outputPath, entry.fileName);
                await fs.promises.mkdir(path.dirname(filePath), { recursive: true });
                const ostream = fs.createWriteStream(filePath);
                await finished(istream.pipe(ostream));
                resolve(undefined);
              } catch (err) {
                reject(err);
              }
            });
          });
        }

        zipfile.readEntry();
      });

      zipfile.on('end', resolve);
      zipfile.readEntry();
    });
  });
}

async function main(packages) {
  const cacheDir = path.join(__dirname, '.cache');
  const sdkPath = path.join(cacheDir, 'SpeechSDK');

  for (const [name, version] of Object.entries(packages)) {
    const packagePath = path.join(cacheDir, 'packages', `${name}.${version}.nupkg`);
    await decompress(packagePath, sdkPath);
  }
}

main(packages).catch(console.error);
