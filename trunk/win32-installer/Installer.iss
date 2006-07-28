; Problems: README must have .txt ending, so that it could be opened

[Languages]
Name: "de"; MessagesFile: "languages\German.isl"
Name: "nl"; MessagesFile: "languages\Dutch.isl"
Name: "ru"; MessagesFile: "languages\Russian.isl"
Name: "pt"; MessagesFile: "languages\Portuguese.isl"
Name: "sk"; MessagesFile: "languages\Slovak.isl"
Name: "pl"; MessagesFile: "languages\Polish.isl"
Name: "no"; MessagesFile: "languages\Norwegian.isl"
Name: "it"; MessagesFile: "languages\Italian.isl"
Name: "hu"; MessagesFile: "languages\Hungarian.isl"
Name: "fr"; MessagesFile: "languages\French.isl"
Name: "fi"; MessagesFile: "languages\Finnish.isl"
Name: "dk"; MessagesFile: "languages\Danish.isl"
Name: "cz"; MessagesFile: "languages\Czech.isl"
Name: "es"; MessagesFile: "languages\Catalan.isl"
Name: "en"; MessagesFile: "languages\English.isl"

[Setup]
LanguageDetectionMethod=locale
AppName=Blobby Volley 2.0 Alpha 3
AppVerName=Blobby Volley 2.0 Alpha 3
DefaultDirName={pf}\Blobby Volley 2.0 Alpha 3
DefaultGroupName=Blobby Volley 2.0 Alpha 3
UninstallDisplayIcon={app}\blobby.exe
Compression=lzma
SolidCompression=yes

[Files]
Source: "../blobby.exe"; DestDir: "{app}"
Source: "../README"; DestDir: "{app}"; Flags: isreadme
Source: "../COPYING";DestDir: "{app}";
;Source: "SDL.dll"; DestDir: "{app}"
;Source: "SDL_net.dll"; DestDir: "{app}"
Source: "../data\*"; DestDir: "{app}\data"
