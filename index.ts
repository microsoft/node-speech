/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

import { createDecipheriv, createHash } from 'crypto';

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
  readonly modelKey: string;

  readonly authTag: Buffer;
  readonly iv: Buffer;
  readonly cipher: Buffer;

  readonly signal: AbortSignal;
}

interface SpeechLib {
  transcribe: (modelPath: string, modelName: string, modelKey: string, authTagHex: Buffer, ivHex: Buffer, cipherHex: Buffer, callback: (error: Error | undefined, result: ITranscriptionResult) => void, fallbackModelKey: string) => number,
  untranscribe: (id: number) => void
}

function getKey(modelKey: string, authTag: Buffer, iv: Buffer, cipher: Buffer): string {
  const sha256hash = createHash('sha256');
  sha256hash.update(modelKey);
  const key = sha256hash.digest();

  const decipher = createDecipheriv('aes-256-gcm', key, iv);
  decipher.setAuthTag(authTag);

  return Buffer.concat([decipher.update(cipher), decipher.final()]).toString();
}

export function transcribe({ modelPath, modelName, modelKey, authTag, iv, cipher, signal }: ITranscriptionOptions, callback: ITranscriptionCallback): void {
  const id = speechapi.transcribe(modelPath, modelName, modelKey, authTag, iv, cipher, callback, getKey(modelKey, authTag, iv, cipher));

  const onAbort = () => {
    speechapi.untranscribe(id);
    signal.removeEventListener('abort', onAbort);
  };

  signal.addEventListener('abort', onAbort);
}

export default transcribe;