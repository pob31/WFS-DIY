; WFS-DIY Inno Setup Installer Script
; Requires Inno Setup 6.x (https://jrsoftware.org/isinfo.php)

#define MyAppName "WFS-DIY"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "Pix et Bel"
#define MyAppURL "https://wfs-diy.net"
#define MyAppExeName "WFS-DIY.exe"

[Setup]
AppId={{E7A3F2C1-8B4D-4E6F-9A5C-1D2E3F4A5B6C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=Output
OutputBaseFilename=WFS-DIY-Setup-{#MyAppVersion}
SetupIconFile=..\Builds\VisualStudio2022\icon.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
ChangesAssociations=yes

; GPL requires the license to be conveyed with the binaries. Display on the
; standard Inno license page and ship it (plus README + third-party notices)
; alongside the installed app.
LicenseFile=..\LICENSE

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Main executable (Release build)
Source: "..\Builds\VisualStudio2022\x64\Release\App\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

; Language files
Source: "..\Resources\lang\*.json"; DestDir: "{app}\lang"; Flags: ignoreversion recursesubdirs

; App icon (for folder branding)
Source: "..\Builds\VisualStudio2022\icon.ico"; DestDir: "{app}"; DestName: "WFS-DIY.ico"; Flags: ignoreversion

; Legal / reference documents bundled with the app, renamed to .txt so Windows
; Notepad opens them on double-click without a "choose an app" prompt.
Source: "..\LICENSE";                DestDir: "{app}"; DestName: "LICENSE.txt";              Flags: ignoreversion
Source: "..\README.md";              DestDir: "{app}"; DestName: "README.txt";               Flags: ignoreversion
Source: "..\THIRD_PARTY_NOTICES.md"; DestDir: "{app}"; DestName: "THIRD_PARTY_NOTICES.txt";  Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
; .wfs file association
Root: HKA; Subkey: "Software\Classes\.wfs"; ValueType: string; ValueName: ""; ValueData: "WFS-DIY.Project"; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\WFS-DIY.Project"; ValueType: string; ValueName: ""; ValueData: "WFS-DIY Project"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\WFS-DIY.Project\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\WFS-DIY.ico,0"
Root: HKA; Subkey: "Software\Classes\WFS-DIY.Project\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    // Notify the shell that file associations have changed
    // SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0)
    // Inno Setup handles this automatically with ChangesAssociations=yes
  end;
end;
