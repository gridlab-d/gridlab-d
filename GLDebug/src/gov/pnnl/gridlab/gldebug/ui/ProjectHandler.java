package gov.pnnl.gridlab.gldebug.ui;

import gov.pnnl.gridlab.gldebug.model.GLDebugManager;
import gov.pnnl.gridlab.gldebug.model.GLProjectListener;
import gov.pnnl.gridlab.gldebug.model.GLProjectSettings;
import gov.pnnl.gridlab.gldebug.model.GLProjectListener.ProjectState;
import gov.pnnl.gridlab.gldebug.ui.util.FileChooserEx;
import gov.pnnl.gridlab.gldebug.ui.util.RecentFileManager;
import gov.pnnl.gridlab.gldebug.ui.util.Utils;

import java.awt.event.ActionEvent;
import java.io.File;
import java.util.*;

import javax.swing.JOptionPane;

/**
 * handles user interactions with the project
 * 
 */

public class ProjectHandler {
	
	
	public static List<GLProjectListener> listeners = new ArrayList<GLProjectListener>();
	
	public static void addListener(GLProjectListener l){
		listeners.add(l);
	}

	public static void removeListener(GLProjectListener l){
		listeners.remove(l);
	}
	
	private static void notifyProjectEvent(ProjectState state) {
		for (GLProjectListener listener : listeners) {
			listener.notifyProjectChanged(GLDebugManager.instance().getProjectSettings(), state);
		}
	}
	
	private ProjectHandler(){
	}
	
	public static void openProject() {
		GLDebugManager manager = GLDebugManager.instance();
		
		// close currently open project
		if (!closeProject() ) {
			return;
		}
		
        FileChooserEx chooser = FileChooserEx.fetchFileChooserEx();
        chooser.addFilter( new String[]{"gld"}, "GLDebug Project (*.gld)" );

      int retVal = chooser.showOpenDialog(manager.getMainFrame());
      if (chooser.getSelectedFile() != null && retVal == FileChooserEx.APPROVE_OPTION ) {
            String name = FileChooserEx.ensureFileExtension(chooser.getSelectedFile().getAbsolutePath(), ".gld", false);
    	    File file = new File(name);
    	    if(file.exists()) {
    	    	try {
    	    		manager.openProject(file.getAbsolutePath());
					RecentFileManager.getSingleton().activeProjectChange(manager.getProjectSettings());
					notifyProjectEvent(ProjectState.PROJECT_OPENED);
					
				} catch (Exception e1) {
					JOptionPane.showMessageDialog(manager.getMainFrame(), "Unable to open that project.");
					e1.printStackTrace();
					manager.clearProject();
				}
    	    }
      }
	}

	/**
	 * opens the project (with as little user interaction as possible)
	 */
	public static void openProject(String fileName) {
		GLDebugManager manager = GLDebugManager.instance();

		// close currently open project
		if (!closeProject() ) {
			return;
		}

	    File file = new File(fileName);
	    if(file.exists()) {
	    	try {
	    		manager.openProject(file.getAbsolutePath());
				RecentFileManager.getSingleton().activeProjectChange(manager.getProjectSettings());
				notifyProjectEvent(ProjectState.PROJECT_OPENED);
				
			} catch (Exception e1) {
				JOptionPane.showMessageDialog(manager.getMainFrame(), "Unable to open that project.");
				e1.printStackTrace();
				manager.clearProject();
			}
	    }

	}
	
	
	public static void saveProject() {
		GLDebugManager manager = GLDebugManager.instance();
		if( manager.getProjectSettings() != null){
	    	manager.saveProject();
			RecentFileManager.getSingleton().activeProjectChange(manager.getProjectSettings());
			notifyProjectEvent(ProjectState.PROJECT_SAVED);
		}
		
	}

