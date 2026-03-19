## Visualization Tools

**Google Earth support**: Currently, GridLAB-d supportx KML output for Google Earth. However, only the **powerflow** module supports the export of KML when the `--kmldump` command line argument is given. There is an effort under way at PNNL to extend and generalize this capability so that all objects in the model are output to KML and users can navigate the objects using Google Earth.

In the Version 1 **kmldump** routine, only objects that have geocoordinates defined are output to KML. In Version 2, an additional option is desired whereby an object that does not have geocoordinates is output as a part its parent object. Accessing the parent object give the user access to an object tree of all those that have been attached at that location.
