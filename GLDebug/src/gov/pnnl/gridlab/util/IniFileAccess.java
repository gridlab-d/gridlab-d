/*
 * <P>Title:
 * <P>Description:  Handles file io to an .ini file.
 *
 * <P>Copyright 2002 Battelle Memorial Institute
 * @author Jon McCall
 * @version 1.0
 *
 */


package  gov.pnnl.gridlab.util;
/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2003</p>
 * <p>Company: Battelle Memorial Institute</p>
 * @author
 * @version 1.0
 */


import  java.io.*;
import  java.util.*;

/**
 * handles reading/writing to a windows style ini file.
 */
public class IniFileAccess {
    private BufferedReader mbr;
    private String msCurrentSection = "";
    private String msIniFileName;
    private List mSections = new Vector();                    // simple list of all section names
    private Hashtable mCurrentSectionData = null;               // name -> value
    private Hashtable mSectionInfo = new Hashtable();           // section name -> hashtable of name/value pairs

    private class NameValuePair {
        public String name;
        public String value;
    }

    /**
     *
     * @param     String sIniFile
     */
    public IniFileAccess (String sIniFile) throws IOException, FileNotFoundException
    {
        msIniFileName = sIniFile;
        mbr = new BufferedReader(new FileReader(sIniFile));
        String line = mbr.readLine();
        while (line != null) {
            line = line.trim();
            if (line.startsWith(";") || line.length() == 0) {
            // do nothing
            }
            else if (line.startsWith("[")) {
                // get the section name
                int index = line.indexOf(']');
                if (index > 1) {
                    String section = line.substring(1, index);
                    msCurrentSection = section;
                    mSections.add(section);
                    mCurrentSectionData = new Hashtable();
                    mSectionInfo.put(section, mCurrentSectionData);
                }
            }
            else {
                if (mCurrentSectionData != null) {
                    NameValuePair nvp = parseOutNameValuePair(line);
                    if (nvp != null) {
                        mCurrentSectionData.put(nvp.name, nvp.value);
                    }
                }
            }
            line = mbr.readLine();
        }
        mbr.close();
    }

    /**
     *
     * @param line
     * @return
     */
    private NameValuePair parseOutNameValuePair (String line) {
        // parse out name=value pairs
        String name = "";
        String value = "";
        int index = line.indexOf("=");
        if (index > 0) {
            name = line.substring(0, index);
            value = line.substring(index + 1);
            if (name.length() <= 0)
                return  null;
        }
        else
            return  null;
        NameValuePair nvp = new NameValuePair();
        nvp.name = name;
        nvp.value = value;
        return  nvp;
    }

    /**
     * get the value of the item in this section with this key name (default if not found)
     */
    public String getPrivateProfileString (String sectionname, String keyname, String defaultvalue) {
        if (!loadSectionData(sectionname))
            return  defaultvalue;
        String retVal = (String)mCurrentSectionData.get(keyname);
        if (retVal == null)
            return  defaultvalue;
        else
            return  retVal;
    }

    /**
     * get the integer value of the item at this section and keyname
     */
    public int getPrivateProfileInt (String sectionname, String keyname, int defaultvalue) {
        String sval = getPrivateProfileString(sectionname, keyname, "" + defaultvalue);
        int retval = defaultvalue;
        try {
            retval = Integer.parseInt(sval);
        } catch (Exception ex) {
            return  defaultvalue;
        }
        return  retval;
    }

