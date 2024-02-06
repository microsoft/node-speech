/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

export const speechapi = require('bindings')('speechapi.node') as SpeechLib;

interface SpeechLib {

  // Transcription
  createTranscriber: (modelPath: string, modelName: string, modelKey: string, logsPath: string | undefined, callback: (error: Error | undefined, result: ITranscriptionResult) => void) => number,
  startTranscriber: (id: number) => void,
  stopTranscriber: (id: number) => void,
  disposeTranscriber: (id: number) => void,

  // Keyword Recognition
  recognize: (modelPath: string, callback: (error: Error | undefined, result: IKeywordRecognitionResult) => void) => number,
  unrecognize: (id: number) => void
}

//#region Transcription

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
  DISPOSED = 10,
  ERROR = 11
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

  /**
   * Path to a file to store verbose logs from the Azure Speech SDK to.
   */
  readonly logsPath?: string;
}

export interface ITranscriber {
  start(): void;
  stop(): void;
  dispose(): void;
}

export function createTranscriber({ modelPath, modelName, modelKey, logsPath }: ITranscriptionOptions, callback: ITranscriptionCallback): ITranscriber {
  const id = speechapi.createTranscriber(modelPath, modelName, modelKey, logsPath ?? undefined, callback);

  return {
    start: () => speechapi.startTranscriber(id),
    stop: () => speechapi.stopTranscriber(id),
    dispose: () => speechapi.disposeTranscriber(id)
  };
}

//#endregion

//#region Keyword Recognition

export enum KeywordRecognitionStatusCode {
  RECOGNIZED = 3,
  STOPPED = 9,
  ERROR = 11
}

export interface IKeywordRecognitionResult {
  readonly status: KeywordRecognitionStatusCode;
  readonly data?: string;
}

export interface IKeywordRecognitionOptions {
  readonly modelPath: string;

  readonly signal: AbortSignal;
}

export function recognize({ modelPath, signal }: IKeywordRecognitionOptions): Promise<IKeywordRecognitionResult> {
  return new Promise<IKeywordRecognitionResult>((resolve, reject) => {
    const id = speechapi.recognize(modelPath, (error, result) => {
      if (error) {
        reject(error);
      } else {
        resolve(result);
      }
    });

    const onAbort = () => {
      speechapi.unrecognize(id);
      signal.removeEventListener('abort', onAbort);
    };

    signal.addEventListener('abort', onAbort);
  });
}

//#endregion
