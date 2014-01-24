[Setup]
Compression=lzma
OutputDir=.

; ++RELEASE++
SourceDir=.\x64\Release
OutputBaseFilename=gridlabd-3.0-x64
AppName=GridLAB-D
AppVerName=GridLAB-D 3.0
; --RELEASE--

; ++DEBUG++
;SourceDir=.\Win32\Debug
;OutputBaseFilename=gridlabd-debug-0.1
;AppName=GridLAB-D Debug
;AppVerName=GridLAB-D 0.1 Debug
; --DEBUG--

AppVersion=3.0
AppPublisher=Pacific Northwest National Laboratory, operated by Battelle
AppCopyright=Copyright © 2004-2013 Battelle Memorial Institute
AppPublisherURL=http://www.pnl.gov
;AppReadmeFile={app}\README.TXT
;AppSupportURL=http://gridlab.pnl.gov/support
;AppUpdatesURL=http://gridlab.pnl.gov//downloads
;AppComments=<Include application comments here>
;AppContact=GridLAB-D Development Team <gridlabd@pnl.gov>
VersionInfoDescription=Gridlab-D - Grid Simulator
VersionInfoVersion=3.0.0.0

;AppMutex=<Mutex string to prevent installation while application is running>
DefaultDirName={pf}\GridLAB-D
DefaultGroupName=GridLAB-D
PrivilegesRequired=admin
AllowNoIcons=yes
;UninstallDisplayIcon={app}\bin\gridlabd.exe
ArchitecturesInstallIn64BitMode=x64

[Types]
Name: typical; Description: Typical Installation
Name: custom; Description: Custom Installation; Flags: iscustom

