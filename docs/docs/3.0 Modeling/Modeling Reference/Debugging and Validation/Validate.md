# Validate

Command line option to run model validation test suite [Template:NEW30]

## Synopsis
    
    
    host% **gridlabd _test-options_ --validate _run-options_**
    

## Description

The validate command line option initiates the standard model validation process for GridLAB-D. The process functions as follows: 

  1. Recursively scan the [working directory] for directories called `autotest`, ignoring any directory that contains a file named `validate.no`.
  2. For each GLM files found in an `autotest` directory: 
     1. Create a folder by the same name (or purge existing folder if the `clean` global variable is defined)
     2. Copy the GLM file into the folder
     3. Run `gridlabd` on the GLM file with all the _run-options_
     4. Time and observe results (success, fail, exception)
  3. Report total time and results
The _test-options_ are applied to the validation instance of GridLAB-D, while the _run-options_ are applied to the test instance of GridLAB-D. For example, 
    
    
    host% **gridlabd --[threadcount] 2 --validate** 
    

will run two validation tests simultaneously, while 
    
    
    host% **gridlabd --validate --[threadcount] 2**
    

will run a single two-threaded validation test at a time. 

### Output

Validation test failures are reported as errors. Optional test failures are reported as warning. 

By default all output is redirected to the default output streams (see [redirect]). You can send all output to the console using --[redirect none] command option. 

### Options

The validate procedure is controlled using the following [global variables]. 

`--define|-[D] [clean]=1`
    If the `clean` variable is defined, the directories used to run each test are purged before the test is started.

`--define|-[D] [validate]=NONE|TRUN|TERR|TEXC|TOPT|TSTD|TALL|RDIR|RGLD|RALL`
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

`--define|-[D] [force_validate]=1` :
Overrides the effect of the `validate.no` file if found in a folder. Normally, validate does not process folders that contain the file `validate.no`.


### Report Format


When a validation run is performed only progress and unexpected results are normally displayed on screen. However, a detailed validation report is output to the file `validate.csv`. You may change the name and type of the validation report file by setting the [validate_report] global variable. For example, 

    
    host% **gridlabd -[D] [validate_report]=validate.txt --validate**
    

will output a (tab-delimited) TXT file instead of a CSV file. 

The validation report is formatted to facilitate parsing by scripts and includes the following sections. 


* **TEST CONFIGURATION** -
    This section provide general information about the gridlabd environment configuration used to perform the validation test.
* **DIRECTORY SCAN RESULTS** -
    This section lists how many files were found in each directory scanned. This section is only output if the **RDIR** or **RALL** [validate (global)] options are set.
* **FILE TEST RESULTS** -
    This section lists the results of each test and is only output if the **RGLM** or **RALL** [validate (global)] options are set. In the first column `S` indicates the test unexpectedly succeeded, `E` indicates the test unexpectedly produced an error, and `X` indicates the test unexpectedly produces an exception. The second column provides the runtime of the test. The third column provides the name of the GLM file tested.
* **OVERALL RESULTS** -
    The section summarizes the overall results of the validation test. Any unusual results are flagged with a triple exclamation point (**!!!**) in the first column. The result code is a unique number that is generated that can be compared to validation test results from other platforms. A result code having all zeros represents complete success. Non-zero values correspond to various failure modes on specific files.

### Mailing Reports


Linux/Mac only
    
    When a validation run is completed, a copy of the validation report can be emailed using the [mailto] global variable. For example

    host% **gridlabd -[D] [mailto] _user_ @localhost --validate**

Will email the validation report in to _user_ on the local machine. 

Note -
    Your local mail server must be configured properly to deliver email to remote hosts and your mail client must be configured properly to read mail on the local mail server. GridLAB-D cannot detect either mail server or mail client configuration errors. Thus problems with either of these can cause mailed report delivery errors to occur with an error being reported in GridLAB-D.

## Caveats

Multithreaded operation can cause intermingled output, particularly when used in conjunction with --[redirect none]. This is due to the lack of locking in the output message streams when running multiple jobs on a single console. 

Some output from gridlabd runs cannot be redirected and will always be displayed on the console. Known problems include output from compilers, linked applications, and scripts. 

## Bugs

Unhandled exceptions in Windows can cause modal dialogs to pop up that block the process. This behavior appears to be impossible to suppress. See <http://sourceforge.net/p/gridlab-d/tickets/606> for details. 

## Version

The built-in validate option was introduced in [Hassayampa (Version 3.0)]. 

## See also

  * [Command options]
  * [Global variables]



  
