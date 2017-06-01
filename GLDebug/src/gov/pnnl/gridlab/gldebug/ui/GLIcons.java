/**
 * <p>Title: GLDebug</p>
 *
 * <p>Description: Provides one place to acces all of the icons.
 * </p>
 *
 * <p>Copyright: Copyright (C) 2008</p>
 *
 * <p>Company: Battelle Memorial Institute</p>
 *
 * @author Jon McCall
 * @version 1.0
 */

package gov.pnnl.gridlab.gldebug.ui;

import java.io.File;

import javax.swing.ImageIcon;

public final class GLIcons {
	
	public final static String SPLASH_NAME = "GLSplash.jpg";
		
    //public final static ImageIcon SPLASH = getImageIcon("gdlogo.jpg");


	private static GLIcons instance;
	
	private static GLIcons getInstance() {
		if(instance == null){
			instance = new GLIcons();
		}
		return instance;
	}


	public static ImageIcon getImageIcon(String imageName)
	{
		String resourceName = "/images/"+imageName;
	    java.net.URL imageURL = getInstance().getClass().getResource(resourceName);
        if (imageURL == null) {
	    	// if unable to find in the jar, try the file system
            //System.err.println("##Software configuration error: jar missing icon image ["+resourceName+"]");
            String fileName = "./src/images/" + imageName;
            File file = new File(fileName);
            if(!file.exists()){
                System.err.println("**** Software configuration error: jar missing icon image ["+resourceName+"]");
            }
            return new ImageIcon(fileName);
	    }
	    return new ImageIcon(imageURL);
	}

}



