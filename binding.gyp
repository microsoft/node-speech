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
      'cflags': ['-std=c++17' ],
      'cflags_cc': ['-std=c++17' ],
      'defines': [ 'NAPI_CPP_EXCEPTIONS' ],
      'conditions': [
        ['OS=="mac"', {
          'link_settings': {
            'libraries': [
              '-Wl,-rpath,@loader_path',
              '-lMicrosoft.CognitiveServices.Speech.core',
              '-lMicrosoft.CognitiveServices.Speech.extension.embedded.sr',
              '-lMicrosoft.CognitiveServices.Speech.extension.embedded.sr.runtime',
              '-lMicrosoft.CognitiveServices.Speech.extension.onnxruntime',
            ],
          },
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
          }
        }],
        ["OS=='win'", {
          "defines": [
              "WINDOWS",
              "_HAS_EXCEPTIONS=1"
          ],
          "msvs_configuration_attributes": {
              "SpectreMitigation": "Spectre"
          },
          "msvs_settings": {
              "VCCLCompilerTool": {
                  "ExceptionHandling": 1,
                  'AdditionalOptions': [
                      '/guard:cf',
                      '-std:c++17',
                      '/we4244',
                      '/we4267',
                      '/ZH:SHA_256'
                  ],
              },
              'VCLinkerTool': {
                  'AdditionalOptions': [
                      '/guard:cf'
                  ]
              }
          },
        }],
        ['OS=="win" and target_arch=="arm64"', {
          'link_settings': {
            'libraries': [
              '<(module_root_dir)\\.cache\\SpeechSDK\\build\\native\\ARM64\\Release\\Microsoft.CognitiveServices.Speech.core.lib',
              '<(module_root_dir)\\.cache\\SpeechSDK\\build\\native\\ARM64\\Release\\Microsoft.CognitiveServices.Speech.extension.embedded.sr.lib',
              '<(module_root_dir)\\.cache\\SpeechSDK\\build\\native\\ARM64\\Release\\Microsoft.CognitiveServices.Speech.extension.onnxruntime.lib',
            ],
          },
        }],
        ['OS=="win" and target_arch=="x64"', {
          'link_settings': {
            'libraries': [
              '<(module_root_dir)\\.cache\\SpeechSDK\\build\\native\\x64\\Release\\Microsoft.CognitiveServices.Speech.core.lib',
              '<(module_root_dir)\\.cache\\SpeechSDK\\build\\native\\x64\\Release\\Microsoft.CognitiveServices.Speech.extension.embedded.sr.lib',
              '<(module_root_dir)\\.cache\\SpeechSDK\\build\\native\\x64\\Release\\Microsoft.CognitiveServices.Speech.extension.onnxruntime.lib',
            ],
          },
        }],
        ['OS=="win" and target_arch=="ia32"', {
          'link_settings': {
            'libraries': [
              '<(module_root_dir)\\.cache\\SpeechSDK\\build\\native\\Win32\\Release\\Microsoft.CognitiveServices.Speech.core.lib',
              '<(module_root_dir)\\.cache\\SpeechSDK\\build\\native\\Win32\\Release\\Microsoft.CognitiveServices.Speech.extension.embedded.sr.lib',
              '<(module_root_dir)\\.cache\\SpeechSDK\\build\\native\\Win32\\Release\\Microsoft.CognitiveServices.Speech.extension.onnxruntime.lib',
            ],
          },
        }],
        ['OS=="linux"', {
          "libraries": [
            "-Wl,-rpath,'$$ORIGIN'",
            '-lMicrosoft.CognitiveServices.Speech.core',
            '-lMicrosoft.CognitiveServices.Speech.extension.embedded.sr',
            '-lMicrosoft.CognitiveServices.Speech.extension.embedded.sr.runtime',
            '-lMicrosoft.CognitiveServices.Speech.extension.onnxruntime',
          ],
        }],
        ['OS=="linux" and target_arch=="x64"', {
          "ldflags": [
            "-L<(module_root_dir)/.cache/SpeechSDK/runtimes/linux-x64/native"
          ]
        }],
        ['OS=="linux" and target_arch=="arm64"', {
          "ldflags": [
            "-L<(module_root_dir)/.cache/SpeechSDK/runtimes/linux-arm64/native"
          ]
        }],
        ['OS=="linux" and target_arch=="armhf"', {
          "ldflags": [
            "-L<(module_root_dir)/.cache/SpeechSDK/runtimes/linux-arm/native"
          ]
        }],
      ]
    },
    {
      "target_name": "action_before_build",
      "type": "none",
      'conditions': [
        ['OS=="mac" and target_arch=="arm64"', {
          "copies": [{
              "files": [
                ".cache/SpeechSDK/runtimes/osx-arm64/native/libMicrosoft.CognitiveServices.Speech.core.dylib",
                ".cache/SpeechSDK/runtimes/osx-arm64/native/libMicrosoft.CognitiveServices.Speech.extension.embedded.sr.dylib",
                ".cache/SpeechSDK/runtimes/osx-arm64/native/libMicrosoft.CognitiveServices.Speech.extension.embedded.sr.runtime.dylib",
                ".cache/SpeechSDK/runtimes/osx-arm64/native/libMicrosoft.CognitiveServices.Speech.extension.onnxruntime.dylib",
              ],
              "destination": "<(PRODUCT_DIR)"
          }],
        }],
        ['OS=="mac" and target_arch=="x64"', {
          "copies": [{
              "files": [
                ".cache/SpeechSDK/runtimes/osx-x64/native/libMicrosoft.CognitiveServices.Speech.core.dylib",
                ".cache/SpeechSDK/runtimes/osx-x64/native/libMicrosoft.CognitiveServices.Speech.extension.embedded.sr.dylib",
                ".cache/SpeechSDK/runtimes/osx-x64/native/libMicrosoft.CognitiveServices.Speech.extension.embedded.sr.runtime.dylib",
                ".cache/SpeechSDK/runtimes/osx-x64/native/libMicrosoft.CognitiveServices.Speech.extension.onnxruntime.dylib",
              ],
              "destination": "<(PRODUCT_DIR)"
          }],
        }],
        ['OS=="win" and target_arch=="x64"', {
          "copies": [{
              "files": [
                ".cache/SpeechSDK/runtimes/win-x64/native/Microsoft.CognitiveServices.Speech.core.dll",
                ".cache/SpeechSDK/runtimes/win-x64/native/Microsoft.CognitiveServices.Speech.extension.audio.sys.dll",
                ".cache/SpeechSDK/runtimes/win-x64/native/Microsoft.CognitiveServices.Speech.extension.embedded.sr.dll",
                ".cache/SpeechSDK/runtimes/win-x64/native/Microsoft.CognitiveServices.Speech.extension.embedded.sr.runtime.dll",
                ".cache/SpeechSDK/runtimes/win-x64/native/Microsoft.CognitiveServices.Speech.extension.kws.dll",
                ".cache/SpeechSDK/runtimes/win-x64/native/Microsoft.CognitiveServices.Speech.extension.kws.ort.dll",
                ".cache/SpeechSDK/runtimes/win-x64/native/Microsoft.CognitiveServices.Speech.extension.onnxruntime.dll",
              ],
              "destination": "<(PRODUCT_DIR)"
          }],
        }],
        ['OS=="win" and target_arch=="ia32"', {
          "copies": [{
              "files": [
                ".cache/SpeechSDK/runtimes/win-x86/native/Microsoft.CognitiveServices.Speech.core.dll",
                ".cache/SpeechSDK/runtimes/win-x86/native/Microsoft.CognitiveServices.Speech.extension.audio.sys.dll",
                ".cache/SpeechSDK/runtimes/win-x86/native/Microsoft.CognitiveServices.Speech.extension.embedded.sr.dll",
                ".cache/SpeechSDK/runtimes/win-x86/native/Microsoft.CognitiveServices.Speech.extension.embedded.sr.runtime.dll",
                ".cache/SpeechSDK/runtimes/win-x86/native/Microsoft.CognitiveServices.Speech.extension.kws.dll",
                ".cache/SpeechSDK/runtimes/win-x86/native/Microsoft.CognitiveServices.Speech.extension.kws.ort.dll",
                ".cache/SpeechSDK/runtimes/win-x86/native/Microsoft.CognitiveServices.Speech.extension.onnxruntime.dll",
              ],
              "destination": "<(PRODUCT_DIR)"
          }],
        }],
        ['OS=="win" and target_arch=="arm64"', {
          "copies": [{
              "files": [
                ".cache/SpeechSDK/runtimes/win10-arm64/native/Microsoft.CognitiveServices.Speech.core.dll",
                ".cache/SpeechSDK/runtimes/win10-arm64/native/Microsoft.CognitiveServices.Speech.extension.audio.sys.dll",
                ".cache/SpeechSDK/runtimes/win10-arm64/native/Microsoft.CognitiveServices.Speech.extension.embedded.sr.dll",
                ".cache/SpeechSDK/runtimes/win10-arm64/native/Microsoft.CognitiveServices.Speech.extension.embedded.sr.runtime.dll",
                ".cache/SpeechSDK/runtimes/win10-arm64/native/Microsoft.CognitiveServices.Speech.extension.kws.dll",
                ".cache/SpeechSDK/runtimes/win10-arm64/native/Microsoft.CognitiveServices.Speech.extension.kws.ort.dll",
                ".cache/SpeechSDK/runtimes/win10-arm64/native/Microsoft.CognitiveServices.Speech.extension.onnxruntime.dll",
              ],
              "destination": "<(PRODUCT_DIR)"
          }],
        }],
        ['OS=="linux" and target_arch=="x64"', {
          "copies": [{
              "files": [
                ".cache/SpeechSDK/runtimes/linux-x64/native/libMicrosoft.CognitiveServices.Speech.core.so",
                ".cache/SpeechSDK/runtimes/linux-x64/native/libMicrosoft.CognitiveServices.Speech.extension.audio.sys.so",
                ".cache/SpeechSDK/runtimes/linux-x64/native/libMicrosoft.CognitiveServices.Speech.extension.embedded.sr.so",
                ".cache/SpeechSDK/runtimes/linux-x64/native/libMicrosoft.CognitiveServices.Speech.extension.embedded.sr.runtime.so",
                ".cache/SpeechSDK/runtimes/linux-x64/native/libMicrosoft.CognitiveServices.Speech.extension.kws.so"
                ".cache/SpeechSDK/runtimes/linux-x64/native/libMicrosoft.CognitiveServices.Speech.extension.kws.ort.so"
                ".cache/SpeechSDK/runtimes/linux-x64/native/libMicrosoft.CognitiveServices.Speech.extension.onnxruntime.so",
              ],
              "destination": "<(PRODUCT_DIR)"
          }],
        }],
        ['OS=="linux" and target_arch=="arm"', {
          "copies": [{
              "files": [
                ".cache/SpeechSDK/runtimes/linux-arm/native/libMicrosoft.CognitiveServices.Speech.core.so",
                ".cache/SpeechSDK/runtimes/linux-arm/native/libMicrosoft.CognitiveServices.Speech.extension.audio.sys.so",
                ".cache/SpeechSDK/runtimes/linux-arm/native/libMicrosoft.CognitiveServices.Speech.extension.embedded.sr.so",
                ".cache/SpeechSDK/runtimes/linux-arm/native/libMicrosoft.CognitiveServices.Speech.extension.embedded.sr.runtime.so",
                ".cache/SpeechSDK/runtimes/linux-arm/native/libMicrosoft.CognitiveServices.Speech.extension.kws.so",
                ".cache/SpeechSDK/runtimes/linux-arm/native/libMicrosoft.CognitiveServices.Speech.extension.kws.ort.so",
                ".cache/SpeechSDK/runtimes/linux-arm/native/libMicrosoft.CognitiveServices.Speech.extension.onnxruntime.so",
              ],
              "destination": "<(PRODUCT_DIR)"
          }],
        }],
        ['OS=="linux" and target_arch=="arm64"', {
          "copies": [{
              "files": [
                ".cache/SpeechSDK/runtimes/linux-arm64/native/libMicrosoft.CognitiveServices.Speech.core.so",
                ".cache/SpeechSDK/runtimes/linux-arm64/native/libMicrosoft.CognitiveServices.Speech.extension.audio.sys.so",
                ".cache/SpeechSDK/runtimes/linux-arm64/native/libMicrosoft.CognitiveServices.Speech.extension.embedded.sr.so",
                ".cache/SpeechSDK/runtimes/linux-arm64/native/libMicrosoft.CognitiveServices.Speech.extension.embedded.sr.runtime.so",
                ".cache/SpeechSDK/runtimes/linux-arm64/native/libMicrosoft.CognitiveServices.Speech.extension.kws.so",
                ".cache/SpeechSDK/runtimes/linux-arm64/native/libMicrosoft.CognitiveServices.Speech.extension.kws.ort.so",
                ".cache/SpeechSDK/runtimes/linux-arm64/native/libMicrosoft.CognitiveServices.Speech.extension.onnxruntime.so",
              ],
              "destination": "<(PRODUCT_DIR)"
          }],
        }]
      ]
    }
  ]
}