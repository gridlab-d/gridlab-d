/**
 * <p>Title: GLDebug </p>
 *
 * <p>Description: Holds the current software version information.
 * The ant build process in the installer package replaces the $ANT_ strings below with the
 * appropriate values.  If the values are not replaced we know it's a developer version
 * and we set the version info to that.
 * </p>
 *
 * <p>Copyright: Copyright (C) 2008</p>
 *
 * <p>Company: Battelle Memorial Institute</p>
 *
 * @author Jon McCall
 * @version 1.0
 */

package gov.pnnl.gridlab.gldebug.model;


public class GLVersion {
    private static String mVersion = "$ANT_MM_VERSION$";
    private static String mBuildDate = "$ANT_MM_BUILD_DATE$";

    static {
        // fix the version message string for devel builds.
        if( mVersion.contains("$ANT_MM") ) {
            mVersion = "Development Version";
            mBuildDate = "";
        }
    }

    public static String getVersion() {
        return mVersion;
    }
    public static String getBuildDate() {
        return mBuildDate;
    }

}
