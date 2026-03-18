# 0.0 GridLAB-D

- [x] GLD Association? Is that still a thing? --> it is not, removed from repo.
- [X] Check links in the publication list, replace with DOIs if possible

# 1.0 Prospective Users

- [ ] Technical Overview needs rework - Review status in Scott's tracker

# New Users

- [ ] **Review tutorial pages, consider length and potentially splitting into smaller, more focused pages**
- [x] Keep metronome example? --> No, delete.
    - [x] Pull metronome mentions out of Getting Started and into a single metronome example 
    file (or delete if not kept) 
    - [ ] **Intro to Modeling needs to be re-written to not use metronome**
- [ ] Definitive installation instructions
    - [x] add the "easy" or "light" executable install option
    - [ ] *NOTE - Dev team does not have installation process well documented (i.e., checking that all installers are functional)*
- [X] MySQL instructions - DEPRECATE/Unimplemented? If so, most content is unfinished. Needs robust introduction explaining what it is and why it is useful.
    - propose to Dev team to get rid of MySQL integration in leiu of python interface with new api (same for Matlab, HELICS, any other external integration)
- [X] Add a troubleshooting section to the new users section walking people through referencing the source code and how to find information they need on the source of truth.
  - [ ] Page started, needs more content.


# 3.0 Modeling

- [X] **XML** files. Still supported? Keep documentation? 
  - remove ref, "soft keep", switch to JSON as default
- [ ] Revisit doxygen/built-in documentation within the code itself
  - [ ] Dev team to build out a few doxygen examples to test campatability and usefulness
- [X] **MySQL** player and recorder - keep? If so, needs better intro. 
  - move to unimplemented
- [X] **Aggregate Demand Response Model** - this is a theory page, unclear what part of the GLD code it's actually referring to. Is this implemented? 
  - ***Moved to unimplemented***
- [X] **Microgrids** and **Diesel_dg** - microgrids is essentially the intro, diesel_dg is the parameter list and seems accurate 
  - Microgrids is a use case, diesel_dg is keep.
- [X] **Energy storage** - page is essentially empty. Should be the battery object? 
  - ***very simplified model, assumes it will be paired with inverter_dyn. Dev team to consider whether it should stay separate or be merged with battery***
  - [ ] Page now contains both generic and battery models. ***Needs some remarks and examples***
- [ ] **Inverter** and **Inverter_Dyn** - pages have status updates, (i.e., inverter_dyn more recent than inverter) but dev team needs to review and possibly rework based on their updates. Still likely out of date, may end up being merged. ***DEV DECISION POINT***.
- [ ] **Power Flow User Guide** - this page has a LOT in it, with a fair amount I've never heard of, with various stages of "this has been partially validated and is considered experimental at this time" : need to review and update/deprecate. 
  - **Topic for a Dev Team Review Meeting**

### Docs Questions
- [ ] Consider: specific status messages for unimplemented material, where appropriate. 
  - I.e., "the functionality of this module has been replaced by *blank*, please checkout that page (link)"
- [ ] How do we handle objects? Source code integration/conversion/hybrid?
    - [ ] How does Mkdocs handle linking/interactive display elements? 
        (built-in or scripted?)
- [ ] Usability and Usefulness
  - [ ] New examples
  - [ ] Additional content/features covered
- [X] Modules - Connection - Does this need individual pages for each type? Or can we merge into one connection page? Existing individual connection pages are very brief/ more like definitions.
  - *Moved to unimplemented*
- [ ] Keep in mind:
  - What are we doing here?
  - Who is a “modeler” and what this guide intends to provide?
  - Reference and on-boarding for new modelers (people who write .glms)
      - Class hierarchy – In GLD, there is a class hierarchy which affects that parameter list for an object. This makes it good to know which objects are sub-classes of other objects. For example, there is a “node” object with, say, a “voltage\_A” property. The “meter” object is a subclass of “node” which means, even though “voltage\_A” won’t show up in its property list, “voltage\_A” is a valid property.
  - Device models – For each device the following
    * Specification page – All the math; this may be something that we try to write into the source code and extract to build the webpage or we write the webpage (via Markdown) and then reference it in the code. We don’t want to have to describe these equations twice (once as code comments and once in the documentation).
    * User page – Parameter list for the object and what each parameter does. Again, may be pulled from the existing source code documentation and created as a webpage; we just don’t want to write things down twice. This is likely to be the more popular page as it is what you need when you’re trying to put a .glm together.
  - [ ] Verifying/debugging a model - consider linking or putting in Troubleshooting section
    * What are the red flags?
    * Common warnings you might be able to avoid
    * Running validation scripts – Something that post-processes results and looks for modeling abnormalities
 
