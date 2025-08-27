# 0.0 GridLAB-D
- [x] GLD Association? Is that still a thing? --> it is not, removed from repo.

# 1.0 Prospective Users
- [ ] Technical Overview needs rework

# New Users
- [x] Keep metronome example? --> No, delete.
    - [x] Pull metronome mentions out of Getting Started and into a single metronome example 
    file (or delete if not kept) 
    - [ ] Intro to Programming needs to be re-written to not use metronome
- [ ] Definitive installation instructions
    - [x] add the "easy" or "light" executable install option
- [x] Pair down Getting Started Using GLD to be more approachable, noting that much of what's in there is already/can be moved to the Modeling 101 section of Modeling

# 3.0 Modeling
- [ ] XML files. Still supported? Keep documentation?
- [ ] How do we handle objects? Source code integration/conversion/hybrid?
    - [ ] How does Mkdocs handle linking/interactive display elements? 
        (built-in or scripted?)
    - [ ] Talk with dev team to build out the built-in documentation within the code itself

# 4.0 Developing
- [ ] 4.1 Write intro for developer documentation (who is this section of the documentation for and what is the content)
- [ ] 4.2.1 Update and verify pre-requisite installation command list for each build platform
- [ ] 4.2.3 Describe how to update the PATH variable for WSL
- [ ] Spec pages: planning document written pre-implementation -- is it actually implemented?
- [ ] Missing or needing updated: release process
- [ ] Missing or needing updated: build process
- [ ] Missing or needing updated: style guide
- [ ] Missing or needing updated: comprehensive developer API reference
- [ ] Missing or needing updated: code templates (maybe just delete?)
- [ ] Missing or needing updated: developer guide
- [ ] Missing or needing updated: disccusion of t0, t1, t2 for object model updates
- [ ] Missing or needing updated: disccusion of the various stages of object model update (pre-commit, commit, pre-sync, sync...)


# 5.0 Integration
- [ ] 

# Miscellaneous Notes
- [ ] Branding, logo
- [ ] Math doesn't display on search, can we fix that?
- [ ] Search in general is not great, what can we do
- [x] Get images from wiki (urls to wiki pages will no longer work)
- [ ] Remove "History" sections from docs, irrelevant 
- [ ] Visual navigation:
    - [ ] Pages with subpages should be more visually distinct, bold, underilned, something. The [+] left of the header that is just barely visible until you hover over it is not very helpful.
- [ ] Word/page limit of a single doc page?
- [ ] Consistency of Terms, example blocks `<mymodel>`, for example (maybe standalone page in New Users)
- [ ] Remove dated clauses, like:

    `"As of Hassayampa (Version 3.0)..."`
- [ ] Code blocks have embedded wiki links that will no longer work. Code blocks should be reworked to just display the code snippbit. Can be helpful to refer to original wiki page and copy/paste code. Example, change this:


``
    
    
    host% **gridlabd -[D](/wiki/Define "Define") [validate_report](/wiki/Validate_report "Validate report")=validate.txt --validate**
    

``

to
    
    
    host% gridlabd -[D] [validate_report]=validate.txt --validate
  


## Definition Plan
- Open Index Tracker excel sheet on shareopint --> filter `Page Type` by `Definition`
- Navigate to *Sharepoint/docs/scraped pages/definitions*
- Going down the list, open the next definition page, figure out where that content should be (if you don't know, search the repo for the term and look through the results to find out where it seems to be first introduced).
- Copy the relevant information (definition & synopsis, and another pertinent content)
- Paste into corresponding doc page in the repo where it makes sense to be
- Clean up any formatting/syntax (remove wiki links from code snipbits)
- Note file location in Index Tracker: `Final Doc Location`
- Move original definition file into subfolder of *Sharepoint/docs/scraped pages/definitions/integrated* so we can make sure we get to everything