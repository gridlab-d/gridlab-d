package gov.pnnl.gridlab.gldebug.model;

public enum GLStepType {
	OBJECT, RANK, PASS, ITERATION, CLOCK;
	
	public static GLStepType[] getTypes() {
		return new GLStepType[] {OBJECT, RANK, PASS, ITERATION, CLOCK};
	}

	public String toString() {
		switch(this){
			case OBJECT:
				return "Object";
			case RANK:
				return "Rank";
			case PASS:
				return "Pass";
			case ITERATION:
				return "Iteration";
			case CLOCK:
				return "Clock";
		}
		return "";
	}
}
