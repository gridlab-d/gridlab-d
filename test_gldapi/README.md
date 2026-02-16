## Test Harness for GridLabD C++ API


## Building the Test Harness
1. Install GridLabD. Use branch `feature/gldapi`
2. Set environment variable `GridLabD_DIR`. This should point to the location of your GridLabD installation.
3. Create a `build` directory and copy the file `build_script` to it. Edit `build_script` to change the installation location `-DCMAKE_INSTALL_PREFIX=<your_install_location>`
4. cd `build`
5. Run the build_script.
```
./build_script
```
Note: You may have to change the file permission to make it an executable (if not already).
```
chmod +x build_script
```
This will install the executable test_gldapi in the `build` and the `install` directory.

## Execution
Run the test harness executable `test_gldapi`. This will run a bunch of commands to test the GridLabD C++ API.