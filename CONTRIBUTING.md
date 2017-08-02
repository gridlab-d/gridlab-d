# Contributing To GridLAB-D

*This page is a work in progress*

## Table of Contents
1. [Welcome](#welcome)
2. [What Should I Know First](#starting)
3. [How Can I Contribute](#contribution)
3.1 [Reporting Bugs](#issues)
3.2 [Suggesting Enhancements](#enhancements)
3.3 [Submitting Code](#codeSubmittal)
4. [Coding Practices and Standards](#codingQA)

## Welcome! <a name="welcome"></a>
Hello user and/or contributor of GridLAB-D. Your patronage to the GridLAB-D project is welcome. This page is here to provide you with some guide lines for submitting bugs and features you have found as well as offering your own code to be included in the source. These guidelines are here to help both you, the contributor, and us, the project stewards, to avoid unnecessary hiccups in working together to present a grade A class product to the GridLAB-D community. Therefore, I urge you to read and adhere to these guidelines.

## What Should I Know First? <a name="starting"></a>
GridLAB-D's main page is found [here](http://www.gridlabd.org/). It's in a current state of construction due to our change to using GitHub as our main repository. So while it is under construction I'd like to point you to a couple links.

* [GridLAB-D's Wiki](http://gridlab-d.sourceforge.net/wiki/index.php/Main_Page) contains just about everything you need to know about building, using, and developing GridLAB-D.
* There is a new [biginner's guide](http://gridlab-d.sourceforge.net/wiki/index.php/Chapter_0_-_Introduction) for GridLAB-D! If you are wondering about how to do something or how to use the plethora of models contained in GridLAB-D start with  this guide.
* If you wish to develop something new inside of GridLAB-D. We have a [developer's guide](http://gridlab-d.sourceforge.net/wiki/index.php/Guide_to_Programming_GridLAB-D).
* If the links above have failed to help you solve your problem check the [forum page](https://sourceforge.net/p/gridlab-d/discussion/?source=navbar).

## How Can I Contribute? <a name="contribution"></a>

### Reporting Bugs: <a name="issues"></a>
This section guides you through submitting a bug/enhancement/documentation report for GridLAB-D. Following these guidelines helps maintainers and the community understand your report and reproduce the behavior.

Before creating a report please check [this list](#issueCheckList) as you might find out that you don't need to create one. Please [include as many details as possible](#issueDetails) when creating a bug report.

#### Before Submitting A Bug Report! <a name="issueCheckList"></a>
* Visit the links provided in the [What Should I Know First?](#starting). The majority of the time you'll find the answer to your problem/question in the wiki or the forums.
* Search the [existing issues](https://github.com/gridlab-d/gridlab-d/issues) to see if the problem has already been reported. If it has, feel free to add a comment instead of opening a new issue.

#### How Do I Submit A Good Bug Report? <a name="issueCheckList"></a>
Bugs are tracked as [GitHub Issues](https://guides.github.com/features/issues/). Create an issue and provide the following information.

* Use a clear and descriptive title identifying the problem. Preferably under 72 characters.
* Describe the problem with as many details as possible.
* Provide links to documentation you followed.
* Provide all model, input, and output files used/generated when encountering the problem. Due to GitHub's limited file type support for attachments it would be easiest to supply your files in a tar/zip folder.
* Provide system and software information like the OS and GridLAB-D version you are using.
* Give descritpions of the expected behavior and the observed behavior.

### Suggesting Enhancements: <a name="enhancements"></a>
This section guides you through submitting an enhancement suggestion for GridLAB-D. This includes request for better/clearer documentation to proposing completely new features and improvements to existing functionality. Following these guideline helps maintainers and the community understand your suggestion.

Before creating an enhancement suggestion, check [this list](#enhancementCheckList) as you might find out that you don't need to create one. Please [include as many details as possible](#enhancementDetails) when creating an enhancment suggestion.

#### Before Submitting An Enhancement Suggestion! <a name="enhancementCheckList"></a>
* Visit the links provided in the [What Should I Know First?](#starting). The majority of the time you'll find the answer to your problem/question in the wiki or the forums.
* Search the [existing issues](https://github.com/gridlab-d/gridlab-d/issues) to see if the problem has already been reported. If it has, feel free to add a comment instead of opening a new issue.

#### How Do I Submit a Good Enhancement Suggestion? <a name="enhancementDetails"></a>
Enhancement suggestions are tracked as [GitHub Issues](https://guides.github.com/features/issues/). Create an issue and provide the following information.

* Use a clear and descriptive title identifying the enhancement. Preferably under 72 characters.
* Describe the enhancement with as many details as possible.
* Explain why the enhancement would be useful.
* List some applications where this enhancement exists if applicable.
* Provide system and software information like the OS and GridLAB-D version you are using.
* Give descritpions of the current behavior and the expected behavior.

### Submitting Code: <a name="codeSubmittal"></a>
This section will guide you through the process for submitting code to the project source. here are the "Thou Shalt Do's and Don'ts" for code submittal.
* All code gets submitted to the source through [GitHub Pull Requests](https://help.github.com/articles/creating-a-pull-request/).
* When creating a pull request, the base repository will be the develop branch.
* Any third party libraries used must not corrupt the GridLAB-D License.
* Currently code must be able to compile under -std=c90 for C and -std=c\++98 for C\++. We are looking into supporting c\++11.
* Whenever possible, an unit test must be created for validating the new functionality/bugfix. This unit test should be added to the autotest folder for the specific module the work was done in. 
* Post the validation script results of your build in the pull request. To run the validation script simpley run <pre><code>gridlabd \--validate</code></pre>
* There must be no failed tests from the validation script in order for your pull request to be accepted.

## Coding Practices and Standards: <a name="codingQA"></a>
The coding practices and standards for GridLAB-D can be found [here](http://gridlab-d.sourceforge.net/wiki/index.php/Programming_conventions)