[Components]
Name: "core"; Description: "Core Files"; Types: typical custom; Flags: fixed
Name: "modules"; Description: "Modules"; Types: typical custom
Name: "modules\climate"; Description: "Climate Module"; Types: typical custom
Name: "modules\generators"; Description: "Generators Module"; Types: typical custom
Name: "modules\market"; Description: "Market Module"; Types: typical custom
;Name: "modules\mysql"; Description: "MYSQL Module"; Types: typical custom
Name: "modules\network"; Description: "Network Module"; Types: typical custom
Name: "modules\powerflow"; Description: "Power Flow Module"; Types: typical custom
Name: "modules\reliability"; Description: "Reliability Module"; Types: typical custom
Name: "modules\residential"; Description: "Residential Module"; Types: typical custom
Name: "modules\tape"; Description: "Tape Module"; Types: typical custom
Name: "samples"; Description: "Sample Models"; Types: typical custom
Name: "Compilers"; Description: "Download and install MinGW"
Name: "Plotting_Tools"; Description: "Download and install GNUPlot"
Name: "Climate_Data"; Description: "Download climate data files"
Name: "Climate_Data\US"; Description: "US climate data"
Name: "Climate_Data\US\North"; Description: "Northern US climate data"
Name: "Climate_Data\US\North\CN"; Description: "Conneticut climate data"
Name: "Climate_Data\US\North\IA"; Description: "Iowa climate data"
Name: "Climate_Data\US\North\ID"; Description: "Idaho climate data"
Name: "Climate_Data\US\North\IL"; Description: "Illinois climate data"
Name: "Climate_Data\US\North\IN"; Description: "Indiana climate data"
Name: "Climate_Data\US\North\MA"; Description: "Massachusetts climate data"
Name: "Climate_Data\US\North\ME"; Description: "Maine climate data"
Name: "Climate_Data\US\North\MI"; Description: "Michigan climate data"
Name: "Climate_Data\US\North\MN"; Description: "Minnesota climate data"
Name: "Climate_Data\US\North\MT"; Description: "Montana climate data"
Name: "Climate_Data\US\North\ND"; Description: "North Dakota climate data"
Name: "Climate_Data\US\North\NE"; Description: "Nebraska climate data"
Name: "Climate_Data\US\North\NH"; Description: "New Hampshire climate data"
Name: "Climate_Data\US\North\NJ"; Description: "New Jersey climate data"
Name: "Climate_Data\US\North\NY"; Description: "New York climate data"
Name: "Climate_Data\US\North\OH"; Description: "Ohio climate data"
Name: "Climate_Data\US\North\PA"; Description: "Pennsylvania climate data"
Name: "Climate_Data\US\North\RI"; Description: "Rhode Island climate data"
Name: "Climate_Data\US\North\SD"; Description: "South Dakota climate data"
Name: "Climate_Data\US\North\VT"; Description: "Vermont climate data"
Name: "Climate_Data\US\North\WI"; Description: "Wisconsin climate data"
Name: "Climate_Data\US\North\WY"; Description: "Wyoming climate data"
Name: "Climate_Data\US\Central"; Description: "Central US climate data"
Name: "Climate_Data\US\Central\AR"; Description: "Arkansas climate data"
Name: "Climate_Data\US\Central\DE"; Description: "Delaware climate data"
Name: "Climate_Data\US\Central\KS"; Description: "Kansas climate data"
Name: "Climate_Data\US\Central\KY"; Description: "Kentucky climate data"
Name: "Climate_Data\US\Central\MD"; Description: "Maryland climate data"
Name: "Climate_Data\US\Central\MO"; Description: "Missouri climate data"
Name: "Climate_Data\US\Central\NC"; Description: "North Carolina climate data"
Name: "Climate_Data\US\Central\OK"; Description: "Oklahoma climate data"
Name: "Climate_Data\US\Central\SC"; Description: "South Carolina climate data"
Name: "Climate_Data\US\Central\TN"; Description: "Tennessee climate data"
Name: "Climate_Data\US\Central\VA"; Description: "Virginia climate data"
Name: "Climate_Data\US\Central\WV"; Description: "West Virginia climate data"
Name: "Climate_Data\US\South"; Description: "Southern US climate data"
Name: "Climate_Data\US\South\AL"; Description: "Alabama climate data"
Name: "Climate_Data\US\South\FL"; Description: "Florida climate data"
Name: "Climate_Data\US\South\GA"; Description: "Georgia climate data"
Name: "Climate_Data\US\South\LA"; Description: "Louisiana climate data"
Name: "Climate_Data\US\South\MS"; Description: "Mississippi climate data"
Name: "Climate_Data\US\South\TX"; Description: "Texas climate data"
Name: "Climate_Data\US\Southwest"; Description: "Southwestern US climate data"
Name: "Climate_Data\US\Southwest\AZ"; Description: "Arizona climate data"
Name: "Climate_Data\US\Southwest\CO"; Description: "Colorado climate data"
Name: "Climate_Data\US\Southwest\NM"; Description: "New Mexico climate data"
Name: "Climate_Data\US\Southwest\NV"; Description: "Nevada climate data"
Name: "Climate_Data\US\Southwest\UT"; Description: "Utah climate data"
Name: "Climate_Data\US\West"; Description: "Western US climate data Region"
Name: "Climate_Data\US\West\CA"; Description: "California climate data"
Name: "Climate_Data\US\West\OR"; Description: "Oregon climate data"
Name: "Climate_Data\US\West\WA"; Description: "Washington climate data"
Name: "Climate_Data\US\Other"; Description: "Non-Continental US climate data"
Name: "Climate_Data\US\Other\AK"; Description: "Alaska climate data"
Name: "Climate_Data\US\Other\HI"; Description: "Hawaii climate data"

[Tasks]
Name: environment; Description: Add GridLAB-D to &PATH environment variable; GroupDescription: Environment
Name: overwriteglpath; Description: Create GLPATH, overwrite if exists (recommended); GroupDescription: Environment; Components: 
Name: desktopicon; Description: Create a &desktop icon; GroupDescription: Additional icons:
Name: desktopicon\common; Description: For all users; GroupDescription: Additional icons:; Flags: exclusive
Name: desktopicon\user; Description: For the current user only; GroupDescription: Additional icons:; Flags: exclusive unchecked
Name: quicklaunchicon; Description: Create a &Quick Launch icon; GroupDescription: Additional icons:; Flags: unchecked

