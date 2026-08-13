; PDF 翻译阅读器 Inno Setup 脚本
; 用法: ISCC.exe /Q PdfTranslator.iss（静默构建，退出码即结果）
#define MyAppName "PDF翻译阅读器"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "Graturate"
#define MyAppExeName "PdfTranslator.exe"

[Setup]
; 固定 GUID：生成一次后永不更改
AppId={{8F3A4C6E-5B2D-4E1A-9C7F-2D4E6A8B0C1D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={localappdata}\Programs\PdfTranslator
DefaultGroupName={#MyAppName}
PrivilegesRequired=lowest
OutputDir=..\installer
OutputBaseFilename=PdfTranslator-Setup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "chinesesimplified"; MessagesFile: "ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加任务："

[Files]
Source: "..\dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "运行 {#MyAppName}"; Flags: nowait postinstall skipifsilent
