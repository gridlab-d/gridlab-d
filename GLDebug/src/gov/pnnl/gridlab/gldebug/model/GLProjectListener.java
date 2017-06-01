/**
 *  GLDebug
 * <p>Title: Handles notifications of changes in project.
 * </p>
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
package gov.pnnl.gridlab.gldebug.model;

public interface GLProjectListener {
	
	public enum ProjectState  {PROJECT_OPENED, NEW_PROJECT, 
		PROJECT_CLOSED, PROJECT_SAVED, PROJECT_SETTINGS_CHANGE};
	
	void notifyProjectChanged(GLProjectSettings proj, ProjectState state);

}