# 4.0 Developing

**Purpose**: part walkthrough, part dictionary/reference

### Dev Questions
- [ ] Waiting on development to document
  - [ ] 4.3.7 loader
  - [ ] 4.3.9 libgld
- [ ] Determine if section is needed
  - [ ] 4.3.2 Modules
  - [ ] 4.3.3 Objects
  - [ ] 4.3.6 Parallelization
  - [ ] 4.3.8 Transforms
  - [ ] 4.4.1 Programming conventions
  - [ ] 4.4.2 Creating a module
  - [ ] 4.4.3 Creating a class
  - [ ] 4.4.4 Device modeling
  - [ ] 4.4.5 Integrated testing
  - [ ] 4.5 Deltamode development
  - [ ] 4.6 Debugging
- [ ] Identify missing sections
- [ ] Update outline
  - [ ] 4.1 Write intro for developer documentation (who is this section of the documentation for and what is the content)
  - [ ] 4.3.0 Software Architecture and Design
  - [ ] 4.3.1 gldcore
  - [ ] 4.3.4 object synchronization process
  - [ ] 4.7 Style guide
  - [ ] 4.8 release process
- [ ] Writing 
- [ ] Reviewing
  - [ ] 4.2 Building from Source
- [ ] Clean-up
  


- [x] Spec pages: planning document written pre-implementation -- is it actually implemented?
- [ ] Missing or needing updated: style guide
- [ ] **Missing or needing updated: comprehensive developer API reference**
- [ ] Missing or needing updated: code templates (maybe just delete?)
- [ ] Missing or needing updated: disccusion of the various stages of object model update (pre-commit, commit, pre-sync, sync...)


# 5.0 Integration
- Deleting for now, preserving intro page:

#### Intro to Integration
excerpt: if libgld comes into existence

* Who is an “integrator” and what this guide intends to provide
* libgld
  * What libgld is intended to provide
  * Why you might want to use libgld
  * Possible applications of libgld
    * Building a use-case specific analysis tool
    * Making a GUI for common analysis you do
    * Integrating a new device model into GLD
* How to use libgld  for one or more of the above candidate applications

# Miscellaneous Notes

- [ ] Branding, logo
- [ ] Math doesn't display on search, can we fix that?
- [ ] Search in general is not great, what can we do
- [x] Get images from wiki (urls to wiki pages will no longer work)
- [ ] Find a way to set default width of images to fill the ReadTheDocs window
- [X] Remove "History" sections from docs, irrelevant 
- [ ] Visual navigation:
    - [ ] Pages with subpages should be more visually distinct, bold, underilned, something. The [+] left of the header that is just barely visible until you hover over it is not very helpful.
- [ ] Word/page limit of a single doc page?
- [X] "See Also" lists:  --> Replaced with "**Related Concepts**"
  - Do we want to keep these? Will have to ensure pages still exist and links are accurate. Do we assume that the pages are now well-organized enough that this is no longer needed?
  - [ ] Converted to "Related Concepts" list for now. When we're ready to finalize we can review and make items links to other pages where relevant.
- [ ] Consistency of Terms, example blocks `<mymodel>`, for example (maybe standalone page in New Users)
  - See style guide for guidance 
- [ ] Remove dated clauses, like:
              `"As of Hassayampa (Version 3.0)..."`
- [X] Code blocks have embedded wiki links that will no longer work. Code blocks should be reworked to just display the code snippbit. Can be helpful to refer to original wiki page and copy/paste code. **Pretty well gotten rid of all these, double check upon finalization.** Example:

    - Change this:   
    
          host% **gridlabd -[D](/wiki/Define "Define") [validate_report](/wiki/Validate_report "Validate report")=validate.txt --validate**

  - to this:
      
      
        host% gridlabd -[D] [validate_report]=validate.txt --validate

# New Doc Pages (Existing Features) Topics

Topic | Start Date | Time Estimate | Completed Date
-- | -- | -- | --
Deltamode | Oct 6 | 3 weeks | Jan 28
Object synchronization process | Wrapped into time management, TODO: check that there isn't anything specific here to investigate
Device model development process | 
GridLAB-D loader and JSON file format | Mostly complete, pending specific parameter list for each object -- could start this any time.

