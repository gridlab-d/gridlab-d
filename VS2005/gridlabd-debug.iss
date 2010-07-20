[Setup]
Compression=lzma
OutputDir=.

; ++RELEASE++
;SourceDir=.\Win32\Release
;OutputBaseFilename=gridlabd-0.1
;AppName=GridLAB-D
;AppVerName=GridLAB-D 0.1
; --RELEASE--

; ++DEBUG++
SourceDir=.\Win32\Debug
OutputBaseFilename=gridlabd-debug-1.1
AppName=GridLAB-D Debug
AppVerName=GridLAB-D 1.1 Debug
; --DEBUG--

AppVersion=0.1
AppPublisher=Pacific Northwest National Laboratory, operated by Battelle
AppCopyright=Copyright (C) 2004-2007 Battelle Memorial Institute
AppPublisherURL=http://www.pnl.gov
;AppReadmeFile={app}\README.TXT
;AppSupportURL=http://gridlab.pnl.gov/support
;AppUpdatesURL=http://gridlab.pnl.gov//downloads
;AppComments=<Include application comments here>
;AppContact=GridLAB-D Development Team <gridlabd@pnl.gov>
VersionInfoDescription=Gridlab-D - Grid Simulator
VersionInfoVersion=1.1.0.0

;AppMutex=<Mutex string to prevent installation while application is running>
DefaultDirName={pf}\GridLAB-D
DefaultGroupName=GridLAB-D
PrivilegesRequired=admin
AllowNoIcons=yes
;UninstallDisplayIcon={app}\bin\gridlabd.exe

[Types]
Name: typical; Description: Typical Installation
Name: custom; Description: Custom Installation; Flags: iscustom

[Components]
Name: core; Description: Core Files; Types: typical custom; Flags: fixed
; ++DEBUG++
Name: debug; Description: Debug Files; Types: typical custom
; --DEBUG--
Name: modules; Description: Modules; Types: typical custom
Name: modules\climate; Description: Climate Module; Types: typical custom
Name: modules\commercial; Description: Commercial Module; Types: typical custom
Name: modules\network; Description: Network Module; Types: typical custom
Name: modules\plc; Description: PLC Module; Types: typical custom
Name: modules\market; Description: Market Module; Types: typical custom
Name: modules\powerflow; Description: Power Flow Module; Types: typical custom
Name: modules\residential; Description: Residential Module; Types: typical custom
Name: modules\tape; Description: Tape Module; Types: typical custom
Name: samples; Description: Sample Models; Types: typical custom

[Tasks]
Name: environment; Description: Add GridLAB-D to &PATH environment variable; GroupDescription: Environment
Name: desktopicon; Description: Create a &desktop icon; GroupDescription: Additional icons:
Name: desktopicon\common; Description: For all users; GroupDescription: Additional icons:; Flags: exclusive
Name: desktopicon\user; Description: For the current user only; GroupDescription: Additional icons:; Flags: exclusive unchecked
Name: quicklaunchicon; Description: Create a &Quick Launch icon; GroupDescription: Additional icons:; Flags: unchecked

[Dirs]
Name: {app}\bin
Name: {app}\etc
Name: {app}\etc\tmy
Name: {app}\lib
Name: {userdocs}\GridLAB-D

[Files]
;; Microsoft CRT redist
Source: Microsoft.VC80.DebugCRT.manifest; DestDir: {app}\bin; Flags: ignoreversion; Components: core
Source: msvcp80d.dll; DestDir: {app}\bin; Flags: ignoreversion; Components: core
Source: msvcr80d.dll; DestDir: {app}\bin; Flags: ignoreversion; Components: core