    /**
     *
     * @param sectionname
     * @param keyname
     * @param value
     * @return
     */
    public boolean writePrivateProfileString (String sectionname, String keyname, String value) {
        //Debug.DEBUG("writing section,key=" + sectionname + ", " + keyname);
        // change our in memory copy
        if (loadSectionData(sectionname)) {
            mCurrentSectionData.put(keyname, value);
        }
        else {
            msCurrentSection = sectionname;
            mSections.add(sectionname);
            mCurrentSectionData = new Hashtable();
            mSectionInfo.put(sectionname, mCurrentSectionData);
            mCurrentSectionData.put(keyname, value);
        }

        // change it on disk
        RandomAccessFile file = null;
        try {
            String lineToWrite = keyname + "=" + (value == null ? "" : value);
            boolean bInSection = false;
            long lastPos = 0;
            file = new RandomAccessFile(new File(msIniFileName), "rw");
            lastPos = file.getFilePointer();
            String line = file.readLine();
            while (line != null) {
                line = line.trim();
                if (line.startsWith(";") || line.length() == 0) {
                // do nothing
                }
                else if (line.startsWith("[")) {
                    if (!bInSection) {
                        // get the section name
                        int index = line.indexOf(']');
                        if (index > 1) {
                            String section = line.substring(1, index);
                            if (section.equals(sectionname))
                                bInSection = true;
                        }
                    }
                    else {
                        // didn't find the key in the section, need to INSERT data
                        file.seek(lastPos);
                        return  insertDataLine(file, lineToWrite);
                    }
                    lastPos = file.getFilePointer();
                }
                else {
                    if (bInSection) {
                        NameValuePair nvp = parseOutNameValuePair(line);
                        if (nvp != null) {
                            //Debug.DEBUG("keyname=" + keyname + ", nvp.name=" + nvp.name);
                            if (nvp.name.equals(keyname)) {
                                // found it, now overwrite it
                                long currentPos = file.getFilePointer();
                                // read in everything from here out
                                byte buf[] = new byte[(int)(file.length() - currentPos)];
                                file.read(buf);
                                // write out the new line
                                file.seek(lastPos);
                                file.write(lineToWrite.getBytes());
                                file.write('\r');
                                file.write('\n');
                                // write rest of data
                                file.write(buf);
                                // set size to current file pointer
                                file.setLength(file.getFilePointer());
                                return  true;
                            }
                        }
                    }
                    lastPos = file.getFilePointer();
                }
                line = file.readLine();
            }
            // if we get here we did not find the section, or found the section but not the key, write it out
            if (bInSection) {
                byte[] b = new byte[1];
                file.seek(file.length() - 1);
                file.read(b, 0, 1);
                if (b[0] != '\r' && b[0] != '\n') {
                    file.write('\r');
                    file.write('\n');
                }
                file.write(lineToWrite.getBytes());
                file.write('\r');
                file.write('\n');
            }
            else {
                //Debug.DEBUG("Did not find it, writing out section and key:" + lineToWrite);
                line = "\r\n\r\n[" + sectionname + "]\r\n";
                file.write(line.getBytes());
                file.write(lineToWrite.getBytes());
                file.write('\r');
                file.write('\n');
            }
        } catch (Exception ex) {
            ex.printStackTrace();
            return  false;
        } finally {
            if (file != null) {
                //Debug.DEBUG("Finished writing key " + keyname );
                try {
                    file.close();
                } catch (Exception ex) {}
            }
        }
        return  true;
    }

    /**
     * insert data into a random access file.
     * We add carriage control for you.
     */
    private boolean insertDataLine (RandomAccessFile file, String data) {
        try {
            long pos = file.getFilePointer();
            // read in everything from here out
            byte buf[] = new byte[(int)(file.length() - pos)];
            file.read(buf);
            // make the file bigger
            file.setLength(file.length() + data.length() + 2);
            // position back to the location and write out the data
            file.seek(pos);
            // write new line
            file.write(data.getBytes());
            file.write('\r');
            file.write('\n');
            // write rest of data
            file.write(buf);
        } catch (Exception ex) {
            ex.printStackTrace();
            return  false;
        }
        return  true;
    }

    /**
     *
     * @param section
     * @return
     */
    private boolean loadSectionData (String section) {
        if (msCurrentSection.equals(section))
            return  true;
        // need to load it
        Hashtable hdata = (Hashtable)mSectionInfo.get(section);
        if (hdata != null) {
            msCurrentSection = section;
            mCurrentSectionData = hdata;
            return  true;
        }
        return  false;
    }

    /**
     * return an enumeration of all of the sections in the ini file ([section 1])
     */
    public Iterator getSections () {
        return  mSections.iterator();
    }

