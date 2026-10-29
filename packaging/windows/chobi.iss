; SPDX-License-Identifier: GPL-3.0-or-later
;
; Inno Setup script for the Windows installer.
;   iscc /DAppVersion=0.2.0 /DSourceDir=..\..\dist\chobi-windows-x64 /DOutputDir=..\..\dist chobi.iss
;
; Per-user install (no administrator rights, no UAC prompt), so the in-app updater can run a
; newer installer silently: chobi.exe starts it with /SILENT /CLOSEAPPLICATIONS and quits.
; Never change AppId: it is how upgrades find the existing installation.

#ifndef AppVersion
  #error Pass /DAppVersion=x.y.z
#endif
#ifndef SourceDir
  #define SourceDir "..\..\dist\chobi-windows-x64"
#endif
#ifndef OutputDir
  #define OutputDir "..\..\dist"
#endif

[Setup]
AppId={{DD668F25-9192-46DA-B38A-F1838C565198}
AppName=Chobi
AppVersion={#AppVersion}
AppVerName=Chobi {#AppVersion}
AppPublisher=Chobi contributors
AppPublisherURL=https://github.com/dev-taosif-net/chobi
AppSupportURL=https://github.com/dev-taosif-net/chobi/issues
AppUpdatesURL=https://github.com/dev-taosif-net/chobi/releases
DefaultDirName={localappdata}\Programs\Chobi
DisableProgramGroupPage=yes
DisableDirPage=auto
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#OutputDir}
OutputBaseFilename=Chobi-{#AppVersion}-windows-x64-setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
LicenseFile=..\..\LICENSE
UninstallDisplayIcon={app}\chobi.exe
UninstallDisplayName=Chobi
ChangesAssociations=yes
CloseApplications=force
RestartApplications=no
MinVersion=10.0

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[INI]
; Tells chobi.exe it was installed by this installer and may update itself.
Filename: "{app}\chobi-install.ini"; Section: "install"; Key: "kind"; String: "installer"

[InstallDelete]
; DLL set changes between releases; start from a clean folder so stale libraries never linger.
Type: filesandordirs; Name: "{app}\*.dll"
Type: filesandordirs; Name: "{app}\platforms"
Type: filesandordirs; Name: "{app}\styles"
Type: filesandordirs; Name: "{app}\imageformats"
Type: filesandordirs; Name: "{app}\iconengines"
Type: filesandordirs; Name: "{app}\tls"

[Icons]
Name: "{autoprograms}\Chobi"; Filename: "{app}\chobi.exe"
Name: "{autodesktop}\Chobi"; Filename: "{app}\chobi.exe"; Tasks: desktopicon

[Registry]
; "Open with → Chobi" for video files, per user. Chobi does not take over defaults; Windows
; lets the user choose. Keep the extension list in sync with src/core/MediaFormats.cpp.
Root: HKCU; Subkey: "Software\Classes\Chobi.Video"; ValueType: string; ValueData: "Video (Chobi)"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Chobi.Video\DefaultIcon"; ValueType: string; ValueData: """{app}\chobi.exe"",0"
Root: HKCU; Subkey: "Software\Classes\Chobi.Video\shell\open\command"; ValueType: string; ValueData: """{app}\chobi.exe"" -- ""%1"""
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe"; ValueType: string; ValueName: "FriendlyAppName"; ValueData: "Chobi"; Flags: uninsdeletekey
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\shell\open\command"; ValueType: string; ValueData: """{app}\chobi.exe"" -- ""%1"""
Root: HKCU; Subkey: "Software\Classes\.3g2\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".3g2"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.3gp\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".3gp"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.asf\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".asf"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.avi\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".avi"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.divx\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".divx"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.dv\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".dv"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.f4v\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".f4v"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.flv\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".flv"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.m2t\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".m2t"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.m2ts\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".m2ts"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.m2v\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".m2v"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.m4v\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".m4v"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.mkv\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".mkv"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.mov\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".mov"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.mp4\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".mp4"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.mpe\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".mpe"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.mpeg\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".mpeg"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.mpg\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".mpg"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.mts\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".mts"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.mxf\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".mxf"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.nut\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".nut"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.ogm\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".ogm"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.ogv\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".ogv"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.qt\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".qt"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.rm\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".rm"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.rmvb\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".rmvb"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.ts\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".ts"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.vob\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".vob"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.webm\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".webm"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.wmv\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".wmv"; ValueData: ""
Root: HKCU; Subkey: "Software\Classes\.wtv\OpenWithProgids"; ValueType: string; ValueName: "Chobi.Video"; ValueData: ""; Flags: uninsdeletevalue
Root: HKCU; Subkey: "Software\Classes\Applications\chobi.exe\SupportedTypes"; ValueType: string; ValueName: ".wtv"; ValueData: ""

[Run]
; Runs after interactive installs (checkbox) and after silent in-app updates.
Filename: "{app}\chobi.exe"; Description: "{cm:LaunchProgram,Chobi}"; Flags: nowait postinstall
