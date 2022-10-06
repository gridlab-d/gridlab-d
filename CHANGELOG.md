#v5.0 Release Notes

## CMAKE
GridLAB-D is now built using the CMAKE build tools (rather than the previously utilized `autotools`). This eases the build process on all supported platforms.

Resolved issues:

- [#361 Standardize tree for automake platform](https://github.com/gridlab-d/gridlab-d/issues/361)
- [#854 Numerous compiler warning come out of newer version of gcc](https://github.com/gridlab-d/gridlab-d/issues/854)
- [#905 autoconf.ac update to better support mysql](https://github.com/gridlab-d/gridlab-d/issues/905)
- [Fix C/C++ Warnings during build](https://github.com/gridlab-d/gridlab-d/issues/1038)
- [Unable to build target "dist-deb"](https://github.com/gridlab-d/gridlab-d/issues/1226)


## HELICS support
HELICS v3.0 is now fully supported "out-of-the-box" in GridLAB-D. ([HELICS](https://helics.org) is a co-simulation platform that allows runtime-data-synchronization between supported tools.) 

Resolved issues:

- [#715 PowerWorld interface fails under x64 builds](https://github.com/gridlab-d/gridlab-d/issues/715)
- [#879 Autoconf mangling MATLAB configuration on MacOSX](https://github.com/gridlab-d/gridlab-d/issues/879)
- [Support Reiteration from FNCS Co-Simulation](https://github.com/gridlab-d/gridlab-d/issues/1045)
- [Modify helics_msg to use the HELICS 2.0 API](https://github.com/gridlab-d/gridlab-d/issues/1153)
- [Windows installer with HELICS](hhttps://github.com/gridlab-d/gridlab-d/issues/1170)
- [GridLAB-D and NS-3 Co-simulation Support](https://github.com/gridlab-d/gridlab-d/issues/1072)
- [Modify GridLAB-D so it can compile with the version 2.1 of HELICS](https://github.com/gridlab-d/gridlab-d/issues/1179)
- [HELICS-GLD - Enable the ability to swap between QSTS and deltamode](https://github.com/gridlab-d/gridlab-d/issues/1209)
- [Module 'connection' load failed, even if the dll exists](https://github.com/gridlab-d/gridlab-d/issues/1227)
- [Add GridAPP-D message format translation to helics_msg](https://github.com/gridlab-d/gridlab-d/issues/1230)
- [Helics : Cannot return the current time or less than the current time](https://github.com/gridlab-d/gridlab-d/issues/1253)
- [Integrate HELICS 3.0](https://github.com/gridlab-d/gridlab-d/issues/1279)
- [Unit conversion bug with HELICS integration](https://github.com/gridlab-d/gridlab-d/issues/1302)
- [Develop branch helics_msg broken for GridAPPS-D](https://github.com/gridlab-d/gridlab-d/issues/1383)


## MATLAB-link deprecation
With full HELICS support now standard and the connection mechanism between GridLAB-D and MATLAB being unsupported, the use of HELICS is now the supported method to connect GridLAB-D and MATLAB. ([HELICS provides a .mex](https://github.com/GMLC-TDC/matHELICS) to provide MATLAB integration.)

Resolved issues:

- [#639 Matlab link does not respect global access control when importing values](https://github.com/gridlab-d/gridlab-d/issues/639)
- [#659 Suggested Matlab link upgrades](https://github.com/gridlab-d/gridlab-d/issues/659)
- [#909 enable glxmatlab in release](https://github.com/gridlab-d/gridlab-d/issues/909)
- [#984 Enable cmex support for matlab](https://github.com/gridlab-d/gridlab-d/issues/984)
- [MATLAB R2018a does not appear to work with GridLAB-D](https://github.com/gridlab-d/gridlab-d/issues/1168)
- [MATLAB Link: double_array functionality](https://github.com/gridlab-d/gridlab-d/issues/1217)
- [not being able to Link GRIDLAB-D with MATLAB with error object not found](https://github.com/gridlab-d/gridlab-d/issues/1309)
- [MATLAB_link - Add check for semi-colon on "non-needed" properties to help debug](https://github.com/gridlab-d/gridlab-d/issues/1328)
- [Deprecate MATLAB-link support](https://github.com/gridlab-d/gridlab-d/issues/1351)

## Improved wind turbine support

GridLAB-D's modeling of distributed wind turbines has been improved to bring it more on par with the solar PV modeling that already exists.

- [windturb_dg has unit mismatches in it - needs to be checked](https://github.com/gridlab-d/gridlab-d/issues/1039)
- [Distributed Wind Power Curve Update](https://github.com/gridlab-d/gridlab-d/issues/1190)
- [Power Curves and Generic wind turbines](https://github.com/gridlab-d/gridlab-d/issues/1284)
- [Wind Speed Specification for Wind Turbines | TAP-API Integration](https://github.com/gridlab-d/gridlab-d/issues/1330)
- [Wind Turbine-Inverter Coupling | Enabling Smart Inverter Functions for Wind Turbine](https://github.com/gridlab-d/gridlab-d/issues/1364)

## Improved deltamode support
"deltamode" is the transient analysis capability that commonly used in microgrid analysis though its support throughout the GridLAB-D code base is inconsistent and always being worked on.

- [Create deltamode lockout for first QSTS timestep](https://github.com/gridlab-d/gridlab-d/issues/1287)
- [Recorder incorrect or "misaligned" output - CR/LF and missing data - possibley deltamode only](https://github.com/gridlab-d/gridlab-d/issues/1320)
- [Enable schedule/player transforms in deltamode](https://github.com/gridlab-d/gridlab-d/issues/1323)
- [group_recorder with interval - does not update properly in deltamode](https://github.com/gridlab-d/gridlab-d/issues/1354)
- [deltamode generator current (for metering) needs re-examined](https://github.com/gridlab-d/gridlab-d/issues/1366)


## Other new features

Resolved issues:

- [#725 Troubleshooting source link goes to the wrong repository,](https://github.com/gridlab-d/gridlab-d/issues/725)
- [#811 Update Inverter model capabilities,](https://github.com/gridlab-d/gridlab-d/issues/811)
- [#847 Cloud transient model,](https://github.com/gridlab-d/gridlab-d/issues/847)
- [#991 - Inverters only look for triplex_meter or meter objects](https://github.com/gridlab-d/gridlab-d/issues/991)
- [Add External Reliability Event Handler.](https://github.com/gridlab-d/gridlab-d/issues/1162)
- [Implement multi-layer waterheater model](https://github.com/gridlab-d/gridlab-d/issues/1175)
- [Revisit GLD constant current implementation in non-deltamode models](https://github.com/gridlab-d/gridlab-d/issues/1239)
- [Develop basic synch-check object and paralleling controller](https://github.com/gridlab-d/gridlab-d/issues/1240)
- ["Harmonize" the use of status in link-based powerflow objects so it works properly with reliability/configuration](https://github.com/gridlab-d/gridlab-d/issues/1307)
- [Implement fixed-point current-injection powerflow solver](https://github.com/gridlab-d/gridlab-d/issues/1319)
- [NR powerflow - grandchild and "greater generations" support](https://github.com/gridlab-d/gridlab-d/issues/1358)

## General bug fixes

Resolved issues:

- [#733 --example residential:dishwasher crashes](https://github.com/gridlab-d/gridlab-d/issues/733)
- [#907 interval < -1 causes recorder to hang](https://github.com/gridlab-d/gridlab-d/issues/907)
- [High Sierra build error](https://github.com/gridlab-d/gridlab-d/issues/1112)
- [Mac build fails validation on Sierra](https://github.com/gridlab-d/gridlab-d/issues/1034)
- [Unusual behaviour with latest GridLAB-D installation](https://github.com/gridlab-d/gridlab-d/issues/1043)
- [Autotest err and exc tests return pass when test model passes](https://github.com/gridlab-d/gridlab-d/issues/1104)
- [Add ability to determine "SWING_PQ bus state" without a debugger](https://github.com/gridlab-d/gridlab-d/issues/1277)
- [Metrics collector writer uses strcat calls that may have copy overlaps](https://github.com/gridlab-d/gridlab-d/issues/1294)
- [Incorrect compare tests in gldcore/compare.c](https://github.com/gridlab-d/gridlab-d/issues/1297)
- [Adjust ZIP-fraction capability in powerflow/load.cpp to include delta-connected loads](https://github.com/gridlab-d/gridlab-d/issues/1305)
- [Tape player sscanf typecast for raw timestamps is incorrect](https://github.com/gridlab-d/gridlab-d/issues/1311)
- [inverter_dyn 1547 checks do not work for childed powerflow objects](https://github.com/gridlab-d/gridlab-d/issues/1318)
- [Sqrt(3) Errors with a delta-connected load's current_fraction and impedance_fraction](https://github.com/gridlab-d/gridlab-d/issues/1312)
- [Insolation calculation for DEFAULT orientation in solar is incorrect](https://github.com/gridlab-d/gridlab-d/issues/1333)
- [Typo in house_e source code for cooling COP units](https://github.com/gridlab-d/gridlab-d/issues/1337)
- [inverter_dyn connected to a childed triplex device is failing to work](https://github.com/gridlab-d/gridlab-d/issues/1352)
- [Transformer in-rush (especially saturation) may not work properly anymore](https://github.com/gridlab-d/gridlab-d/issues/1369)
- [Update/verify global delta_current_clock so that it is updating in QSTS as well](https://github.com/gridlab-d/gridlab-d/issues/1370)
- [Rfloor values incorrect for thermal integrity of NORMAL and ABOVE_NORMAL](https://github.com/gridlab-d/gridlab-d/issues/1376)

## Misc

Resolved issues:

- [#74 Doxygen warnings need to be fixed](https://github.com/gridlab-d/gridlab-d/issues/74)
- [#110 JAVA_HOME](https://github.com/gridlab-d/gridlab-d/issues/110)
- [#454 Locking does not work in win32 when using runtime classes](https://github.com/gridlab-d/gridlab-d/issues/454)
- [#822 Merge misc small fixes from ticket 697 to 3.1](https://github.com/gridlab-d/gridlab-d/issues/822)
- [#856 Additional Specifications for Solar Panels](https://github.com/gridlab-d/gridlab-d/issues/856)
- - [#876 SVN does not ignore build related files](https://github.com/gridlab-d/gridlab-d/issues/876)
- [#962 Support Mac OS Line Endings](https://github.com/gridlab-d/gridlab-d/issues/962)
- [Update phase checks and current calculation approach for NR-based powerflow](https://github.com/gridlab-d/gridlab-d/issues/1052)
- [RC1 installer is automatically adding a GNU plot item to the path](https://github.com/gridlab-d/gridlab-d/issues/1088)
- [Development branch for general performance and testing](https://github.com/gridlab-d/gridlab-d/issues/1091)
- [property base_power of ZIPload:22 could not be set to 'LIGHTS*1.8752'](https://github.com/gridlab-d/gridlab-d/issues/1163)
- [Cannot figure out how to run my glm files](https://github.com/gridlab-d/gridlab-d/issues/1188)
- [HOW TO SOLVE THIS install wrong](https://github.com/gridlab-d/gridlab-d/issues/1189)
- [Version 5.0 - Remove DEPRECATED properties prior to RC](https://github.com/gridlab-d/gridlab-d/issues/1288)
- [issue in using server mode of gridlabd](https://github.com/gridlab-d/gridlab-d/issues/1317)
- [Failure to read player files when different time zone are specified in the .glm and player files](https://github.com/gridlab-d/gridlab-d/issues/1332)
- [Fixed-point current-injection solver: Updated fault current calculations](https://github.com/gridlab-d/gridlab-d/issues/1348)
- [Evaluate if Xerces support still needed - remove if not](https://github.com/gridlab-d/gridlab-d/issues/1367)
- [Update COPYRIGHT and LICENSE for build output](https://github.com/gridlab-d/gridlab-d/issues/1367)