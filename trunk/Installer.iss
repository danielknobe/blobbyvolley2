; -- Example1.iss --
; Demonstrates copying 3 files and creating an icon.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

[Setup]
AppName=Blobby Volley 2.0 Alpha 2
AppVerName=Blobby Volley 2.0 Alpha 2
DefaultDirName={pf}\Blobby Volley 2.0 Alpha 2
DefaultGroupName=Blobby Volley 2.0 Alpha 2
UninstallDisplayIcon={app}\blobby.exe
Compression=lzma
SolidCompression=yes

[Files]
Source: "blobby.exe"; DestDir: "{app}"
Source: "Readme.txt"; DestDir: "{app}"
Source: "License.txt";DestDir: "{app}"; Flags: isreadme
Source: "SDL.dll"; DestDir: "{app}"
Source: "SDL_net.dll"; DestDir: "{app}"
Source: "data\*"; DestDir: "{app}\data"
