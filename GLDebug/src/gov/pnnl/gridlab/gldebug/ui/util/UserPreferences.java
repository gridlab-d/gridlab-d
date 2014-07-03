/*
 * <P>Title:
 * <P>Description:  This (singleton) class manages preferences for the user.
 *  <P>
 *  Right now it stores data on the local hard drive.  One goal of this class
 *  is to provide an easy way to migrate to storing the data on the database.
 *  This class needs to insulate users of it so they don't have to know where the
 *  data is being stored.
 *
 * <P>Copyright 2002 Battelle Memorial Institute
 * @author Jon McCall
 * @version 1.0
 *
 */

package gov.pnnl.gridlab.gldebug.ui.util;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2003</p>
 * <p>Company: Battelle Memorial Institute</p>
 * @author
 * @version 1.0
 */



import gov.pnnl.gridlab.util.IniFileAccess;

import java.io.*;

import java.awt.*;

public class UserPreferences {

    private static final String PREF_FILE = "GLDebug.ini";
    private static UserPreferences mUserPreferences;

    private IniFileAccess mIniFileAccess;

    private UserPreferences () {
        try {
            File file = new File(PREF_FILE);
            if( !file.exists() ) {
                file.createNewFile();
            }
            mIniFileAccess = new IniFileAccess(PREF_FILE);
        }
        catch (Exception ex) {
            System.out.println("Error loading preferences: " + ex.toString());
        }
    }

    /**
     * get the one and only instance of this class
     */
    public static UserPreferences instanceOf (){
        if( mUserPreferences == null ) {
            mUserPreferences = new UserPreferences();
        }
        return mUserPreferences;
    }

    /**
     * simple name, value pair access
     */
    public String getValue (String section, String key, String defaultValue){
        return mIniFileAccess.getPrivateProfileString(section, key, defaultValue);
    }
    /**
     *
     */
    public void setValue (String section, String key, String value){
        mIniFileAccess.writePrivateProfileString(section, key, value);
    }

    /**
     *
     */
    public boolean getBooleanValue (String section, String key, boolean defaultValue){
        String s = getValue(section, key, Boolean.toString(defaultValue));
        return (s.compareToIgnoreCase("true") == 0);
    }
    /**
     *
     */
    public void setBooleanValue (String section, String key, boolean value){
        setValue(section, key, String.valueOf(value));
    }

    /**
     *
     */
    public void setIntValue (String section, String key, int value){
        setValue(section, key, String.valueOf(value));
    }
    /**
     *
     */
    public int getIntValue (String section, String key, int defaultValue){
        String s = getValue(section, key, Integer.toString(defaultValue));
        return Integer.parseInt(s);
    }

    /**
     *
     */
    public void setLongValue (String section, String key, long value){
        setValue(section, key, String.valueOf(value));
    }
    /**
     *
     */
    public long getLongValue (String section, String key, long defaultValue){
        String s = getValue(section, key, Long.toString(defaultValue));
        return Long.parseLong(s);
    }

    /**
     *
     */
    public void setFloatValue (String section, String key, float value){
        setValue(section, key, String.valueOf(value));
    }
    /**
     *
     */
    public float getFloatValue (String section, String key, float defaultValue){
        String s = getValue(section, key, Float.toString(defaultValue));
        return Float.parseFloat(s);
    }

    /**
     *
     */
    /**
     * Save the window location at the section specified.  Prefix lets you save
     * several windows under the same section and differentiate between them.
     * @param win
     * @param section
     * @param prefix
     */
    public void saveWindowLocation (Window win, String section, String prefix){
        if (win.isShowing()) {
            Point pt = win.getLocationOnScreen();
            Dimension dim = win.getSize();

            // do not save if minimized, or if too short (not all showing)
            // if minimized the location is -32000,-32000.  So we check to make sure
            // that it's greater than that.  Multiple monitors can result in negative values
            // so we allow positioning a little into the negative.
            if ((pt.x >=-2000 ) && (pt.y >= -2000)) {
                setIntValue(section, prefix + "x", pt.x);
                setIntValue(section, prefix + "y", pt.y);
                setIntValue(section, prefix + "width", dim.width);
                setIntValue(section, prefix + "height", dim.height);
            }
        }

    }

    /**
     * load the window location.  See saveWindowLocation.
     */
    public Rectangle getWindowLocation (String section, String prefix, Rectangle defaultLocation){
        Rectangle rect = new Rectangle();

        rect.x = getIntValue(section, prefix + "x", defaultLocation.x);
        rect.y = getIntValue(section, prefix + "y", defaultLocation.y);
        rect.width = getIntValue(section, prefix + "width", defaultLocation.width);
        rect.height = getIntValue(section, prefix + "height", defaultLocation.height);
        return rect;
    }


    /**
     * Blob data is text that contains carriage control.  For now we create
     * a separate file and store the data their.  Someday maybe the database.
     */
    public String getTextBlobValue (String section, String key, String defaultValue){
        File file = new File(section + "." + key + ".blob");
        if( !file.exists() ) {
            return defaultValue;
        }
        else {
            FileReader reader=null;
            try {
                reader = new FileReader(file);
                char[] cbuf = new char[10240];
                StringBuffer sb = new StringBuffer();
                int len = 0;
                while( (len = reader.read(cbuf)) != -1 ){
                    sb.append(cbuf, 0, len);
                }

                return sb.toString();
            }
            catch (FileNotFoundException ex) {
                ex.printStackTrace();
                return defaultValue;
            }
            catch (IOException ex) {
                ex.printStackTrace();
                return defaultValue;
            }
            finally {
                try { reader.close(); } catch (IOException ex) {};
            }
        }
    }

    /**
     *
     */
    public void setTextBlobValue (String section, String key, String value){
        File file = new File(section + "." + key + ".blob");
        FileWriter writer=null;
        try {
            writer = new FileWriter(file);
            writer.write(value);
            writer.flush();
        }
        catch (IOException ex) {
            ex.printStackTrace();
        }
        finally {
            try { writer.close(); } catch (IOException ex) {};
        }
    }


    /**
     *
     */
    public void deleteTextBlob (String section, String key){
        File file = new File(section + "." + key + ".blob");
        if( file.exists() ) {
            file.delete();
        }
    }

}