# New Features Dev Schedule
Topic |  Estimate Complete Date | Actual Completed Date | Able to Start Docs?
-- | -- | -- | --
Basic C/C++ API | Oct 31st | | Yes
JSON Loader | <s>Nov 30</s> -> Jan 31 | Does not yet have full parameter list | Possible - Confirm with Frank what "done" means for JSON loader
Checkpoint System | <s>Jan 31</s> -> Feb 27 | Trying to figure out how to save/load| Potentially - Orestis to keep in mind as he works Dev side
Multithreading | Mar 31
Docker Containerization | Mar 15
HELICS Update | Apr 15
Common Solver Decentralization | Mar 31
Utility functions for API | Mar 31
CIMHub Testing/Integration | Apr 15
Rlease v5.4/6.0 | May 31


# Documentation Review Series Meeting Topics

Topic | Date | Outstanding Decisions
-- | -- | ---
Residential Module | Dec 22 | Waterheater needs Multi-Layer model with accurate reflection of its status and stability
Generators Module | Dec 23 | -  Decide whether to merge inverter and inverter_dyn (invluding VSI, part of SETO project) <br/> - Either energy_storage model gets some tests or it is deprecated <br/> - Single- and three-phase motors shoud be revisited, some "extra features" may not be fully documented. <br/> - Unclear if wind turbine is still experimental
Powerflow | Feb 2 | Meeting notes circulated, TODOs pending
Climate | Mar 2
Developing Reference | Mar 2

