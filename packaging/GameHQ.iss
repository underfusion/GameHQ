#ifndef AppVersion
  #error AppVersion is required
#endif
#ifndef AppVersionInfo
  #error AppVersionInfo is required
#endif
#ifndef InstallerAppId
  #error InstallerAppId is required
#endif
#ifndef PayloadRoot
  #error PayloadRoot is required
#endif
#ifndef ReleaseOutput
  #error ReleaseOutput is required
#endif
#ifndef SetupBaseName
  #error SetupBaseName is required
#endif

[Setup]
AppId={{#InstallerAppId}
AppName=GameHQ
AppVersion={#AppVersion}
AppVerName=GameHQ {#AppVersion}
VersionInfoVersion={#AppVersionInfo}
VersionInfoCompany=underfusion
VersionInfoDescription=GameHQ for Windows Setup
VersionInfoProductName=GameHQ
AppPublisher=underfusion
AppPublisherURL=https://github.com/underfusion/GameHQ
AppSupportURL=https://github.com/underfusion/GameHQ/issues
AppUpdatesURL=https://github.com/underfusion/GameHQ/releases
DefaultDirName={localappdata}\Programs\GameHQ
DefaultGroupName=GameHQ
PrivilegesRequired=lowest
Uninstallable=yes
UsePreviousAppDir=yes
CloseApplications=no
RestartApplications=no
AppMutex={code:ApplicationMutexName}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.18362
OutputDir={#ReleaseOutput}
OutputBaseFilename={#SetupBaseName}
SetupIconFile={#SourcePath}\..\assets\icons\gamehq.ico
UninstallDisplayIcon={app}\GameHQ.exe
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
DisableProgramGroupPage=yes
DisableWelcomePage=no
AllowNoIcons=yes

[Files]
Source: "{#PayloadRoot}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\GameHQ"; Filename: "{app}\GameHQ.exe"
Name: "{autodesktop}\GameHQ"; Filename: "{app}\GameHQ.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"; Flags: unchecked

[Registry]
Root: HKCU; Subkey: "Software\underfusion\GameHQ"; ValueType: string; ValueName: "InstallLocation"; ValueData: "{app}"
Root: HKCU; Subkey: "Software\underfusion\GameHQ"; ValueType: string; ValueName: "Version"; ValueData: "{#AppVersion}"
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\GameHQ.exe"; ValueType: string; ValueName: ""; ValueData: "{app}\GameHQ.exe"
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\GameHQ.exe"; ValueType: string; ValueName: "Path"; ValueData: "{app}"

[Run]
Filename: "{app}\GameHQ.exe"; Description: "Launch GameHQ"; Flags: nowait postinstall skipifsilent

[Code]
const
  ExitAppRunning = 20;
  ExitUpdateActive = 21;

procedure ExitProcess(ExitCode: Integer);
  external 'ExitProcess@kernel32.dll stdcall';

procedure FailSilent(ExitCode: Integer);
begin
  if WizardSilent then
    ExitProcess(ExitCode);
end;

function ApplicationMutexName(Param: String): String;
begin
  { Interactive Setup and Uninstall use Inno's native AppMutex prompt. Silent
    Setup reaches PrepareToInstall so automation receives reserved code 20. }
  if WizardSilent then
    Result := ''
  else
    Result := 'Local\GameHQApplicationActive';
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := '';
  if CheckForMutexes('Local\GameHQApplicationActive') then
  begin
    FailSilent(ExitAppRunning);
    Result := 'GameHQ is running. Close it normally, then run Setup again.';
    Exit;
  end;
  if CheckForMutexes('Local\GameHQUpdaterActive') or
     FileExists(ExpandConstant('{app}\.update\maintenance.lock')) then
  begin
    FailSilent(ExitUpdateActive);
    Result := 'A GameHQ update or recovery transaction is active. Let it finish, then run Setup again.';
  end;
end;

function InitializeUninstall: Boolean;
begin
  Result := True;
  if CheckForMutexes('Local\GameHQApplicationActive') then
  begin
    if UninstallSilent then
      ExitProcess(ExitAppRunning);
    MsgBox('GameHQ is running. Close it normally before uninstalling.', mbError, MB_OK);
    Result := False;
  end;
end;

function CommandTargetsThisInstall(CommandLine: String): Boolean;
var
  Candidate: String;
  ClosingQuote, SpaceAt: Integer;
begin
  Result := False;
  Candidate := Trim(CommandLine);
  if Candidate = '' then
    Exit;

  if Candidate[1] = '"' then
  begin
    Delete(Candidate, 1, 1);
    ClosingQuote := Pos('"', Candidate);
    if ClosingQuote = 0 then
      Exit;
    Candidate := Copy(Candidate, 1, ClosingQuote - 1);
  end
  else
  begin
    SpaceAt := Pos(' ', Candidate);
    if SpaceAt > 0 then
      Candidate := Copy(Candidate, 1, SpaceAt - 1);
  end;

  Result := CompareText(ExpandFileName(Candidate),
    ExpandFileName(ExpandConstant('{app}\GameHQ.exe'))) = 0;
end;

procedure RemoveOwnedIntegration;
var
  Value: String;
  ProductKey, AppPathKey, RunKey: String;
begin
  ProductKey := 'Software\underfusion\GameHQ';
  AppPathKey := 'Software\Microsoft\Windows\CurrentVersion\App Paths\GameHQ.exe';
  RunKey := 'Software\Microsoft\Windows\CurrentVersion\Run';

  if RegQueryStringValue(HKCU, ProductKey, 'InstallLocation', Value) and
     (CompareText(RemoveBackslashUnlessRoot(Value),
       RemoveBackslashUnlessRoot(ExpandConstant('{app}'))) = 0) then
  begin
    RegDeleteValue(HKCU, ProductKey, 'InstallLocation');
    RegDeleteValue(HKCU, ProductKey, 'Version');
    RegDeleteKeyIfEmpty(HKCU, ProductKey);
  end;

  if RegQueryStringValue(HKCU, AppPathKey, '', Value) and
     (CompareText(ExpandFileName(Value),
       ExpandFileName(ExpandConstant('{app}\GameHQ.exe'))) = 0) then
  begin
    RegDeleteValue(HKCU, AppPathKey, '');
    RegDeleteValue(HKCU, AppPathKey, 'Path');
    RegDeleteKeyIfEmpty(HKCU, AppPathKey);
  end;

  if RegQueryStringValue(HKCU, RunKey, 'GameHQ', Value) and
     CommandTargetsThisInstall(Value) then
    RegDeleteValue(HKCU, RunKey, 'GameHQ');
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    RemoveOwnedIntegration;
end;