    /**
     * delete the stuff between start and end
     */
    private boolean deleteSection (RandomAccessFile file, long start, long end) {
        try {
            file.seek(end);
            // read in everything from here out
            byte buf[] = new byte[(int)(file.length() - end)];
            file.read(buf);

             // position to the start
             file.seek(start);

            // make the file smaller
            file.setLength(file.length() - (end - start));

            // position back to the location and write out the data
            file.write(buf);
        } catch (Exception ex) {
            ex.printStackTrace();
            return  false;
        }
        return  true;
    }


    /**
     * deletes a section from the file
     */
    public boolean removeSection (String sectionName) {
        // change our in memory copy
        if (loadSectionData(sectionName)) {
            msCurrentSection = "";
            mSections.remove(sectionName);
            mCurrentSectionData = null;
            mSectionInfo.remove(sectionName);
        }

        // delete from the file
        RandomAccessFile file = null;
        try {
            boolean bInSection = false;
            long startPos = -1;
            long endPos = -1;
            long lastPos = 0;
            file = new RandomAccessFile(new File(msIniFileName), "rw");
            lastPos = file.getFilePointer();
            String line = file.readLine();
            while (line != null) {
                line = line.trim();
                if (line.startsWith(";") || line.length() == 0) {
                // do nothing
                }
                else if (line.startsWith("[")) {
                    if (!bInSection) {
                        // get the section name
                        int index = line.indexOf(']');
                        if (index > 1) {
                            String section = line.substring(1, index);
                            if (section.equals(sectionName)){
                                bInSection = true;
                                startPos = lastPos;
                            }
                        }
                    }
                    else {
                        // found the end of the section that we want to delete
                        endPos = lastPos;
                        break;
                    }
                    lastPos = file.getFilePointer();
                }
                lastPos = file.getFilePointer();
                line = file.readLine();
            }

            // if we get here see if we have the section start/end
            if (startPos > -1) {
                if( endPos == -1 )
                    endPos = file.getFilePointer();

                return deleteSection( file, startPos, endPos );
            }
            else {
                return false;
            }
        } catch (Exception ex) {
            ex.printStackTrace();
            return  false;
        } finally {
            if (file != null) {
                try {
                    file.close();
                } catch (Exception ex) {}
            }
        }

    }

//    public static void main (String[] args) {
//        String sIniFile = "c:\\temp\\test.ini";
//
//        try {
//            IniFileAccess ini = new IniFileAccess( sIniFile );
//
//            Iterator iter = ini.getSections();
//            while (iter.hasNext()) {
//                String section = (String)iter.next();
//                System.out.println("Section:  " + section);
//                System.out.println("string1 = " + ini.getPrivateProfileString(section, "string1", "NOT FOUND"));
//                System.out.println("string1 = " + ini.getPrivateProfileString(section, "string2", "NOT FOUND"));
//                System.out.println("int1 = " + ini.getPrivateProfileInt(section, "int1", -1));
//                System.out.println("int2 = " + ini.getPrivateProfileInt(section, "int2", -1));
//
//                ini.writePrivateProfileString(section, "string1", "overwritten string1");
//                ini.writePrivateProfileString(section, "string2", "overwritten string2");
//                ini.writePrivateProfileString(section, "int1", "11111");
//                ini.writePrivateProfileString(section, "int2", "22222");
//            }
//
//            iter = ini.getSections();
//            while (iter.hasNext()) {
//                String section = (String)iter.next();
//                System.out.println("Section:  " + section);
//                System.out.println("string1 = " + ini.getPrivateProfileString(section, "string1", "NOT FOUND"));
//                System.out.println("string1 = " + ini.getPrivateProfileString(section, "string2", "NOT FOUND"));
//                System.out.println("int1 = " + ini.getPrivateProfileInt(section, "int1", -1));
//                System.out.println("int2 = " + ini.getPrivateProfileInt(section, "int2", -1));
//            }
//
//        }
//        catch (Exception ex) {
//            ex.printStackTrace();
//        }
//
//    }
}



