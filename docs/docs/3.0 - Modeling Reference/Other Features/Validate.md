# Validate

The validate command line option initiates the standard model validation process for GridLAB-D™. The process functions as follows: 

  1. Recursively scan the working directory for directories called `autotest`, ignoring any directory that contains a file named `validate.no`.
  2. For each GLM files found in an `autotest` directory: 
     1. Create a folder by the same name (or purge existing folder if the `clean` global variable is defined)
     2. Copy the GLM file into the folder
     3. Run `gridlabd` on the GLM file with all the `run-options`
     4. Time and observe results (success, fail, exception)
  3. Report total time and results

Example: 

    host% gridlabd test-options --validate run-options

The `test-options` are applied to the validation instance of GridLAB-D™, while the `run-options` are applied to the test instance of GridLAB-D™. For example, 
    
    
    host% gridlabd --threadcount 2 --validate 
    

will run two validation tests simultaneously, while 
    
    
    host% gridlabd --validate --threadcount 2
    

will run a single two-threaded validation test at a time. 

### Output

Validation test failures are reported as errors. Optional test failures are reported as warning. 

By default all output is redirected to the default output streams (see redirect**TODO** - Redirect - pull in def for redirect). You can send all output to the console using `--redirect none command option`. 

### Options

The validate procedure is controlled using the following **global variables**. 

`--define|-D clean=1`: 
    If the `clean` variable is defined, the directories used to run each test are purged before the test is started.

`--define|-D validate=NONE|TRUN|TERR|TEXC|TOPT|TSTD|TALL|RDIR|RGLD|RALL`:
    Controls which validation tests are performed. 

* **NONE** -
    None of the tests are performed (provided to definition purposes only)
* **TRUN** -
    Includes tests that are expected to succeed.
* **TERR** -
    Includes tests that are expected to cause errors.
* **TEXC** -
    Includes tests that are expected to cause exceptions.
* **TOPT** -
    Includes tests that are optional
* **TSTD** -
    Includes all standard tests (`TRUN|TERR|TEXC`)
* **TALL** -
    Includes all tests (`TRUN|TERRT|EXC|TOPT`)
* **RDIR** -
    Includes directory scan results in output report
* **RGLM** -
    Includes GLM file results in output report
* **RALL** -
    Includes both directory and file results in output report

`--define|-D force_validate=1`:
Overrides the effect of the `validate.no` file if found in a folder. Normally, validate does not process folders that contain the file `validate.no`.


### Report Format


When a validation run is performed only progress and unexpected results are normally displayed on screen. However, a detailed validation report is output to the file `validate.csv`. You may change the name and type of the validation report file by setting the validate_report global variable. For example, 

    
    host% **gridlabd -D validate_report=validate.txt --validate**
    

will output a (tab-delimited) TXT file instead of a CSV file. 

The validation report is formatted to facilitate parsing by scripts and includes the following sections. 


* **TEST CONFIGURATION** -
    This section provide general information about the gridlabd environment configuration used to perform the validation test.
* **DIRECTORY SCAN RESULTS** -
    This section lists how many files were found in each directory scanned. This section is only output if the **RDIR** or **RALL** validate (global) options are set.
* **FILE TEST RESULTS** -
    This section lists the results of each test and is only output if the **RGLM** or **RALL** validate (global) options are set. In the first column `S` indicates the test unexpectedly succeeded, `E` indicates the test unexpectedly produced an error, and `X` indicates the test unexpectedly produces an exception. The second column provides the runtime of the test. The third column provides the name of the GLM file tested.
* **OVERALL RESULTS** -
    The section summarizes the overall results of the validation test. Any unusual results are flagged with a triple exclamation point (**!!!**) in the first column. The result code is a unique number that is generated that can be compared to validation test results from other platforms. A result code having all zeros represents complete success. Non-zero values correspond to various failure modes on specific files.

### Mailing Reports


Linux/Mac only
    
    When a validation run is completed, a copy of the validation report can be emailed using the mailto global variable. For example

    host% **gridlabd -D mailto _user_ @localhost --validate**

Will email the validation report in to _user_ on the local machine. 

!!! note

    Your local mail server must be configured properly to deliver email to remote hosts and your mail client must be configured properly to read mail on the local mail server. GridLAB-D™ cannot detect either mail server or mail client configuration errors. Thus problems with either of these can cause mailed report delivery errors to occur with an error being reported in GridLAB-D™.

## Caveats

Multithreaded operation can cause intermingled output, particularly when used in conjunction with --redirect none. This is due to the lack of locking in the output message streams when running multiple jobs on a single console. 

Some output from gridlabd runs cannot be redirected and will always be displayed on the console. Known problems include output from compilers, linked applications, and scripts. 