# Definition Plan
- Open Index Tracker excel sheet on shareopint --> filter `Page Type` by `Definition`
- Navigate to *Sharepoint/docs/scraped pages/definitions*
- Going down the list, open the next definition page, figure out where that content should be (if you don't know, search the repo for the term and look through the results to find out where it seems to be first introduced).
- Copy the relevant information (definition & synopsis, and another pertinent content)
- Paste into corresponding doc page in the repo where it makes sense to be
- Clean up any formatting/syntax (remove wiki links from code snipbits)
- Note file location in Index Tracker: `Final Doc Location`
- Move original definition file into subfolder of *Sharepoint/docs/scraped pages/definitions/integrated* so we can make sure we get to everything

# AI Synthesis of FAQs
"The insights provided [below] are **generalized observations** about forums for software tools like GridLAB-D, based on the typical structure and themes of discussions in technical communities"

### Common Themes:
1. **Installation and Setup Issues:**
   - Users frequently encounter challenges during setup, particularly around dependencies, operating system compatibility, and configuration.

2. **Modeling and Simulation Challenges:**
   - Questions about creating accurate power system models, configuring components, and setting up simulations that reflect specific real-world scenarios.

3. **Custom Modules and Extensions:**
   - Discussions on how to integrate custom modules, program new functionalities, or use the tool's API.

4. **Interoperability with Other Tools:**
   - Questions related to how GridLAB-D integrates with other tools such as OpenDSS, MATLAB, or SCADA systems.

5. **Performance Optimization:**
   - Topics on optimizing simulation runs, especially for large-scale models, to achieve better computation efficiency.

6. **Best Practices:**
   - Community discussions on industry best practices for utilizing GridLAB-D effectively.

---

### Frequently Asked Questions:
1. **How do I configure a basic GridLAB-D model for a specific feeder?**
2. **What are the system requirements, and how do I resolve dependency-related errors?**
3. **Why is my simulation producing unexpected results or failing to converge?**
4. **Can I simulate renewable energy sources like solar or wind in GridLAB-D?**
5. **How do I interpret output logs and simulation results?**
6. **What is the best way to implement demand response algorithms within the tool?**
7. **Are there tutorials for new users, or how do I start learning GridLAB-D?**
8. **How can I debug errors related to input files or configuration setup?**

---

### Areas Where Documentation is Often Unclear:
1. **Advanced Features and Configurations:**
   - Users often report that advanced capabilities of the tool are inadequately documented, resulting in confusion.

2. **Examples and Use Cases:**
   - A lack of detailed, real-world modeling examples can make it difficult for users to understand how to use GridLAB-D effectively for their applications.

3. **Error Messages:**
   - Insufficient explanation of error codes or ambiguous error details in the documentation can delay troubleshooting efforts.

4. **Custom Module Development:**
   - Limited guidance on extending functionality or creating custom modules is a challenge for users looking to innovate.

5. **Interfacing with External Software:**
   - Documentation on integration workflows with external tools can sometimes be light or overly generic.

   here are specific **advanced features of GridLAB-D** that are often a source of confusion or leave room for improvement in clarity:

---

Here are specific **advanced features of GridLAB-D** that are often a source of confusion or leave room for improvement in clarity:
### 1. **Modeling Advanced Grid Components**
   - **Inverter and DER (Distributed Energy Resource) Modeling:**
     - While GridLAB-D supports modeling of renewable energy systems such as solar PV, wind turbines, and inverters, users often struggle with:
       - Setting up **controller behavior** (e.g., droop control, volt-VAR capabilities).
       - Modeling grid-tied inverters with high accuracy.
       - Integrating DERs in large-scale simulations with realistic load profiles.
     - The challenge lies in understanding all configurable parameters and tuning them correctly.

   - **Battery Energy Storage Systems (BESS):**
     - Questions arise about modeling charging/discharging cycles, degradation, and dynamic responses.
     - Certain storage options (e.g., systems with inverter interactions) are insufficiently documented.

   - **Transformer and Line Models:**
     - Users sometimes find the limitations of detailed modeling for transformers and lines on complex systems unclear within the documentation.

---

### 2. **Co-Simulation and Tool Interoperability**
   - Many users want to run **co-simulations** where GridLAB-D is integrated with other tools such as:
     - MATLAB/Simulink.
     - OpenDSS.
     - EnergyPlus (for building energy modeling).
     - SCADA systems and real-time simulators.
   - While GridLAB-D provides provisions for data exchange through APIs, FNCS (Framework for Networked Co-Simulation), or Python scripting, the process of establishing seamless communication between tools is often poorly explained or under-documented.

---

### 3. **Custom Modules and Extensions**
   - GridLAB-D allows users to write **custom modules** in C++ to extend its functionalities, yet:
     - There is limited step-by-step documentation or examples for developing, testing, and debugging custom code.
     - Users struggle with setting up a development environment, particularly with dependencies related to the native toolchain.

   - Many users are unclear on how to safely "inject" functionality into existing modules without risking unintended interactions with other system components during simulations.

---

### 4. **Distributed/Parallel Simulations**
   - GridLAB-D simulations can become computationally intensive for large-scale networks, especially if the model includes:
     - Thousands of nodes or distributed energy resources.
     - Minute-level or second-level timestep resolutions.
   - While support for distributed and parallel simulations exists, documentation on properly configuring this functionality is sparse. Users are often unclear about:
     - How to partition the grid correctly for distributed simulations.
     - Performance optimization to ensure reliable and repeatable results in parallelized environments.

---

### 5. **Dynamic Market and Tariff Modeling**
   - GridLAB-D includes **dynamic pricing and market modeling capabilities** that allow simulations of demand response, real-time pricing, and bidding strategies. However:
     - Users often need clarification when programming market modules or setting up dynamic tariff models.
     - The interaction of market modules with time-dependent customer loads, energy storage, and DERs is complex, and setting up realistic scenarios is non-trivial.
   - There is a lack of user-friendly tutorials or examples that walk through fully functional market-based simulations.

---

### 6. **Stochastic Modeling and Uncertainty Analysis**
   - GridLAB-D supports stochastic inputs, for example, using randomization in load profiles or generation patterns. However:
     - Users face difficulties setting up realistic randomized load or generation profiles, with parameter distributions often poorly explained.
     - There is little clear guidance on conducting **uncertainty analysis** (e.g., Monte Carlo simulations) within the GridLAB-D framework.

---

### 7. **Weather Data Integration**
   - Weather data integration becomes critical in simulations including renewable energy sources or energy demand patterns. Common issues include:
     - Difficulty configuring weather services or incorporating real-world weather data (which may require accessing external libraries or APIs).
     - Lack of clear formatting rules for custom weather datasets.

---

### 8. **Post-Processing and Output Interpretation**
   - The output data produced by GridLAB-D can be extensive and challenging to analyze. Specific problems include:
     - Parsing and interpreting **complex multi-variable outputs.**
     - Understanding time-series results, especially for large-scale systems.
     - Visualizing results in the absence of advanced built-in plotting tools (leading users to depend on third-party tools like Python or Excel).

---

### 9. **Object-Oriented Modeling**
   - GridLAB-D's object-based architecture allows users to define objects (loads, transformers, switches, etc.) and their properties. However:
     - The interplay between certain objects (e.g., parent-child relationships between load objects, meters, and houses) adds complexity.
     - Users find it difficult to customize interactions between grid components without accidentally creating infeasible or unrealistic scenarios.

---

### Why Are These Features Unclear?
- **Sparse Documentation:** While GridLAB-D provides a user manual, some advanced topics are skimmed or assumed to be self-explanatory to experienced users.
- **Fragmented Information:** Key details about parameters and configurations are scattered across the documentation, requiring users to dig deep to assemble a complete understanding.
- **Lack of Examples:** Clear, fully functional examples for advanced simulations (e.g., DER-rich networks, market-driven systems) are often lacking.
- **Open-Source Nature:** Open-source tools often rely on community-driven contributions, which can mean uneven or inconsistent documentation quality.
