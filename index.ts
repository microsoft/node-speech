/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

import * as dotenv from 'dotenv';
import { join } from 'path';

dotenv.config();

export enum TranscriptionStatusCode {
  STARTED = 1,
  RECOGNIZING = 2,
  RECOGNIZED = 3,
  NOT_RECOGNIZED = 4,
  INITIAL_SILENCE_TIMEOUT = 5,
  END_SILENCE_TIMEOUT = 6,
  SPEECH_START_DETECTED = 7,
  SPEECH_END_DETECTED = 8,
  STOPPED = 9,
  ERROR = 10
}

export interface ITranscriptionResult {
  readonly status: TranscriptionStatusCode;
  readonly data?: string;
}

export interface ITranscriptionCallback {
  (error: Error | undefined, result: ITranscriptionResult): void;
}

export interface ITranscriptionOptions {
  readonly signal: AbortSignal;
}

interface SpeechLib {
  transcribe: (path: string, key: string, model: string, callback: (error: Error | undefined, result: ITranscriptionResult) => void) => number,
  untranscribe: (id: number) => void
}

const speechapi = require(join(__dirname, 'build', 'Release', 'speechapi.node')) as SpeechLib;

export function transcribe(callback: ITranscriptionCallback, options: ITranscriptionOptions): void {
  const path = join(__dirname, 'dependencies', 'sr-models');
  const model = 'Microsoft Speech Recognizer en-US FP Model V8.1';
  const key = process.env['AZURE_SPEECH_KEY'];

  if (!key) {
    throw new Error('Missing Azure Speech API key, please set AZURE_SPEECH_KEY environment variable in the .env file');
  }

  const id = speechapi.transcribe(path, key, model, callback);

  const onAbort = () => {
    speechapi.untranscribe(id);
    options.signal.removeEventListener('abort', onAbort);
  };

  options.signal.addEventListener('abort', onAbort);
}

if (require.main === module) {
  transcribe((error, result) => {
    if (result.status === TranscriptionStatusCode.RECOGNIZING) {
      process.stdout.write(`\r${result.data}`);
    } else if (result.status === TranscriptionStatusCode.RECOGNIZED) {
      console.log(`\r${result.data}`);
    }
  }, {
    signal: new AbortController().signal
  });
}

export default transcribe;