$SDK_VERSION = "1.37.0"
$TEMP_PACKAGE_DIR = "temp_packages"

dotnet add package Microsoft.CognitiveServices.Speech --version $SDK_VERSION --package-directory $TEMP_PACKAGE_DIR
dotnet add package Microsoft.CognitiveServices.Speech.Extension.Embedded.SR --version $SDK_VERSION --package-directory $TEMP_PACKAGE_DIR
dotnet add package Microsoft.CognitiveServices.Speech.Extension.Embedded.TTS --version $SDK_VERSION --package-directory $TEMP_PACKAGE_DIR
dotnet add package Microsoft.CognitiveServices.Speech.Extension.ONNX.Runtime --version $SDK_VERSION --package-directory $TEMP_PACKAGE_DIR

New-Item -Path .cache/packages -Type Directory
New-Item -Path .cache/SpeechSDK -Type Directory
Get-ChildItem -Path $TEMP_PACKAGE_DIR -Recurse -Filter "*.nupkg" | Foreach-Object { cp $_ .cache/packages }

rm -recurse -Force $TEMP_PACKAGE_DIR
