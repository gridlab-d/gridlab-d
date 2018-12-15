## Important Notice

This repository is a fork of the official version of GridLAB-D.  To make contributions to that version, see the PNNL-managed GitHub repository at https://github.com/gridlab-d/gridlab-d].

Authorized github users may contribute to this repository. You must be a Stanford faculty, staff, student, affiliate, or subcontractor to be a contributor.  Please contact the repository owner to be authorized as a contributor.

# SLAC Workflow
The following diagram provides an overview of the SLAC Workflow for this repository
~~~
[SLAC creates a project] --> [Issue created] --> [Issue assigned] --> [Branch created] 
                                                                             |
     +-----------------------------------------------------------------------+
     |
     V
[PR created] --> [Work done] --> [PR Review] --> [PR ok?] (y) --> [PR Merged] --> [Issue ok?] (y) --+
     ^                ^                            (n)                               (n)            |
     |                |                             |                                 |             |
     |                +-----------------------------+                                 |             |
     +--------------------------------------------------------------------------------+             |
                                                                                                    V
                                                                                             [Issue closed]
~~~
When a PR is merged, a new docker build and push must be run to incorporate the changes into the latest docker hub image.

## Projects

SLAC has a number of active projects making contributions to the SLAC development fork.  Project are encouraged to use the `master` branch. Project may use a sub-master branch named `project-master`, which can be used to consolidate PRs from multiple project-specific branches.

## Issues

Issues may be contributed by project team members at any time.  The author of the issues should specify the label, originating project, milestone, and an initial assignment if possible. Please provide any detail and supporting data necessary to reproduce the issue.

## Branches

Working branches are created to address a single issue.  A comment should be added to the issue to indicate which branch is being used to address it.  If the branch addresses multiple issues, pick one issue to use as the primary issue, and have all the other issues refer to both the primary issue and the branch. Do not close the referring issues unless the primary issue will address it.

Working branches are named using the lead developer and a unique description topic, e.g., `leader_github_username/some_topic`. Commits and pushes of the branch should be made as soon as work is initiated in order to facilitate discussion early on. Once a branch is started, discussions should take on the PR page and the issue should only be used to add new information relating to the issue not captured in the PR record, e.g., references to new issues or new branches.

## Pull Request

A pull request should be opened as soon as work on the branch is pushed.  Initially the PR should have a the same title as the primary issue it is intended to address. The title should include a prefix tag `[WORKING]` indicating the initial working status.  

When the PR is initially created, the first comment must include the following information:
~~~
This PR addresses issue #XYZ.

The current status of the PR is:
1. list of open questions/problems

The affected wiki page(s) are:
- [[link to page]]
~~~
All discussion regarding the branch must be recorded on the PR, rather than on the issue.  

The following prefix tags are the recognized:

1. `[WORKING]` -- The PR is current work in progress.  A reviewer may be assigned as a courtesy to offer the opportunity to consider the changes while work is still in progress. The PR may be merged, but the PR and the branch must not be deleted while work is in progress. While work is in progress the PR is assigned to the lead.  When the lead determines that the work is complete, the tag is changed to `[REVIEW]` and makes the file reviewer selection(s).  The PR remains assigned to the lead until the review is completed. _NOTE_: the changes must include wiki updates, if any, and the opening PR comment must cite the wiki pages added or changed.

1. `[REVIEW]` -- The PR is ready for final review.  The reviewer is charged with ensuring that the proposed changes address the issue(s) satisfactorily.  If the review is successful, the reviewer makes a comment to that affect and changes the assignment to a developer other than the lead to perform the merge.  If the review is unsuccessful, the review makes any necessary comments, and changes the tag back to `[WORKING]` to indicate that the lead must address the questions. _NOTE_: the review must include review of the wiki pages added or changed, if any, as well as the docker image built from the the `utilities/docker` folder.

1. `[HOLD]` -- The PR is on hold pending future resolution by the lead developer.  No further work is expected and the PR should not be merged at this time.

_Note_: If a merge is performed, the PR is automatically closed.  If the original issue is resolved, then it may be closed at this point with a comment indicated why it is closed.  However, if the issue is not fully resolved, or if the PR has unresolved problems or questions, then a new PR will be needed to address those.  In that case, the issue should not be closed.  Instead a comment should be added to the original issue with a copy of the outstanding problems or questions from the closed PR.

## Documentation changes

All documentation changes arising from new features and capabilities should be made to this repository's wiki pages at https://github.com/dchassin/gridlabd/wiki. Fixes or correction to GridLAB-D's online documentation should be made to https://gridlab-d.shoutwiki.org/.

# Coding Conventions

The general GridLAB-D coding conventions are described at http://gridlab-d.shoutwiki.com/wiki/Programming_conventions.

# Copyrights and Licensing

All code changes are copyright of the original authors or their assignees.  All code must be licensed in accordance with the original GridLAB-D license (see https://github.com/gridlab-d/gridlab-d/blob/master/LICENSE).  Please do not contribute code which conflicts with this license. In particular, so-called "contaminating licenses" are prohibited.
