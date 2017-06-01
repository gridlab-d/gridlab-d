package gov.pnnl.gridlab.gldebug.model;

/**
 * holds the gridlab simulation status ("where")
 */
public class SimulationStatus {
	private String globalClock="";
	private String objectClock="";
	private String object="";
	private String pass="";
	private int rank;
	private int hardEvents;
	private String syncStatus="";
	private String stepToTime="";
	
	public String getGlobalClock() {
		return globalClock;
	}
	public void setGlobalClock(String globalClock) {
		this.globalClock = globalClock;
	}
	public String getObject() {
		return object;
	}
	public void setObject(String object) {
		this.object = object;
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
	public int getHardEvents() {
		return hardEvents;
	}
	public void setHardEvents(int hardEvents) {
		this.hardEvents = hardEvents;
	}
	public String getSyncStatus() {
		return syncStatus;
	}
	public void setSyncStatus(String syncStatus) {
		this.syncStatus = syncStatus;
	}
	public String getStepToTime() {
		return stepToTime;
	}
	public void setStepToTime(String stepToTime) {
		this.stepToTime = stepToTime;
	}
	public String getObjectClock() {
		return objectClock;
	}
	public void setObjectClock(String objectClock) {
		this.objectClock = objectClock;
	}

}
