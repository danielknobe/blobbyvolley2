; Problems: README must have .txt ending, so that it could be opened
#define FullyQualifiedAppName "Blobby Volley 2 Version 1.1.1"

[Languages]
// Languages which are also supported by the game
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "de"; MessagesFile: "compiler:Languages\German.isl"
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"
Name: "it"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "cz"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "pl"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "es"; MessagesFile: "compiler:Languages\Spanish.isl"

[Icons]
Name: "{group}\{#FullyQualifiedAppName}"; Filename: "{app}\blobby.exe"; WorkingDir: "{app}"
Name: "{group}\Uninstall"; Filename: "{uninstallexe}"

[Setup]
LanguageDetectionMethod=locale
AppName={#FullyQualifiedAppName}
AppVerName={#FullyQualifiedAppName}
DefaultDirName={pf}\{#FullyQualifiedAppName}
DefaultGroupName={#FullyQualifiedAppName}
UninstallDisplayIcon={app}\blobby.exe
Compression=lzma
SolidCompression=yes
SourceDir="../"

[dirs]
Name:"{app}/data/replays"
Name:"{app}/data/scripts"
Name:"{app}/data/gfx"
Name:"{app}/data/sounds"
Name:"{app}/data/backgrounds"

[Files]
Source: "../blobby.exe"; DestDir: "{app}"
Source: "../blobby-server.exe"; DestDir: "{app}"
Source: "../README.txt"; DestDir: "{app}"; Flags: isreadme
Source: "../COPYING.txt";DestDir: "{app}";
Source: "../AUTHORS.txt";DestDir: "{app}";
Source: "../SDL.dll"; DestDir: "{app}"
Source: "../libphysfs.dll"; DestDir: "{app}"
Source: "../data/*"; DestDir: "{app}/data"

Source: "../doc/*"; DestDir: "{app}/doc"
