/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

export const speechapi = require('bindings')('speechapi.node') as SpeechLib;

interface SpeechLib {

  // Transcription
  transcribe: (modelPath: string, modelName: string, modelKey: string, wavPath: string | undefined, callback: (error: Error | undefined, result: ITranscriptionResult) => void) => number,
  untranscribe: (id: number) => void,

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

  /**
   * Path to the wav file to transcribe. If not specified, the audio 
   * will be streamed from the microphone.
   */
  readonly wavPath: string | undefined;

  readonly signal: AbortSignal;
}

export function transcribe({ modelPath, modelName, modelKey, signal, wavPath }: ITranscriptionOptions, callback: ITranscriptionCallback): void {
  const id = speechapi.transcribe(modelPath, modelName, modelKey, wavPath, callback);

  const onAbort = () => {
    speechapi.untranscribe(id);
    signal.removeEventListener('abort', onAbort);
  };

  signal.addEventListener('abort', onAbort);
}

export default transcribe;

//#endregion


//#region Keyword Recognition

export enum KeywordRecognitionStatusCode {
  RECOGNIZED = 3,
  STOPPED = 9,
  ERROR = 10
}

export interface IKeywordRecognitionResult {
  readonly status: KeywordRecognitionStatusCode;
  readonly data?: string;
}

export interface IKeywordRecognitionCallback {
  (error: Error | undefined, result: IKeywordRecognitionResult): void;
}

export interface IKeywordRecognitionOptions {
  readonly modelPath: string;

  readonly signal: AbortSignal;
}

export function recognize({ modelPath, signal }: IKeywordRecognitionOptions, callback: IKeywordRecognitionCallback): void {
  const id = speechapi.recognize(modelPath, callback);

  const onAbort = () => {
    speechapi.unrecognize(id);
    signal.removeEventListener('abort', onAbort);
  };

  signal.addEventListener('abort', onAbort);
}

//#endregion

