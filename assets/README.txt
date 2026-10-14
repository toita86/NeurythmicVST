This is a VST3 audio plugin named Neurythmic, developed by Eduard Brahas (aka
Toita86) on GitHub.

To install it on your system:

1. Copy the .vst3 bundle to

    ~/Library/Audio/Plug-Ins/VST3/
    if you're on macOS,

    %LOCALAPPDATA%/Programs/Common/VST3/
    if you're on Windows,

    ~/.vst3/
    if you're on Linux.

    Depending on the operating system, this operation may require elevated
    permissions.

2. Reload your DAW.

Notes:

- The macOS build is a universal binary (Apple Silicon + Intel), but it is not
  signed or notarized. macOS Gatekeeper may quarantine it on first launch;
  right-click the bundle and choose "Open" to allow it.
- The Windows build is not code-signed, so Windows SmartScreen may warn you the
  first time you load it.
- Linux users need libfreetype6 and libasound2 present at plugin load time
  (these are normally installed on desktop distributions).

All rights reserved.
