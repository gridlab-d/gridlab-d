# Testing and Validation 

Each test case must have a header containing descriptive information. It will have the below format and will be inserted at the beginning of the testcase file. Each test-case can have one or more test-units, and their number will be counted in the test case.  

## Test Case Header
  
    //====================================================================== 

    // Test case name: 

    // Functional requirement: 

    // Module(s): 

    // Object(s): 

    // Prerequisites: 

    // Created by/when: 

    // Modified by/when: 

    // Testcase description: 

    //---------------------------------------------------------------------- 

    // Test Unit Number: 

    // Test Unit Description: 

    // Input 

    // Expected Output: 

    //====================================================================== 

Where 

* **Functional requirement** will identify the requirement and its location as being from repository, or functional document, or web page.   

* **Module(s)** contains name of the module being tested.  

* **Object(s)** lists the objects presents in the current module.

* **Prerequisites** lists the prerequisite component(s) specific to the tested module.

* **Created by/when** records creator name and the date of creation.

* **Modified by/when** records modification date and name of the change author.

* **Testcase description** presents in detail the purpose and objective of the test case.

The start of each test-unit must be marked and documented:  

* **Test Unit Number** is the count number of the test unit in the current test case.

* **Input** represents the set of values that are required for the run of the test-unit.

* **Expected Output** presents the desired output of the test-unit.


# Traceability Matrix

This matrix defines the mapping between functional requirements and prepared testcases by test engineers. Traceability matrix is used by testing team to verify how far the testcases prepared have covered the requirements of the tested functionalities. 

The traceability matrix is grouped by the module, primary and secondary object(s) tested. Since our requirements are not grouped numbers, the repository location is used for identifying the main groups of test. 

TODO - LINK - I need to find a way to embed excel in wiki in order to display the traceability matrix. 

[test.htm](/wiki/docs/test.htm)

