/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

//@ts-check
'use strict';

const fs = require('fs');
const path = require('path');
const stream = require('stream');
const { finished } = require('stream/promises');
const yauzl = require('yauzl');

const SDK_VERSION = '1.33.0';

const packages = {
  'Microsoft.CognitiveServices.Speech': SDK_VERSION,
  'Microsoft.CognitiveServices.Speech.Extension.Embedded.SR': SDK_VERSION,
  // 'Microsoft.CognitiveServices.Speech.Extension.Embedded.TTS': SDK_VERSION,
  'Microsoft.CognitiveServices.Speech.Extension.ONNX.Runtime': SDK_VERSION,
  // 'Microsoft.CognitiveServices.Speech.Extension.Telemetry': SDK_VERSION,
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
                await fs.promises.mkdir(path.dirname(filePath), { recursive: true, force: true });
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
  await fs.promises.mkdir(path.join(cacheDir, 'packages'), { recursive: true });
  await fs.promises.mkdir(path.join(cacheDir, 'SpeechSDK'), { recursive: true });

  for (const [name, version] of Object.entries(packages)) {
    const url = `https://www.nuget.org/api/v2/package/${name}/${version}`;
    const packagePath = path.join(cacheDir, 'packages', `${name}-${version}.nupkg`);

    if (fs.existsSync(packagePath)) {
      console.log(`Found ${name} ${version} in cache, skipping.`);
      continue;
    }

    const sdkPath = path.join(cacheDir, 'SpeechSDK');

    const res = await fetch(url);

    if (!res.ok) {
      throw new Error(`Failed to download ${name} ${version}: ${res.statusText}`);
    }

    console.log(`Downloading ${name} ${version}...`);
    const istream = stream.Readable.fromWeb(res.body);
    const ostream = fs.createWriteStream(packagePath);
    await finished(istream.pipe(ostream));

    await decompress(packagePath, sdkPath);
  }
}

main(packages).catch(console.error);
