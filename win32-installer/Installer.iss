; Problems: README must have .txt ending, so that it could be opened
[Languages]
Name: "de"; MessagesFile: "languages\German.isl"
;Name: "nl"; MessagesFile: "languages\Dutch.isl"
;Name: "ru"; MessagesFile: "languages\Russian.isl"
;Name: "pt"; MessagesFile: "languages\Portuguese.isl"
;Name: "sk"; MessagesFile: "languages\Slovak.isl"
Name: "pl"; MessagesFile: "languages\Polish.isl"
;Name: "no"; MessagesFile: "languages\Norwegian.isl"
Name: "it"; MessagesFile: "languages\Italian.isl"
;Name: "hu"; MessagesFile: "languages\Hungarian.isl"
;Name: "fr"; MessagesFile: "languages\French.isl"
;Name: "fi"; MessagesFile: "languages\Finnish.isl"
;Name: "dk"; MessagesFile: "languages\Danish.isl"
;Name: "cz"; MessagesFile: "languages\Czech.isl"
;Name: "es"; MessagesFile: "languages\Catalan.isl"
Name: "en"; MessagesFile: "languages\English.isl"

[Icons]
Name: "{group}\Blobby Volley 2 Version 1.0RC3"; Filename: "{app}\blobby.exe"; WorkingDir: "{app}"
Name: "{group}\Uninstall"; Filename: "{uninstallexe}"

[Setup]
LanguageDetectionMethod=locale
AppName=Blobby Volley 2 Version 1.0RC3
AppVerName=Blobby Volley 2 Version 1.0RC3
DefaultDirName={pf}\Blobby Volley 2 Version 1.0RC3
DefaultGroupName=Blobby Volley 2 Version 1.0RC3
UninstallDisplayIcon={app}\blobby.exe
Compression=lzma
SolidCompression=yes

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
