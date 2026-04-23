; WFS-DIY Plugin Suite - Inno Setup Installer Script
; Requires Inno Setup 6.x (https://jrsoftware.org/isinfo.php)
;
; Input: CMake Release build at ..\Builds\CMake-Windows\
; Output: Plugin\Installer\Output\WFS-DIY-Plugins-Setup-<version>.exe

#define MyAppName       "WFS-DIY Plugin Suite"
#define MyAppVersion    "0.0.1"
#define MyAppPublisher  "Pix et Bel"
#define MyAppURL        "https://wfs-diy.net"

; Path layouts produced by the JUCE CMake plugin targets (multi-config
; generators like Visual Studio put Release under .../Release/). The
; build-all script configures with Ninja and a single-config Release so
; the shorter path holds; fall back to the multi-config path if needed.
#define BuildRoot "..\Builds\CMake-Windows"
#define VstSuffix "_artefacts\Release\VST3"
#define StandaloneSuffix "_artefacts\Release\Standalone"
#define BridgeSuffix "Release"

[Setup]
AppId={{B1A4F3E2-5C9D-4A7E-BE1F-2C3D4E5F6A7B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
DefaultDirName={commoncf64}\VST3\WFS-DIY
DisableDirPage=yes
DefaultGroupName=WFS-DIY
OutputDir=Output
OutputBaseFilename=WFS-DIY-Plugins-Setup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "french";  MessagesFile: "compiler:Languages\French.isl"

[Files]
; Bridge library — installed alongside the VST3 bundles so BridgeLoader finds
; it via its walk-up-parents search. Host processes load this DLL at plugin
; instantiation time.
Source: "{#BuildRoot}\{#BridgeSuffix}\WFS-DIY-PluginBridge.dll"; DestDir: "{commoncf64}\VST3\WFS-DIY"; Flags: ignoreversion

; VST3 bundles — each JUCE CMake plugin target emits a .vst3 directory tree.
; All six live under one WFS-DIY\ subfolder for clean organisation, following
; the Izotope / iZotope / FabFilter convention.
Source: "{#BuildRoot}\WFSPluginMaster{#VstSuffix}\WFS-DIY Master.vst3\*";                 DestDir: "{commoncf64}\VST3\WFS-DIY\WFS-DIY Master.vst3";                 Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildRoot}\WFSPluginTrackCart{#VstSuffix}\WFS-DIY Track - Cartesian.vst3\*";    DestDir: "{commoncf64}\VST3\WFS-DIY\WFS-DIY Track - Cartesian.vst3";      Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildRoot}\WFSPluginTrackCyl{#VstSuffix}\WFS-DIY Track - Cylindrical.vst3\*";   DestDir: "{commoncf64}\VST3\WFS-DIY\WFS-DIY Track - Cylindrical.vst3";    Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildRoot}\WFSPluginTrackSph{#VstSuffix}\WFS-DIY Track - Spherical.vst3\*";     DestDir: "{commoncf64}\VST3\WFS-DIY\WFS-DIY Track - Spherical.vst3";      Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildRoot}\WFSPluginTrackAdmC{#VstSuffix}\WFS-DIY Track - ADM Cartesian.vst3\*"; DestDir: "{commoncf64}\VST3\WFS-DIY\WFS-DIY Track - ADM Cartesian.vst3";  Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#BuildRoot}\WFSPluginTrackAdmP{#VstSuffix}\WFS-DIY Track - ADM Polar.vst3\*";     DestDir: "{commoncf64}\VST3\WFS-DIY\WFS-DIY Track - ADM Polar.vst3";      Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; No Start Menu icons - plugins are scanned by host applications.
