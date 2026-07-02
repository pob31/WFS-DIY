; WFS-DIY Inno Setup Installer Script
; Requires Inno Setup 6.x (https://jrsoftware.org/isinfo.php)

#define MyAppName "WFS-DIY"
; Version: overridable from the command line (CI passes /DMyAppVersion=<tag>
; so the installer matches the release). Default is for local manual builds.
#ifndef MyAppVersion
#define MyAppVersion "1.0.0beta26"
#endif
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

; GPU acceleration plugins, staged next to the exe by
; tools\windows\build-gpu-plugins.ps1 (run it after the Release app build; the
; release workflow runs it too). The app dlopens these by name from beside the
; exe. All GPU files below are optional (skipifsourcedoesntexist) so a CPU-only
; build still produces an installer.
;   wfs_cuda.dll - NVIDIA plugin; loads cudart64 + nvrtc64 (bundled below) plus
;                  nvcuda.dll (the driver API, shipped with the NVIDIA driver and
;                  NOT redistributed here).
;   wfs_hip.dll  - AMD plugin; loads amdhip64 / hiprtc from the user's AMD HIP
;                  runtime (NOT bundled). CI copies a prebuilt copy from
;                  tools\windows\prebuilt since the runner has no HIP SDK.
Source: "..\Builds\VisualStudio2022\x64\Release\App\wfs_cuda.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "..\Builds\VisualStudio2022\x64\Release\App\wfs_hip.dll";  DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

; CUDA Runtime + NVRTC DLLs that wfs_cuda.dll loads at runtime (staged next to it
; by build-gpu-plugins.ps1), so a machine with only the NVIDIA driver still gets
; GPU acceleration. Ship the main nvrtc64 only -- the .alt forward-compat copy is
; ~85 MB and not needed.
Source: "..\Builds\VisualStudio2022\x64\Release\App\cudart64_*.dll";         DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "..\Builds\VisualStudio2022\x64\Release\App\nvrtc64_*.dll";          DestDir: "{app}"; Excludes: "*.alt.dll"; Flags: ignoreversion skipifsourcedoesntexist
Source: "..\Builds\VisualStudio2022\x64\Release\App\nvrtc-builtins64_*.dll"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist

; Language files
Source: "..\Resources\lang\*.json"; DestDir: "{app}\lang"; Flags: ignoreversion recursesubdirs

; MCP knowledge-base resources (AI control surface) + the generated tool
; surface. MainComponent resolves these at {app}\MCP\... via
; exeDir.getChildFile("MCP/...") -- without these, MCP tool-calling and
; knowledge-resource lookups silently find nothing in the installed app.
Source: "..\Documentation\MCP\resources\*";              DestDir: "{app}\MCP\resources"; Flags: ignoreversion recursesubdirs
Source: "..\Source\Network\MCP\generated_tools.json";    DestDir: "{app}\MCP";           Flags: ignoreversion

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
