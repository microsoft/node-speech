/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

import transcribe, { speechapi, TranscriptionStatusCode } from '../index';
import { dirname, join } from 'path';

describe('Basics', () => {

	function transcribeOnce() {
		const path = join(__dirname, 'assets', 'stt');
		const model = 'Microsoft Speech Recognizer en-US FP Model V8.1';
		const key = process.env['AZURE_SPEECH_KEY'];
		if (!key) {
			throw new Error('Missing Azure Speech API key, please set AZURE_SPEECH_KEY environment variable in the .env file');
		}
		const controller = new AbortController();

		return new Promise<void>(resolve => {
			transcribe({ path, model, key, signal: controller.signal }, (error, result) => {
				switch (result.status) {
					case TranscriptionStatusCode.STARTED:
						setTimeout(() => controller.abort(), 1);
						break;
					case TranscriptionStatusCode.STOPPED:
						resolve();
						break;
				}
			});
		});
	}

	(process.env['AZURE_SPEECH_KEY'] ? test : test.skip)('it should be able to start and stop', async () => {
		await transcribeOnce();
		await transcribeOnce();
	}, 60000);

	test('it should load the native module', () => {
		expect(speechapi).toEqual(expect.objectContaining({
			transcribe: expect.any(Function),
			untranscribe: expect.any(Function)
		}));
	});
});
