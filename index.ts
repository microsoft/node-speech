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

export default transcribe;