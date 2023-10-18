/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

import transcribe, { speechapi, TranscriptionStatusCode } from '../index';
import { join } from 'path';
import { existsSync } from 'fs';

describe('Basics', () => {

	const path = join(__dirname, '..', 'assets', 'stt');

	function transcribeOnce() {
		const model = 'Microsoft Speech Recognizer en-US FP Model V8.1';
		const controller = new AbortController();

		return new Promise<void>(resolve => {
			transcribe({ path, model, signal: controller.signal }, (error, result) => {
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

	(existsSync(path) ? test : test.skip)('it should be able to start and stop', async () => {
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
