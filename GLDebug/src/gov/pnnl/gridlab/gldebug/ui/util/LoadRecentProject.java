package gov.pnnl.gridlab.gldebug.ui.util;
/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2003</p>
 * <p>Company: Battelle Memorial Institute</p>
 * @author
 * @version 1.0
 */


import gov.pnnl.gridlab.gldebug.model.GLDebugManager;
import gov.pnnl.gridlab.gldebug.ui.ProjectHandler;

import java.awt.event.ActionEvent;
import java.awt.event.KeyEvent;
import java.awt.Cursor;
import java.io.File;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.KeyStroke;

import javax.swing.*;

/**
 *   An object that encapsulates the process of re-opening a project.
 *
 *   @author J McCall
 *   @version 1.0
 */
public class LoadRecentProject extends AbstractAction
{
    private String mDisplayName;
    private String mFilePath;

    /**
     *  Creates a new object that encapsulates the
     *  NewProject action.
     */
    public LoadRecentProject(String displayName, String filePath){
    	super(); 
        this.mDisplayName = displayName;
        this.mFilePath = filePath;
        putValue(Action.ACCELERATOR_KEY, null);
		putValue(Action.DEFAULT, "Reopen Project");
		putValue(Action.LONG_DESCRIPTION, "Reopen Project");
    }

    /**
     */
    public void actionPerformed(ActionEvent ae) {
        System.out.println("Opening project...");
        loadTheProject();
    }
    
    /**
     *  Loads a project from the specified filename.
     */
    public void loadTheProject() {
		ProjectHandler.openProject(mFilePath);
    }
    
    
    /**
     * sets the item index and menu display name
     */
    public void setIndex (int i){
        putValue(Action.NAME, i + " " + mDisplayName);
        putValue(Action.MNEMONIC_KEY, new Integer(Integer.toString(i).charAt(0)));
    }

    /**
     *
     */
    public String getPath (){
        return mFilePath;
    }

    /**
     *
     */
    public String getDisplayName (){
        return mDisplayName;
    }


}
