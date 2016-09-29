Process for producing circuit mile outputs:

These steps need to be performed once per circuit.  The final *.mat files can be saved and re-used.
***************************************************************************************************
1. Run the circuit model with two changes
	 a. Use FBS, rather than NR.
	 b. Add #set savefile=my_xml.xml to the glm
   This should produce an XML output even if the circuit doesn't converge - don't worry about that.  We just need this to get the rank-order mapping that is produced by the FBS system.  However, if the model fails during init() (should be clearly stated in the error message), then this won't work.

2. Open my_xml.xml in your favorite text editor. There should be a section marked with <powerflow> ...stuff... </powerflow>. Copy this section into another document and save it as a new XML document.

3. Run XMLReader.m on the XML file created in (2).  This will take a long time, so sit back and wait.  The biggest file I tried took about 20 minutes.  THis will save a file called "theStruct.mat".

4. Run XMLCleanup.m.  It is looking for the file created in (3), so if you run this all out of the same folder, it will find it just fine.  DO NOT CLEAR MEMORY AFTER THIS SCRIPT.

5. Run NodeLinkList.m while the structures created in (4) are still part of the workspace.  Saves two files, linkList.mat and nodeList.mat.

6. Run CalcDistances.m.  Loads the files from (5) and produces linkList2.mat and nodeList2.mat.  Save these somewhere safe, as they contain all the needed distance data and can be re-used.
***************************************************************************************************

These steps can be repeated as necessary.
===================================================================================================
1. In the original GLM file (i.e., still NR), add voltdump objects for any given time that you want to look at the voltages:
    object voltdump {
		filename test_voltdump.csv;
		runtime '2012-07-15 06:00:00';
	}
   You can create a series of these at different times...just number the filenames to keep track of them (e.g., test_voltdump_1.csv).
		
2. Run the GLM.

3. Run VoltageData_CircuitMiles.m.  Set the primary voltage of the circuit (e.g., 7200 or 6928).  Also note that there are two locations to point to, one the location of "nodeList2.mat" and one to the voltdump.csv.  This is in case you want to make a library of distance files and not worry about over-writing something by accident.  This script should produce a file called "voltage_distance_data.mat".

4. Run VoltageView_CircuitMiles.m.  You can turn on or off the secondary voltages if the figure gets too busy.  There are also options to set this to run a series of figures...but by default, its set to a single.