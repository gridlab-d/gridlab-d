/**
 *  GLDebug
 * <p>Title: Handles notifications of changes in the run of the application.
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

import gov.pnnl.gridlab.util.ExecutionEvent;

/**
 *
 */
public interface GLListener {
	
	void notifyClockChange(String val);
	void notifyEvent(ExecutionEvent evt);
	void notifyStatusChanged(GLStatus status, GLCommand cmd);
	
	/**
	 * notifies that the command has completed.
	 * Results (if applicable) are available in the GLCommand object.
	 */
	void notifyGLCommandResults(GLCommand cmd);

}
