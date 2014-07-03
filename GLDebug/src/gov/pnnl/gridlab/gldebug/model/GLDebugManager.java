package gov.pnnl.gridlab.gldebug.model;

import java.awt.Frame;

public class GLDebugManager {

	private static GLDebugManager instance;
    private GLProjectSettings projectSettings;
    private GLDebugSession session;
    private Frame mainFrame;
    
    private GLDebugManager() {
		session = new GLDebugSession();
	}
    
    public static GLDebugManager instance() {
		if(instance == null){
			instance= new GLDebugManager();
		}
		return instance;
	}
    
    
	public GLProjectSettings getProjectSettings() {
		return projectSettings;
	}
	public GLDebugSession getSession() {
	
		return session;
	}
	
	public boolean hasProject() {
		return projectSettings != null;
	}
	
	/**
	 * create a new project
	 * @param filePath
	 */
	public void setNewProject(String filePath) {
		projectSettings = new GLProjectSettings();
		projectSettings.setFileName(filePath);
	}

	/**
	 * open a project reading from the file path
	 * @param filePath
	 */
	public void openProject(String filePath) throws Exception {
		projectSettings = GLProjectSettings.load(filePath);
	}

	/**
	 * clear the project settings
	 */
	public void clearProject() {
		projectSettings = null;
	}
	
	/**
	 * saves the currently active project
	 */
	public void saveProject() {
    	projectSettings.save(projectSettings.getFileName());
	}
	/**
	 * do a save as
	 * @param absolutePath
	 */
	public void saveProjectAs(String absolutePath) {
		projectSettings.setFileName(absolutePath);
		projectSettings.save(projectSettings.getFileName());
	}
	
	/**
	 * reload the current project
	 */
	public void reloadProject() {
		if(session.isGridlabRunning()){
			session.postKill();
		}
				
		session.setConfig(projectSettings);
		session.loadSimulation();
	}

	public Frame getMainFrame() {
		return mainFrame;
	}

	public void setMainFrame(Frame mainFrame) {
		this.mainFrame = mainFrame;
	}
	
	
}
