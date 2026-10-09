#define StageDir GetEnv("NETWORKCONFIG_STAGE")
#define ReleaseDir GetEnv("NETWORKCONFIG_RELEASE")
#if StageDir == ""
  #error NETWORKCONFIG_STAGE is required
#endif
#if ReleaseDir == ""
  #error NETWORKCONFIG_RELEASE is required
#endif

[Setup]
AppId={{4B654431-9B84-49CA-A31A-6508D8629B03}
AppName=NetworkConfigManager
AppVersion=2.2.0
AppPublisher=mawenshui
DefaultDirName={localappdata}\Programs\NetworkConfigManager
DefaultGroupName=NetworkConfigManager
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir={#ReleaseDir}
OutputBaseFilename=NetworkConfigManager-Setup-2.2.0-win64
UninstallDisplayIcon={app}\NetworkConfigManager.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
CloseApplications=yes

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\NetworkConfigManager"; Filename: "{app}\NetworkConfigManager.exe"
Name: "{autodesktop}\NetworkConfigManager"; Filename: "{app}\NetworkConfigManager.exe"; Tasks: desktopicon

[Tasks]
Name: desktopicon; Description: "创建桌面快捷方式"; GroupDescription: "其他选项："
