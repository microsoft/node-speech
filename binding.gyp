{
  'targets': [
    {
      'target_name': 'speechapi',
      'sources': [ 'src/main.cc' ],
      'include_dirs': [
        '<!@(node -p "require(\'node-addon-api\').include")',
        '.cache/SpeechSDK/build/native/include/c_api',
        '.cache/SpeechSDK/build/native/include/cxx_api',
      ],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'conditions': [
        ['OS=="mac"', {
          'link_settings': {
            'libraries': [
              '-Wl,-rpath,@loader_path',
              '-lMicrosoft.CognitiveServices.Speech.core',
              '-lMicrosoft.CognitiveServices.Speech.extension.embedded.sr',
              '-lMicrosoft.CognitiveServices.Speech.extension.embedded.sr.runtime',
              # '-lMicrosoft.CognitiveServices.Speech.extension.embedded.tts',
              '-lMicrosoft.CognitiveServices.Speech.extension.onnxruntime',
              # '-lMicrosoft.CognitiveServices.Speech.extension.telemetry',
            ],
          },
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
          }
        }]
      ]
    },
    {
      "target_name": "action_before_build",
      "type": "none",
      'conditions': [
        ['OS=="mac"', {
          "copies": [{
              "files": [
                ".cache/SpeechSDK/runtimes/osx-arm64/native/libMicrosoft.CognitiveServices.Speech.core.dylib",
                ".cache/SpeechSDK/runtimes/osx-arm64/native/libMicrosoft.CognitiveServices.Speech.extension.embedded.sr.dylib",
                ".cache/SpeechSDK/runtimes/osx-arm64/native/libMicrosoft.CognitiveServices.Speech.extension.embedded.sr.runtime.dylib",
                # ".cache/SpeechSDK/runtimes/osx-arm64/native/libMicrosoft.CognitiveServices.Speech.extension.embedded.tts.dylib",
                ".cache/SpeechSDK/runtimes/osx-arm64/native/libMicrosoft.CognitiveServices.Speech.extension.onnxruntime.dylib",
                # ".cache/SpeechSDK/runtimes/osx-arm64/native/libMicrosoft.CognitiveServices.Speech.extension.telemetry.dylib",
              ],
              "destination": "<(PRODUCT_DIR)"
          }],
        }]
      ]
    }
  ]
}