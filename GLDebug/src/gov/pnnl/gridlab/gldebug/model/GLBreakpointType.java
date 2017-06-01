package gov.pnnl.gridlab.gldebug.model;

/**
 * GLBreakpointType defines the types of breakpoints available for the application 
 * to use.
 */
public enum GLBreakpointType {
	CLOCK, ERROR, OBJECT, MODULE, CLASS, PASS, RANK, TIME;
	
	public String toString() {
		switch(this){
			case CLOCK:
				return "Clock";
			case ERROR:
				return "Error";
			case OBJECT:
				return "Object";
			case MODULE:
				return "Module";
			case CLASS:
				return "Class";
			case PASS:
				return "Pass";
			case RANK:
				return "Rank";
			case TIME:
				return "Time";
		}
		return "";
	}
	
	public static GLBreakpointType getTypeFromName(String name){
		if( name.equalsIgnoreCase("CLOCK")){
			return CLOCK;
		}
		else if( name.equalsIgnoreCase("ERROR")){
			return ERROR;
		}
		else if( name.equalsIgnoreCase("OBJECT")){
			return OBJECT;
		}
		else if( name.equalsIgnoreCase("MODULE")){
			return MODULE;
		}
		else if( name.equalsIgnoreCase("CLASS")){
			return CLASS;
		}
		else if( name.equalsIgnoreCase("PASS")){
			return PASS;
		}
		else if( name.equalsIgnoreCase("RANK")){
			return RANK;
		}
		else if( name.equalsIgnoreCase("TIME")){
			return TIME;
		}
		return null;
	}
}