;; core files
Source: gridlabd.exe; DestDir: {app}\bin; Flags: ignoreversion; Components: core
; ++RELEASE++
;Source: "xerces-c_2_8.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: core
;Source: "cppunit.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: core
; --RELEASE--
; ++DEBUG++
Source: xerces-c_2_8D.dll; DestDir: {app}\bin; Flags: ignoreversion; Components: core
Source: cppunitd.dll; DestDir: {app}\bin; Flags: ignoreversion; Components: core
; --DEBUG--
Source: unitfile.txt; DestDir: {app}\etc; Components: core
Source: tzinfo.txt; DestDir: {app}\etc; Components: core
Source: climate.dll; DestDir: {app}\lib; Flags: ignoreversion; Components: modules\climate
Source: commercial.dll; DestDir: {app}\lib; Flags: ignoreversion; Components: modules\commercial
Source: climateview.dll; DestDir: {app}\lib; Flags: ignoreversion
Source: network.dll; DestDir: {app}\lib; Flags: ignoreversion; Components: modules\network
Source: plc.dll; DestDir: {app}\lib; Flags: ignoreversion; Components: modules\plc
Source: market.dll; DestDir: {app}\lib; Flags: ignoreversion; Components: modules\market
Source: powerflow.dll; DestDir: {app}\lib; Flags: ignoreversion; Components: modules\powerflow
Source: residential.dll; DestDir: {app}\lib; Flags: ignoreversion; Components: modules\residential
Source: generators.dll; DestDir: {app}\lib; Flags: ignoreversion
Source: tape.dll; DestDir: {app}\lib; Flags: ignoreversion; Components: modules\tape

;; sample files
Source: tape_file.dll; DestDir: {app}\lib; Flags: ignoreversion
Source: tape_memory.dll; DestDir: {app}\lib; Flags: ignoreversion
Source: tape_odbc.dll; DestDir: {app}\lib; Flags: ignoreversion
Source: glmjava.dll; DestDir: {app}\lib; Flags: ignoreversion
Source: network.dll; DestDir: {app}\lib; Flags: ignoreversion
Source: sample.dll; DestDir: {app}\lib; Flags: ignoreversion
Source: ..\..\..\models\powerflow_IEEE_4node.glm; DestDir: {app}\samples; Components: samples
Source: ..\..\..\models\residential_loads.glm; DestDir: {app}\samples
Source: ..\..\..\models\powerflow_IEEE_37node.glm; DestDir: {app}\samples
Source: ..\..\..\models\lighting.player; DestDir: {app}\samples
Source: ..\..\..\test\shapes\*.*; DestDir: {app}\samples\shapes; Components: samples

;; debug files
; ++DEBUG++
Source: gridlabd.pdb; DestDir: {app}\lib; Components: core debug
Source: xerces-c_2_8D.pdb; DestDir: {app}\lib; Components: core
Source: cppunitd.pdb; DestDir: {app}\lib; Components: core
Source: climate.pdb; DestDir: {app}\lib; Components: modules\climate debug
Source: commercial.pdb; DestDir: {app}\lib; Components: modules\commercial debug
Source: network.pdb; DestDir: {app}\lib; Components: modules\network debug
Source: plc.pdb; DestDir: {app}\lib; Components: modules\plc debug
Source: market.pdb; DestDir: {app}\lib; Components: modules\market debug
Source: powerflow.pdb; DestDir: {app}\lib; Components: modules\powerflow debug
Source: residential.pdb; DestDir: {app}\lib; Components: modules\residential debug
Source: tape.pdb; DestDir: {app}\lib; Components: modules\tape debug
; --DEBUG--
Source: ..\..\..\models\dryer.shape; DestDir: {app}\samples
Source: ..\..\..\README-WINDOWS.txt; DestDir: {app}

[Registry]
Root: HKCU; SubKey: Environment; ValueType: string; ValueName: GLPATH; ValueData: "{app}\bin;{app}\etc;{app}\lib;{app}\samples"; Flags: uninsdeletevalue; Check: not (IsAdminLoggedOn() or IsPowerUserLoggedOn()); AfterInstall: InstallEnvironment()
Root: HKLM; SubKey: SYSTEM\CurrentControlSet\Control\Session Manager\Environment; ValueType: string; ValueName: GLPATH; ValueData: "{app}\etc;{app}\lib"; Flags: uninsdeletevalue; Check: IsAdminLoggedOn() or IsPowerUserLoggedOn(); AfterInstall: InstallEnvironment()

