/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

import transcribe, { TranscriptionStatusCode } from '../index';

describe('Basics', () => {

    function transcribeOnce() {
        const controller = new AbortController();

        return new Promise<void>(resolve => {
            transcribe((error, result) => {
                switch (result.status) {
                    case TranscriptionStatusCode.STARTED:
                        setTimeout(() => controller.abort(), 1);
                        break;
                    case TranscriptionStatusCode.STOPPED:
                        resolve();
                        break;
                }
            }, { signal: controller.signal });
        });
    }

    test('it should be able to start and stop', async () => {
        await transcribeOnce();
        await transcribeOnce();
    }, 60000);
});
