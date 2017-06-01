package gov.pnnl.gridlab.gldebug.model;

/**
 * GLCommand - this class helps keep track of the various commands that are available 
 *    to run using GridLab.
 */
import java.util.EnumMap;

public class GLCommand {
    public enum GLCommandName { RUN, LOAD, CONTEXT, LIST, PRINTCURR, PRINTOBJ, 
    	                       NEXT, GLOBALS_LIST,
    	                       BREAK_LIST, BREAK_CLOCK, BREAK_ERROR, BREAK_ITEM, 
    	                       BREAK_DISABLE, BREAK_DISABLE_ALL,
    	                       BREAK_ENABLE, BREAK_ENABLE_ALL,
    	                       WATCH_LIST, WATCH_PRINT};
    private static EnumMap<GLCommandName, String> cmdMap = new EnumMap<GLCommandName, String>(GLCommandName.class);

    private GLCommandName name;
    private String args;
    private Object output;
    
    static {
    	cmdMap.put(GLCommandName.RUN, "run");
    	cmdMap.put(GLCommandName.LOAD, "");
    	cmdMap.put(GLCommandName.CONTEXT, "where");
    	cmdMap.put(GLCommandName.LIST, "list");
    	cmdMap.put(GLCommandName.PRINTCURR, "print");
    	cmdMap.put(GLCommandName.PRINTOBJ, "print %s");
    	cmdMap.put(GLCommandName.NEXT, "next");
    	
    	cmdMap.put(GLCommandName.BREAK_LIST, "break");
    	cmdMap.put(GLCommandName.BREAK_CLOCK, "break clock");
    	cmdMap.put(GLCommandName.BREAK_ERROR, "break error");
    	cmdMap.put(GLCommandName.BREAK_ITEM, "break %s %s");
    	cmdMap.put(GLCommandName.BREAK_DISABLE, "break disable %s");
    	cmdMap.put(GLCommandName.BREAK_DISABLE_ALL, "break disable");
    	cmdMap.put(GLCommandName.BREAK_ENABLE_ALL, "break enable");
    	cmdMap.put(GLCommandName.BREAK_ENABLE, "break enable %s");
    	
    	cmdMap.put(GLCommandName.WATCH_LIST, "watch");
    	cmdMap.put(GLCommandName.WATCH_PRINT, "watch %s");
    	
    	cmdMap.put(GLCommandName.GLOBALS_LIST, "globals");
    }

    public GLCommand(GLCommandName name) {
		this.name = name;
		this.args = null;
	}
    
    public GLCommand(GLCommandName name, String args) {
		this.name = name;
		this.args = args;
	}

	public GLCommandName getCmdName() {
		return name;
	}

	public String getArgs() {
		return args;
	}
	
	public String getFullCommand() {
		String cmd = cmdMap.get(name);
		if( args != null ){
			cmd = String.format(cmd, args);
		}
		return cmd;
	}

	public Object getOutput() {
		return output;
	}

	public void setOutput(Object output) {
		this.output = output;
	}


}
