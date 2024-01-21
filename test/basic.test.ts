/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

import { speechapi } from '../index';

describe('Basics', () => {

	test('it should load the native module', () => {
		expect(speechapi).toEqual(expect.objectContaining({
			createTranscriber: expect.any(Function),
			startTranscriber: expect.any(Function),
			stopTranscriber: expect.any(Function),
			recognize: expect.any(Function),
			unrecognize: expect.any(Function)
		}));
	});
});