[Dirs]
Name: {app}\bin
Name: {app}\etc
Name: {app}\tmy
Name: {app}\lib
Name: {userdocs}\GridLAB-D
Name: {app}\rt

[Files]
;; Microsoft CRT redist
Source: "Microsoft.VC80.CRT.manifest"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: core
Source: "msvcp80.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: core
Source: "msvcr80.dll"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: core
Source: "msvcr80.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: core
Source: "msvcp80.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: core
Source: "Microsoft.VC80.CRT.manifest"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: core

;; core files
Source: "assert.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: core
Source: "gridlabd.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: core
Source: "glsolvers.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: core
Source: "glxmatlab.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: core
Source: "pthreadVC2.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: core
Source: "test_extern_function.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: core
Source: "tzinfo.txt"; DestDir: "{app}\etc"; Components: core
Source: "unitfile.txt"; DestDir: "{app}\etc"; Components: core
Source: "xerces-c_3_1.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: core
Source: "climate.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: modules\climate
Source: "generators.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: modules\generators
Source: "market.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: modules\market
;Source: "mysql.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: modules\mysql
Source: "network.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: modules\network
Source: "powerflow.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: modules\powerflow
Source: "reliability.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: modules\reliability
Source: "residential.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: modules\residential
Source: "tape.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: modules\tape
Source: "tape_file.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: modules\tape
Source: "tape_memory.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: modules\tape
Source: "tape_plot.dll"; DestDir: "{app}\lib"; Flags: ignoreversion; Components: modules\tape

;; sample files
Source: "..\..\..\models\climate_csvreader_example.glm"; DestDir: "{app}\samples"
Source: "..\..\..\models\collector_example.glm"; DestDir: "{app}\samples"
Source: "..\..\..\models\diesel_deltamode_load_player_A.csv"; DestDir: "{app}\samples"
Source: "..\..\..\models\diesel_deltamode_load_player_B.csv"; DestDir: "{app}\samples"
Source: "..\..\..\models\diesel_deltamode_load_player_C.csv"; DestDir: "{app}\samples"
Source: "..\..\..\models\Four_quadrant_inverter_example.glm"; DestDir: "{app}\samples"
Source: "..\..\..\models\house_HVAC_example.glm"; DestDir: "{app}\samples"
Source: "..\..\..\models\IEEE_13_Node_Test_Feeder.glm"; DestDir: "{app}\samples"
Source: "..\..\..\models\IEEE_13_Node_With_Houses.glm"; DestDir: "{app}\samples"
Source: "..\..\..\models\passive_controller_example.glm"; DestDir: "{app}\samples"
Source: "..\..\..\models\random_fault_generator_example.glm"; DestDir: "{app}\samples"
Source: "..\..\..\models\residential_zipload_example.glm"; DestDir: "{app}\samples"
Source: "..\..\..\models\Standard_Weather.csv"; DestDir: "{app}\samples"
Source: "..\..\..\models\subsecond_diesel_generator_example.glm"; DestDir: "{app}\samples"
Source: "..\..\..\models\taxonomy_feeder_R1-12.47-1.glm"; DestDir: "{app}\samples"
Source: "..\..\..\models\transactive_controller_example.glm"; DestDir: "{app}\samples"
Source: "..\..\..\models\WA-Seattle.tmy2"; DestDir: "{app}\samples"
Source: "..\..\..\models\WA-Yakima.tmy2"; DestDir: "{app}\samples"
Source: "..\..\..\models\waterheater_example.glm"; DestDir: "{app}\samples"
Source: "..\..\..\models\weather.csv"; DestDir: "{app}\samples"
Source: "..\..\..\models\wind_turbine_example.glm"; DestDir: "{app}\samples"

