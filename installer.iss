[Setup]
AppName=Awake Buddy
AppVersion=1.0
AppPublisher=Awake Buddy
DefaultDirName={autopf}\Awake Buddy
DefaultGroupName=Awake Buddy
OutputDir=installer_output
OutputBaseFilename=AwakeBuddy-Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayName=Awake Buddy
PrivilegesRequired=lowest
SetupIconFile=awakebuddy.ico
WizardStyle=modern

[Files]
Source: "deploy\*"; DestDir: "{app}"; Flags: recursesubdirs

[Icons]
Name: "{group}\Awake Buddy"; Filename: "{app}\Awake-Buddy.exe"
Name: "{group}\Uninstall Awake Buddy"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Awake Buddy"; Filename: "{app}\Awake-Buddy.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Run]
Filename: "{app}\Awake-Buddy.exe"; Description: "Launch Awake Buddy"; Flags: nowait postinstall skipifsilent
