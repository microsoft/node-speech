/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

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
  readonly modelPath: string;
  readonly modelName: string;

  readonly authTag: Buffer;
  readonly iv: Buffer;
  readonly cipher: Buffer;

  readonly signal: AbortSignal;
}

interface SpeechLib {
  transcribe: (modelPath: string, modelName: string, authTagHex: Buffer, ivHex: Buffer, cipherHex: Buffer, callback: (error: Error | undefined, result: ITranscriptionResult) => void) => number,
  untranscribe: (id: number) => void
}

export function transcribe({ modelPath, modelName, authTag, iv, cipher, signal }: ITranscriptionOptions, callback: ITranscriptionCallback): void {
  const id = speechapi.transcribe(modelPath, modelName, authTag, iv, cipher, callback);

  const onAbort = () => {
    speechapi.untranscribe(id);
    signal.removeEventListener('abort', onAbort);
  };

  signal.addEventListener('abort', onAbort);
}

if (require.main === module) {
  const modelPath = `/Users/bpasero/Development/Microsoft/vscode-speech/assets/stt/speechmodel.en-US.cpu.8.1.38136433`;
  const modelName = 'Microsoft Speech Recognizer en-US FP Model V8.1';

  const authTag = Buffer.from('bb87382929d0fe55fe0516e20b04c64b', 'hex');
  const iv = Buffer.from('e44467524bbaf66e99d2a837', 'hex');
  const cipher = Buffer.from('8c55b7ded1c8281b839abb62df91479d06e4a4fb69832e927be2830be1527a6af31f235f11c0fe61a7d6935668492a7c9ce458911717107809dc17022d198afdb6bfed260ab249dbddefdae3f2da657c', 'hex');

  transcribe({
    modelPath,
    modelName,
    authTag,
    cipher,
    iv,
    signal: new AbortController().signal
  }, (error, result) => {
    console.log(error, result);
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