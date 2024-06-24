# Contributing to node-speech

## Building the project

### Prerequisites

- [.NET SDK](https://dotnet.microsoft.com/en-us/download) (The default recommended LTS version should work)
- [pwsh](https://learn.microsoft.com/en-us/powershell/scripting/install/installing-powershell?view=powershell-7.4)
- Node.js and npm

### Steps

1. Clone the repository: `git clone https://github.com/microsoft/node-speech.git`
2. Run the powershell script, `setup-packages.ps1`. This script uses dotnet to add various Speech SDK packages required for this repository, and copies the nupkg files to `.cache/packages`.
3. Run the install script: `npm i`.
