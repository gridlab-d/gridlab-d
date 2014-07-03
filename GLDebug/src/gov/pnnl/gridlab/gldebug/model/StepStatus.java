package gov.pnnl.gridlab.gldebug.model;

public class StepStatus {
	String objectName;
	String globalClock;
	String pass;
	int rank;
	int iteration;
	/**
	 * should the GUI update the currently selected object in the tree and its details?
	 */
	boolean updateFocus = true;
	
	
	public String getObjectName() {
		return objectName;
	}
	public void setObjectName(String objectName) {
		this.objectName = objectName;
	}
	public String getGlobalClock() {
		return globalClock;
	}
	public void setGlobalClock(String clock) {
		this.globalClock = clock;
	}
	public String getPass() {
		return pass;
	}
	public void setPass(String pass) {
		this.pass = pass;
	}
	public int getRank() {
		return rank;
	}
	public void setRank(int rank) {
		this.rank = rank;
	}
	public int getIteration() {
		return iteration;
	}
	public void setIteration(int iteration) {
		this.iteration = iteration;
	}
	public boolean isUpdateFocus() {
		return updateFocus;
	}
	public void setUpdateFocus(boolean updateFocus) {
		this.updateFocus = updateFocus;
	}
	
	
}