;; other files
Source: "..\..\..\core\rt\gridlabd.syn"; DestDir: "{code:TextPadDestination}\System"; Flags: ignoreversion; Check: GetTextPadLocation
Source: "..\..\..\README-WINDOWS.txt"; DestDir: "{app}"
Source: "..\..\..\core\rt\mingw.conf"; DestDir: "{app}\rt"
Source: "..\..\..\core\rt\gridlabd.conf"; DestDir: "{app}\rt"; Flags: ignoreversion
Source: "..\..\..\core\rt\debugger.conf"; DestDir: "{app}\rt"; Flags: ignoreversion
Source: "..\..\..\core\rt\gnuplot.conf"; DestDir: "{app}\rt"; Flags: ignoreversion
Source: "..\..\..\core\rt\gridlabd.h"; DestDir: "{app}\rt"; Flags: ignoreversion
Source: "..\..\..\utilities\wget.exe"; DestDir: "{app}"; Flags: deleteafterinstall
Source: "..\..\..\utilities\7za.exe"; DestDir: "{app}"
Source: "..\..\..\core\gridlabd.htm"; DestDir: "{app}"

; ++ Removed Files ++
;Source: commercial.dll; DestDir: {app}\lib; Flags: ignoreversion; Components: modules\legacy\commercial
;Source: climateview.dll; DestDir: {app}\lib; Flags: ignoreversion
;Source: network.dll; DestDir: {app}\lib; Flags: ignoreversion; Components: modules\legacy\network
;Source: plc.dll; DestDir: {app}\lib; Flags: ignoreversion; Components: modules\legacy\plc
;Source: tape_odbc.dll; DestDir: {app}\lib; Flags: ignoreversion
;Source: glmjava.dll; DestDir: {app}\lib; Flags: ignoreversion
;Source: network.dll; DestDir: {app}\lib; Flags: ignoreversion
;Source: sample.dll; DestDir: {app}\lib; Flags: ignoreversion
;Source: cppunit.dll; DestDir: {app}\bin; Flags: ignoreversion; Components: core

;Source: ..\..\..\models\powerflow_IEEE_4node.glm; DestDir: {app}\samples; Components: samples
;Source: ..\..\..\models\residential_loads.glm; DestDir: {app}\samples
;Source: ..\..\..\models\powerflow_IEEE_37node.glm; DestDir: {app}\samples
;Source: ..\..\..\models\lighting.player; DestDir: {app}\samples
;Source: ..\..\..\models\dryer.shape; DestDir: {app}\samples
;Source: ..\..\..\climate\tmy\*.zip; DestDir: {app}\tmy
;Source: ..\..\..\climate\tmy\extract_tmy; DestDir: {app}\tmy
;Source: ..\..\..\climate\tmy\build_pkgs; DestDir: {app}\tmy
;Source: ..\..\..\plc\rt\include\plc.h; DestDir: {app}\rt

[Registry]
Root: HKCU; SubKey: Environment; ValueType: string; ValueName: GLPATH; ValueData: "{app}\bin;{app}\etc;{app}\lib;{app}\samples;{app}\rt;{app}\tmy"; Flags: uninsdeletevalue deletevalue; Check: not (IsAdminLoggedOn() or IsPowerUserLoggedOn()); Tasks: overwriteglpath
Root: HKLM; SubKey: SYSTEM\CurrentControlSet\Control\Session Manager\Environment; ValueType: string; ValueName: GLPATH; ValueData: "{app}\bin;{app}\etc;{app}\lib;{app}\samples;{app}\rt;{app}\tmy"; Flags: uninsdeletevalue deletevalue; Check: IsAdminLoggedOn() or IsPowerUserLoggedOn(); Tasks: overwriteglpath
Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\Session Manager\Environment; ValueType: expandsz; ValueName: GRIDLABD; ValueData: {app}; Flags: uninsdeletevalue deletevalue
;Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\Session Manager\Environment; ValueType: expandsz; ValueName: PATH; ValueData: "%PATH%;{app}\bin"
;Root: HKLM; Subkey: SYSTEM\CurrentControlSet\Control\Session Manager\Environment; ValueType: expandsz; ValueName: PATH; ValueData: "%PATH%;c:\GnuPlot\bin"
Root: HKLM; SubKey: SYSTEM\CurrentControlSet\Control\Session Manager\Environment; ValueType: string; ValueName: GLENVSET; ValueData: true; Flags: uninsdeletevalue deletevalue; Check: IsAdminLoggedOn() or IsPowerUserLoggedOn(); AfterInstall: InstallEnvironment(); Tasks: environment

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

  Path := ExpandConstant('{app}\lib');
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

  Path := 'C:\GnuPlot\bin';
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

  Path := ExpandConstant('{app}\lib');
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

  Path := 'C:\GnuPlot\bin';
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

