package gov.pnnl.gridlab.gldebug.ui.util;

import gov.pnnl.gridlab.gldebug.model.GLProjectSettings;

import java.util.*;
import javax.swing.*;
import java.awt.Component;
import java.awt.event.ActionEvent;
import java.io.File;

/**
 * <p>Title: </p>
 * <p>Description: keeps track of the recent file list and keep
 * the recent file menu up to date.
 * </p>
 * <p>Copyright: Copyright (c) 2004</p>
 * <p>Company: Battelle Memorial Institute</p>
 * @author Jon McCall
 * @version 1.0
 */

public final class RecentFileManager {
    private static RecentFileManager mRecentFileManager;
    private static int MAX_RECENT_FILES = 7;

    private List<LoadRecentProject> mRecentFiles = new ArrayList<LoadRecentProject>();
    private JMenu mnuParent;
    private int mIndex; // index into the menu system where we will start adding items
    private int mCntAdded=0;
    private JComponent mnuInsertBefore;

    private RecentFileManager() {
        // load previous list from project and build a list
        loadRecentFileList();
    }

    /**
     *
     */
    public static RecentFileManager getSingleton (){
        if( mRecentFileManager == null ) {
            mRecentFileManager = new RecentFileManager();
        }
        return mRecentFileManager;
    }

    /**
     * saves the current list of open files to disk.
     */
    private void saveCurrent (){
        UserPreferences prefs = UserPreferences.instanceOf();

        for (int i=0; i < MAX_RECENT_FILES; i++ ) {
            if( i < mRecentFiles.size() ) {
                LoadRecentProject item = (LoadRecentProject)mRecentFiles.get(i);
                prefs.setValue("RecentFiles", "name" + i, item.getDisplayName());
                prefs.setValue("RecentFiles", "file" + i, item.getPath());
            }
            else {
                prefs.setValue("RecentFiles", "name" + i, "");
                prefs.setValue("RecentFiles", "file" + i, "");
            }
        }
    }

    /**
     * load the recent files from disk.
     */
    private void loadRecentFileList (){
        mRecentFiles.clear();

        UserPreferences prefs = UserPreferences.instanceOf();

        for (int i=0; i < MAX_RECENT_FILES; i++ ) {
            String name = prefs.getValue("RecentFiles", "name" + i, "");
            String path = prefs.getValue("RecentFiles", "file" + i, "");
            if( name.length() > 0 && path.length() > 0 ) {
                // make sure the project exists, if it doesn't don't put it in the list
                File f = new File(path);
                if (f.exists()) {
                    mRecentFiles.add(new LoadRecentProject(name, path));
                }
            }
        }

    }

	/**
	 * Loads the most recent project, as listed in the
	 * mRecentFiles list.
	 *
	 */
	public void loadMostRecentProject(){
        if( mRecentFiles.size() > 0 ) {
            UserPreferences prefs = UserPreferences.instanceOf();
            boolean reload = prefs.getBooleanValue("Main", "ReloadLastProject", true);
            if( reload ) {
                LoadRecentProject lrp = (LoadRecentProject) mRecentFiles.get(0);

                File f = new File(lrp.getPath());
                if (!f.exists()) {
                    System.err.println("Error: unable to load recent project: " + lrp.getPath());
                    return;
                }

                lrp.loadTheProject();
            }
        }
	}


    /**
     * associate us with a specific menu
     */
    public void assignToMenu (JMenu menu, JComponent before){
        mnuParent = menu;
        mnuInsertBefore = before;

        Component[] items = mnuParent.getMenuComponents();
        for (int i=0; i < items.length; i++ ) {
            if( items[i].equals(before) ) {
                mIndex = i;
                break;
            }
        }

        mnuParent.insertSeparator(mIndex);

        updateMenu();
    }


    /**
     * notify the a project was opened or saved
     */
    public void activeProjectChange (GLProjectSettings proj){
        if( proj == null ) return;

        String fullPath = proj.getFileName();
        File file = new File(fullPath);
        String displayName = file.getName();
        
        // check if we already have it in the list, if so, put it at the top of the list
        boolean added = false;

        for (Iterator i = mRecentFiles.iterator(); i.hasNext(); ) {
          LoadRecentProject item = (LoadRecentProject)i.next();
          if( item.getPath().equals(fullPath) ) {
              mRecentFiles.remove(item);
              mRecentFiles.add(0, item);
              added = true;
              cleanMenu();
              break;
          }
        }

        // make sure we don't have too many items in the menu
        if( mRecentFiles.size() > MAX_RECENT_FILES ) {
            for (int i = mRecentFiles.size() - 1; i > MAX_RECENT_FILES; i--) {
                mRecentFiles.remove(i);
            }
            cleanMenu();
        }

        if( !added ) {
            LoadRecentProject lrp = new LoadRecentProject(displayName, fullPath);
            mRecentFiles.add(0, lrp);
        }

        updateMenu();
        saveCurrent();
    }


    /**
     * update the menu to reflect the recently opened files
     */
    private void updateMenu (){
        int index = mIndex;
        int i = 1;
        if( mCntAdded != mRecentFiles.size() ) {
            cleanMenu();

            // add our menu items
            Iterator iter = mRecentFiles.iterator();
            while (iter.hasNext()) {
                LoadRecentProject item = (LoadRecentProject)iter.next();
                item.setIndex(i++);
                mnuParent.insert(item, index);
                index++;
                mCntAdded++;
            }
        }

    }

    /**
     * removes all of the recently opened file items from the menu
     */
    private void cleanMenu (){
        for (int j=mIndex; j < mIndex + mCntAdded; j++ ) {
            mnuParent.remove(mIndex);
        }
        mCntAdded = 0;
    }

	public static void reset(){
		mRecentFileManager = null;
	}


}