	public static void saveProjectAs() {
		GLDebugManager manager = GLDebugManager.instance();
        FileChooserEx chooser = FileChooserEx.fetchFileChooserEx();
        chooser.addFilter( new String[]{"gld"}, "GLDebug Project (*.gld)" );
        chooser.setCurrentDirectory(new File(manager.getProjectSettings().getFileName()));

      int retVal = chooser.showSaveDialog(manager.getMainFrame());
      if (chooser.getSelectedFile() != null && retVal == FileChooserEx.APPROVE_OPTION ) {
			String name = FileChooserEx.ensureFileExtension(chooser.getSelectedFile().getAbsolutePath(), ".gld", false);
			File file = new File(name);
    	    if(file.exists()) {
    	    	int res = JOptionPane.showConfirmDialog(manager.getMainFrame(), "File " + file.getName() + " exists.  Overwrite?", "Overwrite", JOptionPane.YES_NO_CANCEL_OPTION);
    	    	if( res != JOptionPane.YES_OPTION) {
    	    		return;
    	    	}
    	    }
    	    
    	    manager.saveProjectAs(file.getAbsolutePath());
			RecentFileManager.getSingleton().activeProjectChange(manager.getProjectSettings());

			notifyProjectEvent(ProjectState.PROJECT_SAVED);
      }
		
	}

	/**
	 * closes the active project, if any.
	 * @return false if the user cancels
	 */
	public static boolean closeProject() {
		GLDebugManager manager = GLDebugManager.instance();

		// prompt to save first
		GLProjectSettings proj = manager.getProjectSettings();
		if( proj != null && proj.isDirty()){
	    	int res = JOptionPane.showConfirmDialog(manager.getMainFrame(), "Save project changes?", "Save Changes", JOptionPane.YES_NO_CANCEL_OPTION);
	    	if( res == JOptionPane.YES_OPTION) {
				saveProject();
	    	}
		}

		// if gridlab is running, prompt to close it
		if( manager.getSession().isGridlabRunning() ){
	    	int res = JOptionPane.showConfirmDialog(manager.getMainFrame(), "End current debug session?", "End Simulation", JOptionPane.YES_NO_CANCEL_OPTION);
	    	if( res != JOptionPane.YES_OPTION) {
	    		return false;
	    	}
        	manager.getSession().postKill();
        }
		
		// release the project
        manager.clearProject();
        
        notifyProjectEvent(ProjectState.PROJECT_CLOSED);
        return true;
	}

	/**
	 * creates a new project.
	 */
	public static void newProject() {
		GLDebugManager manager = GLDebugManager.instance();
		
		// close existing if running
		if( !closeProject() ){
			return;
		}

    	FileChooserEx chooser = FileChooserEx.fetchFileChooserEx();
        chooser.addFilter( new String[]{"gld"}, "GLDebug Project (*.gld)" );
        chooser.setDialogTitle("New GLDebug Project");
        chooser.setApproveButtonText("Create Project");
        chooser.setApproveButtonToolTipText("Create a new GLDebug Project");
        int retVal = chooser.showOpenDialog(manager.getMainFrame());
       if (chooser.getSelectedFile() != null && retVal == FileChooserEx.APPROVE_OPTION ) {
			String name = FileChooserEx.ensureFileExtension(chooser.getSelectedFile().getAbsolutePath(), ".gld", false);
			File file = new File(name);
			if( file.exists() ) {
				int opt = JOptionPane.showConfirmDialog(manager.getMainFrame(), "That files already exists.  Overwrite Existing file?)", "Overwrite",
						                            JOptionPane.YES_NO_OPTION);
				if( opt == JOptionPane.NO_OPTION) {
					return;
				}
			}

			manager.setNewProject(file.getAbsolutePath());
			RecentFileManager.getSingleton().activeProjectChange(manager.getProjectSettings());
			notifyProjectEvent(ProjectState.NEW_PROJECT);
      }
		
	}

	public static void editProjectSettings() {
		GLDebugManager manager = GLDebugManager.instance();
    	ProjectSettingsDlg dlg = new ProjectSettingsDlg(manager.getMainFrame());
    	dlg.setGLProjectSettings(manager.getProjectSettings());
    	Utils.centerOver(dlg, manager.getMainFrame());
    	dlg.setVisible(true);
    	if( !dlg.isCanceled()) {
    		if(!manager.getProjectSettings().isOnDisk()){
	    		// save it since it has not been saved yet
		    	ProjectHandler.saveProject();
    		}
	    	// notify things have changed
    		notifyProjectEvent(ProjectState.PROJECT_SETTINGS_CHANGE);
    	}
	}
	

}