// Grabbed and modified from stackoverflow
// http://stackoverflow.com/questions/121812/how-do-i-use-inno-setup-to-optionally-install-a-plugin-file-in-a-folder-based-on
var sTextPadDest : String;

//
// Search for the path where TextPad was installed.  Return true if path found.
// Set variable to plugin folder
//

function GetTextPadLocation(): Boolean;
var
  i:      Integer;
  len:    Integer;

begin
  sTextPadDest := '';

  RegQueryStringValue( HKLM, 'SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\TextPad.exe', 'Path', sTextPadDest );
  len := Length(sTextPadDest);

  Result := len > 0;
end;

//
//  Use this function to return path to install plugin
//
function TextPadDestination(Param: String) : String;
begin
   Result := sTextPadDest;
end;
[Run]
Filename: {app}\wget.exe; Parameters: http://downloads.sourceforge.net/sourceforge/gridlab-d/climate-US-2_0.zip?use_mirror=superb-east; WorkingDir: {app}\tmy; Components: Climate_Data\US
Filename: {app}\wget.exe; Parameters: http://downloads.sourceforge.net/sourceforge/gridlab-d/MinGW-5.1.4.exe?use_mirror=superb-west; WorkingDir: {app}; Components: Compilers
Filename: {app}\wget.exe; Parameters: http://downloads.sourceforge.net/sourceforge/gridlab-d/gnuplot-win32-4_2_3.zip?use_mirror=superb-west; WorkingDir: {app}; Components: Plotting_Tools
Filename: {app}\MinGW-5.1.4.exe; WorkingDir: {app}; Components: Compilers
Filename: {app}\7za.exe; Parameters: x gnuplot-win32-4_2_3.zip; WorkingDir: c:\; Components: Climate_Data\US; Tasks: 
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip AK.zip; WorkingDir: {app}\tmy; Components: Climate_Data\US\Other\AK; Tasks: 
Filename: {app}\7za.exe; Parameters: e AK.zip; WorkingDir: {app}\tmy; Components: Climate_Data\US\Other\AK; Tasks: 
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip PA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\PA
Filename: {app}\7za.exe; Parameters: e PA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\PA
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip CN.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\CN
Filename: {app}\7za.exe; Parameters: e CN.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\CN
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip IA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\IA
Filename: {app}\7za.exe; Parameters: e IA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\IA
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip ID.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\ID
Filename: {app}\7za.exe; Parameters: e ID.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\ID
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip IL.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\IL
Filename: {app}\7za.exe; Parameters: e IL.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\IL
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip IN.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\IN
Filename: {app}\7za.exe; Parameters: e IN.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\IN
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip MA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\MA
Filename: {app}\7za.exe; Parameters: e MA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\MA
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip ME.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\ME
Filename: {app}\7za.exe; Parameters: e ME.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\ME
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip MI.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\MI
Filename: {app}\7za.exe; Parameters: e MI.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\MI
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip MN.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\MN
Filename: {app}\7za.exe; Parameters: e MN.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\MN
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip MT.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\MT
Filename: {app}\7za.exe; Parameters: e MT.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\MT
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip ND.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\ND
Filename: {app}\7za.exe; Parameters: e ND.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\ND
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip NE.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\NE
Filename: {app}\7za.exe; Parameters: e NE.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\NE
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip NH.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\NH
Filename: {app}\7za.exe; Parameters: e NH.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\NH
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip NJ.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\NJ
Filename: {app}\7za.exe; Parameters: e NJ.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\NJ
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip NY.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\NY
Filename: {app}\7za.exe; Parameters: e NY.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\NY
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip OH.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\OH
Filename: {app}\7za.exe; Parameters: e OH.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\OH
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip PA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\PA
Filename: {app}\7za.exe; Parameters: e PA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\PA
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip RI.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\RI
Filename: {app}\7za.exe; Parameters: e RI.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\RI
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip SD.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\SD
Filename: {app}\7za.exe; Parameters: e SD.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\SD
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip VT.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\VT
Filename: {app}\7za.exe; Parameters: e VT.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\VT
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip WI.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\WI
Filename: {app}\7za.exe; Parameters: e WI.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\WI
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip WY.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\WY
Filename: {app}\7za.exe; Parameters: e WY.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\North\WY
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip AR.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\AR
Filename: {app}\7za.exe; Parameters: e AR.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\AR
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip DE.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\DE
Filename: {app}\7za.exe; Parameters: e DE.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\DE
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip KS.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\KS
Filename: {app}\7za.exe; Parameters: e KS.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\KS
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip KY.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\KY
Filename: {app}\7za.exe; Parameters: e KY.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\KY
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip MD.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\MD
Filename: {app}\7za.exe; Parameters: e MD.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\MD
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip MO.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\MO
Filename: {app}\7za.exe; Parameters: e MO.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\MO
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip NC.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\NC
Filename: {app}\7za.exe; Parameters: e NC.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\NC
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip OK.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\OK
Filename: {app}\7za.exe; Parameters: e OK.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\OK
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip SC.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\SC
Filename: {app}\7za.exe; Parameters: e SC.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\SC
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip TN.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\TN
Filename: {app}\7za.exe; Parameters: e TN.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\TN
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip VA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\VA
Filename: {app}\7za.exe; Parameters: e VA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\VA
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip WV.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\WV
Filename: {app}\7za.exe; Parameters: e WV.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Central\WV
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip AL.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\South\AL
Filename: {app}\7za.exe; Parameters: e AL.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\South\AL
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip FL.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\South\FL
Filename: {app}\7za.exe; Parameters: e FL.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\South\FL
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip GA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\South\GA
Filename: {app}\7za.exe; Parameters: e GA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\South\GA
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip LA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\South\LA
Filename: {app}\7za.exe; Parameters: e LA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\South\LA
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip MS.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\South\MS
Filename: {app}\7za.exe; Parameters: e MS.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\South\MS
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip TX.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\South\TX
Filename: {app}\7za.exe; Parameters: e TX.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\South\TX
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip AZ.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Southwest\AZ
Filename: {app}\7za.exe; Parameters: e AZ.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Southwest\AZ
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip CO.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Southwest\CO
Filename: {app}\7za.exe; Parameters: e CO.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Southwest\CO
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip NM.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Southwest\NM
Filename: {app}\7za.exe; Parameters: e NM.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Southwest\NM
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip NV.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Southwest\NV
Filename: {app}\7za.exe; Parameters: e NV.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Southwest\NV
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip UT.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Southwest\UT
Filename: {app}\7za.exe; Parameters: e UT.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Southwest\UT
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip CA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\West\CA
Filename: {app}\7za.exe; Parameters: e CA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\West\CA
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip OR.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\West\OR
Filename: {app}\7za.exe; Parameters: e OR.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\West\OR
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip WA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\West\WA
Filename: {app}\7za.exe; Parameters: e WA.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\West\WA
Filename: {app}\7za.exe; Parameters: e climate-US-2_0.zip HI.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Other\HI
Filename: {app}\7za.exe; Parameters: e HI.zip; WorkingDir: {app}\tmy; Tasks: ; Components: Climate_Data\US\Other\HI
[UninstallDelete]
Name: {app}\tmy\climate-US-2_0.zip; Type: files