/**
 *  GLDebug
 * <p>Title: The main class of the application.</p>
 *
 * <p>Description:
 * </p>
 *
 * <p>Copyright: Copyright (C) 2008</p>
 *
 * <p>Company: Battelle Memorial Institute</p>
 *
 * @author Jon McCall
 * @version 1.0
 */

package gov.pnnl.gridlab.gldebug;

import java.awt.Frame;

import gov.pnnl.gridlab.gldebug.model.GLDebugManager;
import gov.pnnl.gridlab.gldebug.model.GLVersion;
import gov.pnnl.gridlab.gldebug.ui.GLDebugFrame;
import gov.pnnl.gridlab.gldebug.ui.GLIcons;
import gov.pnnl.gridlab.gldebug.ui.SplashScreen;

import javax.swing.UIManager;

/**
 *
 */
public class GLDebug {

    public static void main(String[] args) {
        initApp();
    }

    public static void initApp() {
        try {
            // set to the default local system L&F
            UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
        } catch (Exception e) {
            e.printStackTrace();
        }
        System.out.println("-------------------------------------------------------------");
        System.out.println("GLDebug Started: (" + GLVersion.getVersion() + " " + GLVersion.getBuildDate() + ")");
        System.out.println("  Java Version: "+ System.getProperty("java.version") );
        System.out.println("  Os Version: "+ System.getProperty("os.name") + " " + System.getProperty("os.version") );
        System.out.println("  User: "+ System.getProperty("user.name") );
        System.out.println("  Directory: "+ System.getProperty("user.dir") );
        System.out.println("  Memory Max: "+ (Runtime.getRuntime().maxMemory() / 1048576) + "m" );

        System.out.println("  java.home: " + System.getProperty("java.home"));
        System.out.println("  java.class.path: " + System.getProperty("java.class.path"));
        System.out.println("  java.vm.version: " + System.getProperty("java.vm.version"));
        System.out.println("  java.vm.vendor: " + System.getProperty("java.vm.vendor"));
        System.out.println("  java.vm.name: " + System.getProperty("java.vm.name"));
        System.out.println("  java.class.version: " + System.getProperty("java.class.version"));
        System.out.println("  os.home: " + System.getProperty("os.name"));
        System.out.println("  os.arch: " + System.getProperty("os.arch"));
        System.out.println("  os.version: " + System.getProperty("os.version"));
        System.out.println("  user.home: " + System.getProperty("user.home"));


        UIManager.put("DesktopIconUI", "unshred.view.MMDesktopIconUI");

        SplashScreen.showSplashScreen(GLIcons.SPLASH_NAME, 1500, null);

        GLDebugFrame frame = new GLDebugFrame();
        GLDebugManager.instance().setMainFrame(frame);
        frame.setVisible(true);


    }

}
