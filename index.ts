/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

import { join } from 'path';
export const speechapi = require('bindings')('speechapi.node') as SpeechLib;

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
  readonly path: string;
  readonly key: string;
  readonly model: string;
  readonly signal: AbortSignal;
}

interface SpeechLib {
  transcribe: (path: string, key: string, model: string, callback: (error: Error | undefined, result: ITranscriptionResult) => void) => number,
  untranscribe: (id: number) => void
}

export function transcribe({ key, model, path, signal }: ITranscriptionOptions, callback: ITranscriptionCallback): void {
  const id = speechapi.transcribe(path, key, model, callback);

  const onAbort = () => {
    speechapi.untranscribe(id);
    signal.removeEventListener('abort', onAbort);
  };

  signal.addEventListener('abort', onAbort);
}

if (require.main === module) {
  const path = join(__dirname, 'dependencies', 'sr-models');
  const model = 'Microsoft Speech Recognizer en-US FP Model V8.1';
  const key = process.env['AZURE_SPEECH_KEY'];
  if (!key) {
    throw new Error('Missing Azure Speech API key, please set AZURE_SPEECH_KEY environment variable');
  }

  transcribe({
    path,
    key,
    model,
    signal: new AbortController().signal
  }, (error, result) => {
    if (error) {
      console.error(error);
    } else {
      switch (result.status) {
        case TranscriptionStatusCode.RECOGNIZING:
          process.stdout.write(`\r${result.data}`);
          break;
        case TranscriptionStatusCode.RECOGNIZED:
          console.log(`\r${result.data}`);
          break;
        default:
          if (result.data) {
            console.error(result.status, result.data);
          }
      }
    }
  });
}

export default transcribe;