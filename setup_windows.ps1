# Install choco and respective system packages.
Set-ExecutionPolicy Bypass -Scope Process -Force; iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))
Import-Module "$env:ChocolateyInstall\helpers\chocolateyProfile.psm1"
choco install colortool
choco install vscode
choco install python3
choco install vcbuildtools
choco install vswhere
choco install cmake --installargs '"ADD_CMAKE_TO_PATH=System"'
choco install bazel
choco install llvm
choco install pip

# Instal useful python packages.
pip install -r requirements.txt