[Icons]
Name: {group}\GridLAB-D Console; Filename: {cmd}; WorkingDir: {app}; Comment: Launch GridLAB-D Command Prompt
Name: {group}\Uninstall GridLAB-D; Filename: {uninstallexe}; Comment: Launch GridLAB-D Uninstall Utility
Name: {commondesktop}\GridLAB-D Console; Filename: {cmd}; WorkingDir: {app}; Comment: Launch GridLAB-D Command Prompt; Tasks: desktopicon\common
Name: {userdesktop}\GridLAB-D Console; Filename: {cmd}; WorkingDir: {app}; Comment: Launch GridLAB-D Command Prompt; Tasks: desktopicon\user
Name: {userappdata}\Microsoft\Internet Explorer\Quick Launch\GridLAB-D Console; Filename: {cmd}; WorkingDir: {app}; Comment: Launch GridLAB-D Command Prompt; Tasks: quicklaunchicon

[Code]
const
  WM_SETTINGCHANGE = $1A;
  SMTO_ABORTIFHUNG = $2;

function SendMessageTimeout(hWnd: Integer; Msg: Integer; wParam: LongInt; lParam: String; Flags: Integer; Timeout: Integer; var dwResult: Integer): LongInt;
external 'SendMessageTimeoutA@user32.dll stdcall';

procedure InstallEnvironment();
var
  Value, Path, SubKey: String;
  SendResult, RootKey: Integer;
begin
  If IsAdminLoggedOn() or IsPowerUserLoggedOn() then
  begin
    RootKey := HKLM;
    SubKey := 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';
  end
  else
  begin
    RootKey := HKCU;
    SubKey := 'Environment';
  end;

  Path := ExpandConstant('{app}\bin');
  if RegQueryStringValue(RootKey, SubKey, 'PATH', Value) then
  begin
    if Pos(Uppercase(Path + ';'), Uppercase(Value + ';')) <> 0 then
      exit;
    Value := Value + ';' + Path;
  end
  else
    Value := Path;
  RegWriteStringValue(RootKey, SubKey, 'PATH', Value);
  SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 'Environment', SMTO_ABORTIFHUNG, 5000, SendResult);
  Log('Added ''' + Path + ''' to PATH environment variable.');
end;

procedure UninstallEnvironment();
var
  Value, Path, Temp, SubKey: String;
  Index, Len, SendResult, RootKey: Integer;
begin
  If IsAdminLoggedOn() or IsPowerUserLoggedOn() then
  begin
    RootKey := HKLM;
    SubKey := 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';
  end
  else
  begin
    RootKey := HKCU;
    SubKey := 'Environment';
  end;

  if not RegQueryStringValue(RootKey, SubKey, 'PATH', Value) then
    exit;
  Path := ExpandConstant('{app}\bin');
  Index := Pos(Uppercase(Path + ';'), Uppercase(Value + ';'));
  if Index = 0 then
    exit;
  Len := Length(Path) + 1;
  while Index <> 0 do
  begin
    Temp := Copy(Value, Index + Len, Length(Value));
    Value := Copy(Value, 0, Index - 2);
    if (Value <> '') and (Temp <> '') then
      Value := Value + ';' + Temp
    else
      Value := Value + Temp;
    Index := Pos(Uppercase(Path + ';'), Uppercase(Value + ';'));
  end;
  RegWriteStringValue(RootKey, SubKey, 'PATH', Value);
  SendMessageTimeout(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 'Environment', SMTO_ABORTIFHUNG, 5000, SendResult);
  Log('Removed ''' + Path + ''' from PATH environment variable.');
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if (CurUninstallStep = usPostUninstall) then
    UnInstallEnvironment();
end;

