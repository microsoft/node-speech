/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

import * as fs from 'fs';
import { join } from 'path';
import got from 'got';
import decompress from 'decompress';

const packages = {
  'Microsoft.CognitiveServices.Speech': '1.32.1',
  'Microsoft.CognitiveServices.Speech.Extension.Embedded.SR': '1.32.1',
  // 'Microsoft.CognitiveServices.Speech.Extension.Embedded.TTS': '1.32.1',
  'Microsoft.CognitiveServices.Speech.Extension.ONNX.Runtime': '1.32.1',
  // 'Microsoft.CognitiveServices.Speech.Extension.Telemetry': '1.32.1',
};

async function main(packages: Record<string, string>): Promise<void> {
  const cacheDir = join(__dirname, '.cache');
  await fs.promises.mkdir(join(cacheDir, 'packages'), { recursive: true });
  await fs.promises.mkdir(join(cacheDir, 'SpeechSDK'), { recursive: true });

  for (const [name, version] of Object.entries(packages)) {
    const url = `https://www.nuget.org/api/v2/package/${name}/${version}`;
    const packagePath = join(cacheDir, 'packages', `${name}-${version}.nupkg`);

    if (fs.existsSync(packagePath)) {
      console.log(`Found ${name} ${version} in cache, skipping.`);
      continue;
    }

    const sdkPath = join(cacheDir, 'SpeechSDK');

    const downloadStream = got.stream(url);
    const fileWriterStream = fs.createWriteStream(packagePath);

    downloadStream.pipe(fileWriterStream);

    console.log(`Downloading ${name} ${version}...`);
    await new Promise((resolve, reject) => {
      fileWriterStream.on('finish', resolve);
      fileWriterStream.on('error', reject);
    });

    await decompress(packagePath, sdkPath);
  }
}

main(packages).catch(console.error);