## Bugs

Unhandled exceptions in Windows can cause modal dialogs to pop up that block the process. This behavior appears to be impossible to suppress. See [ticket 606](http://sourceforge.net/p/gridlab-d/tickets/606) for details. 


# Test

The user may provide a command option `--test` to enable the various test routines supported in GridLAB-D. The routines are made available to users by listing the core test routines in the `* **test_list` variable and/or exporting the `module_test` routine from modules the support self-tests.


Core (in `core/test.c`)

        static TESTLIST * **test_list[] = {
        // ...
        {"name", component_test, 0, next_ptr}
        };

Component (in `core/_component_.c`)

        int component_test(void)
        {
        // ...
        return SUCCESS; // or FAILED
        }

Module (in `_module_ /test.cpp`)

        EXPORT int module_test (int argc, char *argv[])
        {
        // ...
        return SUCCESS; // or FAILED
        }

# Test: Core

!!! note

    TECHNICAL MANUAL REVIEW NEEDED


The following tests are current performed on the GridLAB-D.

* **test_bare_class** - Create a new runtime class with a single property, initializes the property, and verify that the property is set correctly.

* **test_c_include** -  Verify the "C" include functionality of runtime class compilation.

* **test_core_4blocks_schedule** -  Verify that 96 unique values schedule split into 4 blocks place the correct values into properties of an object at the correct time.

* **test_core_5blocks_schedule_err** -  Verify that a schedule with more than 4 blocks cannot be compiled.

* **test_core_63_schedule** -  Verify that a schedule with 63 entries can be compiled.

* **test_core_64_schedule_err** -  Verify that a schedule with 64 entries cannot be compiled.

* **test_core_player_schedule_1** -  Verify that a player with no parent can serve as a schedule and work with transforms.

* **test_core_schedules_boolean_err** -  Verify that the boolean flag detects a schedule with a non-boolean values.

* **test_core_schedules_nonzero_err** -  Verify that the nonzero flag detects a schedule with a zero value.

* **test_core_schedules_positive_err** -  Verify that the positive flag detects a schedule with a zero or negative values.

* **test_transient mode** -  Verify basic transient mode operation.

* **test_double_array** -  Verify basic double_array operations in a runtime class.

* **test_duplication_function_err Verify that export functions from a runtime class cannot be given duplicate names.

* **test_exec_mainloop** - Verify that the main loop stoptime works properly.

* **test_external_null_source** - Verify that non-existent external transform sources work properly.

* **test_filter_delay** - Verify basic delay filter functionality.

* **test_filter_second** - Verify second-order filter functionality.

* **test_global_unit_convert** - Verify unit conversion on a global variable.

* **test_global_var_expansion** - Verify inline expansion variables and operation.

* **test_groupid** - Verify groupid implementation.

* **test_guid** - Verify globally unique id implementation.

* **test_inline_plc** - Verify inline PLC code ( TODO: : does not appear to do that.)

* **test_kml_output** - Verify kml output ( TODO: : does not appear to do that.)

* **test_latlon** - Verify all the allowed formats of latitude and longitude.

* **test_loadshape_exercise_2_3_3** - Verify end-use loadshapes.

* **test_locale** - Verify the use of locale names instead of timezone specifications ( TODO: : only checks syntax, does not check proper functionality).

* **test_notz** - Verify that omitted timezone does not cause a loader problem when datetime is used.

* **test_now** - Verify the use of the NOW variable.

* **test_opt_alternate_syntax** - Verify the use of alternate value loader syntax.

* **test_opt_commit** - Verify the use of intrinsic commit to influence the advance of global clock.

* **test_opt_core_pc_and_fn** - Verify general runtime class compilation.

* **test_opt_core_runtime_class** - Verify general runtime class compilation.

* **test_opt_runtime_exercise_3_1_1** - Verify basic runtime class compilation.

* **test_parameter_expansion** - Verify general example variables functionality.

* **test_quoted_value** - Verify quoted string concatenation by loader.

* **test_run** - Verify RUN variable implementation.

* **test_schedule_types** - Verify basic schedule flags functionality.

* **test_schedule_xform** - Verify transform implementation.

* **test_schedule_xform_external** - Verify external transform implementation.

* **test_script** - Verify script success detection.

* **test_script_err** - Verify script failure detection.

* **test_script_event** - Verify script event calls.

* **test_seq** - Verify sequence numbering and usage.

* **test_simple_schedule** - Verify a simple schedule implementation.

* **test_statefull_randomization** - Verify implementation of RNG3 stateful random number streams.

* **test_stream_out** - Verify stream output.

* **test_struct** - Verify struct parser.


## Related Concepts:

  * Command options
  * Global variables



  
