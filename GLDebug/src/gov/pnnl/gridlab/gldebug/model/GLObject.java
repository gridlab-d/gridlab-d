package gov.pnnl.gridlab.gldebug.model;

import java.util.ArrayList;
import java.util.List;

/**
 * this class holds information about a single gridlab object.
 * 
 * 
 */

public class GLObject {
	public enum ServiceStatus { NONE, PLANNED, ACTIVE, RETIRED };
	public enum SyncStatus { NONE, PRE, POST };  // corresponds to -, t, T
	
	public static final GLObject ROOT_OBJECT = new GLObject("ROOT");
	
	private ServiceStatus serviceStatus = ServiceStatus.NONE;
	
	private SyncStatus preTopDownSyncStatus = SyncStatus.NONE;
	private SyncStatus bottomUpSyncStatus = SyncStatus.NONE;
	private SyncStatus postTopDownSyncStatus = SyncStatus.NONE;
	
	private boolean locked = false;
	private boolean hasPLC = false;
	
	private int rank = 0;
	private String clock = "";
	private String name = "";
	private String parentName = "";
//	private String GLObjectProperties = null;
	
	public GLObject(){
		
	}
	
	public String toString() {
		return name + " " + buildStatusString();
	}
	
	private GLObject(String name){
		this.name = name;
	}
	
	/**
	 * builds the short form of the status string
	 * @return
	 */
	private String buildStatusString() {
		StringBuilder sb = new StringBuilder();
		switch(serviceStatus){
			case NONE:
				sb.append("-");
				break;
			case ACTIVE:
				sb.append("A");
				break;
			case PLANNED:
				sb.append("P");
				break;
			case RETIRED:
				sb.append("R");
				break;
		}
		sb.append(getSyncStatusStringId(preTopDownSyncStatus));
		sb.append(getSyncStatusStringId(bottomUpSyncStatus));
		sb.append(getSyncStatusStringId(postTopDownSyncStatus));
		sb.append(locked ? "1" : "-");
		sb.append(hasPLC ? "x" : "-");
		return sb.toString();
	}
	private String getSyncStatusStringId(SyncStatus status) {
		switch(status){
		case NONE:
			return "-";
		case PRE:
			return "t";
		case POST:
			return "T";
		}
		return "-";
	}
	
	//	output_message("%1s%1s%1s%1s%1s%1s %4d %-24.24s %-16.16s %-16.16s", 
	//	global_clock<obj->in_svc?"P":(global_clock<obj->out_svc?"A":"R"), /* service status: P=planned, A=active, R=retired */
	//	(obj->oclass->passconfig&PC_PRETOPDOWN)?(pass<PC_PRETOPDOWN?"t":"T"):"-", /* pre-topdown sync status */
	//	(obj->oclass->passconfig&PC_BOTTOMUP)?(pass<PC_BOTTOMUP?"b":"B"):"-", /* bottom-up sync status */
	//	(obj->oclass->passconfig&PC_POSTTOPDOWN)?(pass<PC_POSTTOPDOWN?"t":"T"):"-", /* post-topdown sync status */
	//	(obj->flags&OF_LOCKED)==OF_LOCKED?"l":"-", /* object lock status */
	//	(obj->flags&OF_HASPLC)==OF_HASPLC?"x":"-", /* object PLC status */
	//	obj->rank, 
	//	obj->clock>0?(convert_from_timestamp(obj->clock,buf3,sizeof(buf3))?buf3:"(error)"):"INIT",
	//	obj->name?obj->name:convert_from_object(buf1,sizeof(buf1),&obj,NULL)?buf1:"(error)",
	//	obj->parent?(obj->parent->name?obj->parent->name:convert_from_object(buf2,sizeof(buf2),&(obj->parent),NULL)?buf2:"(error)"):"ROOT");

	/**
	 * create a GLObject from the output of the "list" command.
	 * 
	 * The code the produced this message
	 *	 output_message("%1s%1s%1s%1s%1s%1s %4d %-24.24s %-16.16s %-16.16s",
	 *  example: 
	 *   "ATbt--   10 2000-01-30 07:14:48 EST  Node1            ROOT"
	 */
	public static GLObject createFromOutput(String msg) {
		if( msg.length() < 55) return null;

		GLObject obj = new GLObject();
		
		char c = msg.charAt(0);
		switch (c) {
			case 'A':
				obj.serviceStatus = ServiceStatus.ACTIVE;
				break;
			case 'P':
				obj.serviceStatus = ServiceStatus.PLANNED;
				break;
			case 'R':
				obj.serviceStatus = ServiceStatus.RETIRED;
				break;
		}
		
		obj.preTopDownSyncStatus = determineSyncStatus(msg.charAt(1));
		obj.bottomUpSyncStatus = determineSyncStatus(msg.charAt(2));
		obj.postTopDownSyncStatus = determineSyncStatus(msg.charAt(3));
		
		obj.locked = (msg.charAt(4) == '1');
		obj.hasPLC = (msg.charAt(5) == 'x');
		
		String s = msg.substring(7, 11);
		obj.rank = Integer.parseInt(s.trim());
		
		obj.clock = msg.substring(12, 36).trim();
		obj.name = msg.substring(37, 53).trim();
		obj.parentName = msg.substring(54).trim();
		
		return obj;
	}
	
	private static SyncStatus determineSyncStatus(char c) {
		if( Character.isUpperCase(c) ) {
			return SyncStatus.POST;
		}
		else if (Character.isLowerCase(c)) {
			return SyncStatus.PRE;
		}
		else {
			return SyncStatus.NONE;
		}
	}

	public ServiceStatus getServiceStatus() {
		return serviceStatus;
	}

	public SyncStatus getPreTopDownSyncStatus() {
		return preTopDownSyncStatus;
	}

	public SyncStatus getBottomUpSyncStatus() {
		return bottomUpSyncStatus;
	}

	public SyncStatus getPostTopDownSyncStatus() {
		return postTopDownSyncStatus;
	}

	public boolean isLocked() {
		return locked;
	}

	public boolean isHasPLC() {
		return hasPLC;
	}

	public int getRank() {
		return rank;
	}

	public String getClock() {
		return clock;
	}

	public String getName() {
		return name;
	}

	public String getParentName() {
		return parentName;
	}

	public void setServiceStatus(ServiceStatus serviceStatus) {
		this.serviceStatus = serviceStatus;
	}

	public void setPreTopDownSyncStatus(SyncStatus preTopDownSyncStatus) {
		this.preTopDownSyncStatus = preTopDownSyncStatus;
	}

	public void setBottomUpSyncStatus(SyncStatus bottomUpSyncStatus) {
		this.bottomUpSyncStatus = bottomUpSyncStatus;
	}

	public void setPostTopDownSyncStatus(SyncStatus postTopDownSyncStatus) {
		this.postTopDownSyncStatus = postTopDownSyncStatus;
	}

	public void setLocked(boolean locked) {
		this.locked = locked;
	}

	public void setHasPLC(boolean hasPLC) {
		this.hasPLC = hasPLC;
	}

	public void setRank(int rank) {
		this.rank = rank;
	}

	public void setClock(String clock) {
		this.clock = clock;
	}

	public void setName(String name) {
		this.name = name;
	}

	public void setParentName(String parentName) {
		this.parentName = parentName;
	}

}
