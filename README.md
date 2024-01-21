[![Build Status](https://dev.azure.com/monacotools/Monaco/_apis/build/status%2Fnpm%2Fvscode%2Fmicrosoft.node-speech?repoName=microsoft%2Fnode-speech&branchName=main)](https://dev.azure.com/monacotools/Monaco/_build/latest?definitionId=529&repoName=microsoft%2Fnode-speech&branchName=main)

# node-speech

A node.js binding for a subset of the [Azure Speech SDK](https://learn.microsoft.com/en-us/azure/ai-services/speech-service/speech-sdk). Specifically for the [Embedded Speech](https://learn.microsoft.com/en-us/azure/ai-services/speech-service/embedded-speech).

## Installation

Install with npm:

```sh
npm ci
```

## Usage: Transcription

```ts
import * as speech from "@vscode/node-speech";

const modelName = "<name of the speech model>";
const modelPath = "<path to the speech model>";
const modelKey = "<key for the speech model>";

// Live transcription from microphone
let transcriber = speech.createTranscriber(
  { modelName, modelPath, modelKey, signal, wavPath: undefined },
  (err, res) => console.log(err, res)
);
transcriber.start();
// later when done...
transcriber.stop();

// Transcription from *.wav file
transcriber = speech.createTranscriber(
  { modelName, modelPath, modelKey, signal, wavPath: "path-to-wav-file" },
  (err, res) => console.log(err, res)
);
transcriber.start();
// later when done...
transcriber.stop();
```

## Usage: Keyword Recognition

```ts
import * as speech from "@vscode/node-speech";

const modelPath = "<path to the keyword model>";

// Keyword recognition from microphone
const result = await speech.recgonize({ modelPath, signal });
console.log(result);
```

## Code of Conduct

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## License

Copyright (c) Microsoft Corporation. All rights reserved.

Licensed under the [MIT](LICENSE.txt) license.
