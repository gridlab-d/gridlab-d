package gov.pnnl.gridlab.gldebug.model;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.*;
import java.util.concurrent.ConcurrentLinkedQueue;

import javax.swing.SwingUtilities;

import gov.pnnl.gridlab.gldebug.model.GLCommand.GLCommandName;
import gov.pnnl.gridlab.gldebug.ui.util.UserPreferences;
import gov.pnnl.gridlab.util.*;

/**
 * <p>
 * Title: GridLab GUI
 * </p>
 * 
 * <p>
 * Description:
 * </p>
 * 
 * <p>
 * Copyright: Copyright (c) 2008
 * </p>
 * 
 * <p>
 * Company:
 * </p>
 * 
 * @author Jon McCall
 * @version 1.0
 */
public class GLDebugSession implements ExecutionListener {

	// public enum GLCommand { RUN, LOAD, CONTEXT, LIST, PRINT };

	private String glExePath;
	private GLStatus status = GLStatus.NONE;
	private ExecutionCommand mExecutionCommand;
	private ConcurrentLinkedQueue<ExecutionEvent> mQueue = new ConcurrentLinkedQueue<ExecutionEvent>();
	private GLProjectSettings config;
	private boolean atPrompt = false;
	private StringBuffer errorMsgBuffer;
	private List<GLListener> listeners = new ArrayList<GLListener>();
	private GLCommand executingCommand = null;
	private List<GLCommand> commandQueue = Collections
			.synchronizedList(new LinkedList<GLCommand>());
	private GLCommand lastCommand = null;
	private int cmdIndex = 0;
	
	// variable for keeping track of the step command (NEXT)
	private StepStatus lastStepStatus = null;
	private StepStatus simStepStatus = null;
	private StepStatus startingStepStatus = null;
	private GLStepType currentStepType = null;

	private SimulationStatus simStatus = null;
	private List<GLObject> glObjectList = new ArrayList<GLObject>();
	private GLObjectProperties objectProps;
	private GLGlobalList globalProps;

	// private static EnumMap<GLCommand, String> cmdMap = new EnumMap<GLCommand,
	// String>(GLCommand.class);
	//    
	// static {
	// cmdMap.put(GLCommand.RUN, "run");
	// cmdMap.put(GLCommand.LOAD, "");
	// cmdMap.put(GLCommand.CONTEXT, "where");
	// cmdMap.put(GLCommand.LIST, "list");
	// cmdMap.put(GLCommand.PRINT, "print");
	// }

	/**
	 * is the executable running?
	 */
	private boolean gridlabRunning = false;

	public GLDebugSession() {
		glExePath = UserPreferences.instanceOf().getValue("Main", "GLEXE", "");

	}

	public void addListener(GLListener l) {
		listeners.add(l);
	}

	public void removeListener(GLListener l) {
		listeners.remove(l);
	}

	public GLStatus getStatus() {
		return status;
	}

	/**
	 * notifies that this command has finished and starts the next command if
	 * available
	 * 
	 * @param cmd
	 */
	public synchronized void notifyCommandFinished(GLCommand cmd) {
		executingCommand = null;
		setStatus(GLStatus.STOPPED, cmd);
		startNextCommand();
	}

	public void setStatus(GLStatus status, GLCommand cmd) {
		this.status = status;
		postStatusChanged(cmd);
	}

	/**
	 * launches the gridlabd.exe with the project parameters given. Gridlab will
	 * launch, check the parameters given, load the models, and then wait for
	 * further input.
	 */
	public boolean loadSimulation() {
		mQueue.clear();
		errorMsgBuffer = new StringBuffer();
		gridlabRunning = true;

		config.setGridLabExe(glExePath);
		if (!config.areRequiredParamsSet()) {
			return false;
		}

		executingCommand = new GLCommand(GLCommandName.LOAD);
		String cmd = config.buildCommandLine();

		System.out.println("Executing command: " + cmd);
		System.out.println("   in directory: " + config.getWorkingDir());
		System.out.println("   with environment: ");
		String[] env = config.getEnv();
		for (String item : env) {
			System.out.println("      " + item);
		}

		mExecutionCommand = new ExecutionCommand(cmd, env, new File(config.getWorkingDir()));
		mExecutionCommand.addExecutionListener(this);
		mExecutionCommand.start();

		// ContinuousFileReader cfr = new ContinuousFileReader(this,
		// config.getWorkingDir() + File.separator + "progress.log",
		// ExecutionEvent.STDOUT);

		// // we will signal stopped after the load finishes...
		// setStatus(GLStatus.STOPPED, null);
		return true;
	}

	/**
	 * check for and retrieve any messages the program has returned
	 * 
	 * @return a ExecutionEvent, null if no messages are in the queue
	 */
	public ExecutionEvent getExecutionMsg() {
		return mQueue.poll();
	}

	/**
	 * stop the current command from running, destroys the Gridlab process.
	 */
	public void stop() {
		mExecutionCommand.haltCommand();
	}

	public GLProjectSettings getConfig() {
		return config;
	}

	public void setConfig(GLProjectSettings config) {
		this.config = config;
	}

	public void postOuput(String msg) {
		if (mExecutionCommand != null) {
			System.out.println("Executing cmd: " + msg);
			mExecutionCommand.postOutputMessage(msg);
		} else {
			System.err.println("postOutput (" + msg
					+ ") called without gridlabd running.");
		}
	}

	/**
	 * send the SIGINT signal to the process to tell it to stop
	 */
	public void postBreak() {
		String cmdKill = config.getBreakCmd();
		if (cmdKill != null) {
			System.out.println("Sending SIGINT to process.  " + cmdKill);
			ExecutionCommand exec = new ExecutionCommand(cmdKill, config
					.getEnv(), new File(config.getWorkingDir()));
			exec.start();
		} else {
			System.err.println("Unable to obtain break cmd.");
		}
	}

	/**
	 * send the SIGTERM signal to the process to tell it to exit
	 */
	public void postKill() {
		if (mExecutionCommand == null) {
			return;
		}
		String cmdKill = config.getKillCmd();
		if (cmdKill != null) {
			// stop listening to this running process
			mExecutionCommand.removeExecutionListener(this);
			mExecutionCommand = null;
			gridlabRunning = false;
			handleProcessFinished();

			// then kill it
			System.out.println("Sending SIGTERM to process.  " + cmdKill);
			ExecutionCommand exec = new ExecutionCommand(cmdKill, config
					.getEnv(), new File(config.getWorkingDir()));
			exec.start();

		} else {
			System.err.println("Unable to obtain kill cmd.");
		}
	}

	// public void run() {
	// postOuput("run");
	// status = GLStatus.RUNNING;
	// postStatusChanged();
	// }

	/**
	 * lets you queue a debug command to execute as soon as it can.
	 * 
	 * @param cmd
	 */
	public synchronized void queueCommand(GLCommand cmd) {
		if (executingCommand == null) {
			executeCommand(cmd);
		} else {
			// que the command for later
			commandQueue.add(cmd);
		}
	}

	private synchronized void startNextCommand() {
		if (executingCommand == null && commandQueue.size() > 0 ) {
			GLCommand cmd = commandQueue.remove(0);
			executeCommand(cmd);
		}
	}

	/**
	 * lets you queue a debug command to execute as soon as it can.
	 * 
	 * @param cmd
	 */
	private synchronized void executeCommand(GLCommand cmd) {
		executingCommand = cmd;
		lastCommand = executingCommand;
		cmdIndex = 0;

		switch (cmd.getCmdName()) {
		case LIST:
			glObjectList.clear();
			break;
		case GLOBALS_LIST:
			globalProps = new GLGlobalList();
			break;
		case PRINTCURR:
		case PRINTOBJ:
			objectProps = new GLObjectProperties();
			break;
		}

		// notify client that we are doing something
		setStatus(GLStatus.RUNNING, cmd);

		// ask gridlab to do it
		postOuput(cmd.getFullCommand());
	}

	public GLCommand getLastCommand() {
		return lastCommand;
	}

	// implement interface ExecutionListener extends EventListener {
	public synchronized void executeEvent(ExecutionEvent evt) {
		String msg = evt.getMessage();
		System.out.println("ExecuteEvent: " + msg);
		if (msg.trim().compareTo("GLD>") == 0) {
			atPrompt = true;
		} else {
			atPrompt = false;
		}
		if (executingCommand != null) {
			if (processResponse(evt)) {
				return;
			}
		}

		if (!atPrompt) {
//			if (msg.contains("signal SIGINT caught")) {
//				notifyCommandFinished(executingCommand);
//			}

			switch (evt.getMessageType()) {
			case ExecutionEvent.STDOUT:
			case ExecutionEvent.STDERR:
			case ExecutionEvent.ERROR:
				postEvent(evt);
				break;
			case ExecutionEvent.FINISHED:
			case ExecutionEvent.HALTED:
				System.out.println("Process done. " + evt.getMessageType());
				gridlabRunning = false;
				postEvent(evt);
				setStatus(GLStatus.NONE, executingCommand);
				// stop listening
				mExecutionCommand.removeExecutionListener(this);
				mExecutionCommand = null;
				notifyCommandFinished(executingCommand);
				handleProcessFinished();
				break;
			}
		}

	}

	/**
	 * do whatever cleanup is requried after the process exits
	 */
	private void handleProcessFinished() {
		glObjectList.clear();
	}

	/**
	 * process the command
	 * 
	 * @param evt
	 * @return true if we handled it and the message should not pass to the GUI
	 */
	private boolean processResponse(ExecutionEvent evt) {
		String msg = evt.getMessage();
		//System.out.println("----" + msg);

		// TODO if Matt gives me everything back on one stream, then I can use
		// the atPrompt variable to tell that I'm done with a command...

		switch (executingCommand.getCmdName()) {
		case RUN: {
			// ignore processing messages "DEBUG: global_clock = '2000-09-27 04:05:42 EDT' (970041942)"
			if (msg.startsWith("DEBUG: global_clock = ")) {
				String val = msg.substring(msg.indexOf("'") + 1, msg
						.lastIndexOf("'"));
				postClockChange(val);
				return true;
			}
			boolean ret = populateStepStatus(msg);
			if( atPrompt ){
				notifyStepStatusChange();
				notifyCommandFinished(executingCommand);
			}
			return ret;
		}

		case LOAD:{
			boolean ret = populateStepStatus(msg);
			if (atPrompt) {
				notifyStepStatusChange();
				notifyCommandFinished(executingCommand);
			}

			return ret;
		}
		
		case GLOBALS_LIST:
			if(atPrompt){
				executingCommand.setOutput(globalProps);
				postCommandResults(executingCommand);
				notifyCommandFinished(executingCommand);
			}
			else {
				parseGlobalProps(globalProps, msg);
			}
			return true;
		
		case PRINTCURR:
		case PRINTOBJ:
			// check for the message that tells us the load has completed
			if (atPrompt) {
				executingCommand.setOutput(objectProps);
				postCommandResults(executingCommand);
				notifyCommandFinished(executingCommand);
			} else {
				parseObjectProps(objectProps, msg);
			}
			return true;
		case CONTEXT:
			if (simStatus == null) {
				simStatus = new SimulationStatus();
			}
			// DEBUG: Global clock... 2000-06-11 03:51:13 EDT (960709873)
			// DEBUG: Hard events.... 0
			// DEBUG: Sync status.... SUCCESS
			// DEBUG: Step to time... TS_NEVER (9223372036854775807)
			// DEBUG: Pass........... 0
			// DEBUG: Rank........... 11
			// DEBUG: Object......... Node1
			if (msg.startsWith("DEBUG: Global clock")) {
				simStatus.setGlobalClock(parseValue(msg));
			} else if (msg.startsWith("DEBUG: Hard events")) {
				int hardEvents = Integer.parseInt(parseValue(msg));
				simStatus.setHardEvents(hardEvents);
			} else if (msg.startsWith("DEBUG: Sync status")) {
				simStatus.setSyncStatus(parseValue(msg));
			} else if (msg.startsWith("DEBUG: Step to time")) {
				simStatus.setStepToTime(parseValue(msg));
			} else if (msg.startsWith("DEBUG: Pass")) {
				simStatus.setPass(parseValue(msg));
			} else if (msg.startsWith("DEBUG: Rank")) {
				int n = Integer.parseInt(parseValue(msg));
				simStatus.setRank(n);
			} else if (msg.startsWith("DEBUG: Object")) {
				simStatus.setObject(parseValue(msg));
			} else if (atPrompt) {
				// finished processing output, notify we are done
				executingCommand.setOutput(simStatus);
				postCommandResults(executingCommand);
				simStatus = null;
				notifyCommandFinished(executingCommand);
			} else {
				// unhandled...
				return false;
			}
			return true;			
		case NEXT: {
			boolean ret = populateStepStatus(msg);
			if (atPrompt) {
				// finished processing output, notify we are done
				boolean finished = notifyStepStatusChange();
				notifyCommandFinished(executingCommand);
				if(!finished){
					queueCommand(new GLCommand(GLCommandName.NEXT));
				}
				else {
					queueCommand(new GLCommand(GLCommandName.LIST, "UPDATE"));
				}
			}
			
			return ret;
		}

		case LIST:
			// being back at the prompt notifies us that the command has
			// finished
			if (atPrompt) {
				postCommandResults(executingCommand);
				notifyCommandFinished(executingCommand);
			} else {
				// parse the message into a GLObject and save it
				GLObject obj = GLObject.createFromOutput(msg);
				if (obj != null) {
					glObjectList.add(obj);
				}
				return true;
			}
			break;

		}
		// unhandled
		return false;

	}
	
	/**
	 * notify that the status has changed.  This is a notification of current global clock,
	 * object, and its rank/pass/iteration.
	 * 
	 * @param step
	 * @return true if the step has finished (made it to the next change)
	 */
	private boolean notifyStepStatusChange() {
		if( simStepStatus != null ){
			boolean ret = true;
			if(startingStepStatus != null){
				// we want to keep running the step command until our step by value has changed.
				// Check for a change in the step by value. Return true if it has changed.
				ret = false;
				switch (currentStepType) {
					case OBJECT:
						ret = true;
						break;
					case CLOCK:
						ret = (!startingStepStatus.getGlobalClock().equals(simStepStatus.getGlobalClock()));
						break;
					case ITERATION:
						ret = (startingStepStatus.getIteration() != simStepStatus.getIteration());
						break;
					case PASS:
						ret = (!startingStepStatus.getPass().equals(simStepStatus.getPass()));
						break;
					case RANK:
						ret = (startingStepStatus.getRank() != simStepStatus.getRank());
						break;
				}
			}
			simStepStatus.setUpdateFocus(ret);
			// notify
			executingCommand.setOutput(simStepStatus);
			postCommandResults(executingCommand);
			
			lastStepStatus = simStepStatus;
			simStepStatus = null;
			if(ret){
				currentStepType = null;
				startingStepStatus = null;
			}
			
			return ret;
		}
		return false;
	}

	/**
	 * populate a SimulationObjectStatus object based on messages received
	 * @param msg
	 * @return true if we picked up a message
	 */
	private boolean populateStepStatus(String msg) {
		//DEBUG: time 2000-01-01 00:00:00 PST
		//DEBUG: pass BOTTOMUP, rank 0, object player:17, iteration 1			
		if (msg.startsWith("DEBUG: time")) {
			if( simStepStatus == null) {
				simStepStatus = new StepStatus();
			}
			simStepStatus.setGlobalClock(msg.substring(11).trim());
			return true;
		} 
		else if (msg.startsWith("DEBUG: pass")) {
			if( simStepStatus == null) {
				simStepStatus = new StepStatus();
			}
			String vals[] = msg.substring(11).trim().split(",");
			simStepStatus.setPass(vals[0].trim());
			
			String rank[] = vals[1].trim().split(" ");
			simStepStatus.setRank(Integer.parseInt(rank[1]));

			String obj[] = vals[2].trim().split(" ");
			simStepStatus.setObjectName(obj[1].trim());
			
			String iter[] = vals[3].trim().split(" ");
			simStepStatus.setIteration(Integer.parseInt(iter[1]));
			return true;
		}
		else if (msg.startsWith("DEBUG: ") && msg.contains(" next sync ")) {
			// just ignore these messages
			//DEBUG: microwave:10 next sync TS_NEVER 
			return true;
		}
		else if( msg.trim().length() == 0){
			return true;
		}
		
		return false;
	}

	/**
	 * parse out the data from the PRINT command.
	 * 
	 * Looks something like:
    ... debug command 'where'
DEBUG: object waterheater:5 {
	parent = house:1 ()
	rank = 1;
 	clock = 2000-04-27 00:22:54 PDT (956820174);
 	flags = NONE;
 	double tank_volume = +6.68403 gal;
 	double tank_UA = +2.06596 Btu/h;
 	double tank_diameter = +1.5 ft;
 	double water_demand = +0 gpm;
 }
	 * 
	 * @param objectProps2
	 * @param msg
	 */
	private void parseObjectProps(GLObjectProperties props, String msg) {
		msg = msg.trim();
		int index = -1;
		if( msg.startsWith("DEBUG: object") ) {
			index = msg.indexOf("{");
			String val = msg.substring("DEBUG: object".length() + 1, index - 1);
			props.setObjectName(val.trim());
		}
		else if( msg.startsWith("...")) {
			// ignore verbose messages
		}
		else if((index = msg.indexOf("=")) > -1) {
			String var = msg.substring(0, index - 1).trim();
			String val = msg.substring(index + 1, msg.length()).trim();
			if( val.endsWith(";")) {
				val = val.substring(0, val.length() - 1);
			}
			String type = null;
			String sa[] = var.split(" ");
			if( sa.length > 1) {
				type = sa[0];
				var = sa[1];
			}
			
			props.addProperty(var, val, type);
		}
		else {
			// parse simple name/value pairs
			String s[] = msg.split(" ");
			if( s.length == 2 ){
				props.addProperty(s[0], s[1], null);
			}
		}
		
	}
	
	/**
	 * parse out the data from the GLOBALS command.
	 * 
	 * Looks something like:
version.major                   : "1"
version.minor                   : "3"
command_line                    : ""C:\source\GridLab\GridlabD\..."
environment                     : "batch"
verbose                         : "0"
iteration_limit                 : "100"
workdir                         : ""C:\Program Files\GridLAB-D\samples""
dumpfile                        : "gridlabd.xml"
savefile                        : ""C:\Program Files\GridLAB-D\testoutput.xml""
dumpall                         : "0"
kmlfile                         : """"
modelname                       : ""C:\Program Files\GridLAB-D\samples\residential_loads.glm""
execdir                         : "C:\source\GridLab\GridlabD\source\VS2005\Win32\Debug"
strictnames                     : "TRUE"
primary_voltage_ratio           : "+60"
nominal_frequency               : "+60"
powerflow::require_voltage_contr: "FALSE"
	 * 
	 * @param objectProps2
	 * @param msg
	 */
	private void parseGlobalProps(GLGlobalList props, String msg){
		msg = msg.trim();
		// find the first colon after position 30
		int index = msg.indexOf(":", 30);
		if(index > -1) {
			String var = msg.substring(0, index - 1).trim();
			String val = msg.substring(index + 1).trim();
			// remove surrounding quotes
			if( val.endsWith("\"") && val.startsWith("\"")) {
				val = val.substring(1, val.length() - 1);
			}
			
			props.addGlobal(var, val);
		}
	}

	/**
	 * parse out the data from a debug message. message looks like "DEBUG GLobal
	 * clock... 2000-06-11 03:51:13 EDT (960709873)" We return just the
	 * "2000-06-11 03:51:13 EDT (960709873)" part.
	 * 
	 * @param msg
	 * @return
	 */
	private String parseValue(String msg) {
		int index = msg.indexOf(".");
		int size = msg.length();
		while (index < size - 1 && index > -1) {
			index++;
			if (msg.charAt(index) == ' ') {
				return msg.substring(index + 1).trim();
			}
		}
		return null;
	}

	private void postClockChange(String msg) {
		for (GLListener listener : listeners) {
			listener.notifyClockChange(msg);
		}
	}

	private void postEvent(ExecutionEvent evt) {
		for (GLListener listener : listeners) {
			listener.notifyEvent(evt);
		}
	}

	private void postStatusChanged(GLCommand cmd) {
		for (GLListener listener : listeners) {
			listener.notifyStatusChanged(status, cmd);
		}
	}

	private void postCommandResults(GLCommand cmd) {
		for (GLListener listener : listeners) {
			listener.notifyGLCommandResults(cmd);
		}
	}

	public String getGlExePath() {
		return glExePath;
	}

	public void setGlExePath(String glExePath) {
		this.glExePath = glExePath;
		UserPreferences.instanceOf().setValue("Main", "GLEXE", glExePath);
	}

	/**
	 * Is the executable in memory?
	 * 
	 * @return the gridlabRunning
	 */
	public boolean isGridlabRunning() {
		return gridlabRunning;
	}

	/**
	 * gets the list of GLObjects organized in a tree
	 * @return
	 */
	public GLObjectTree getGLObjectTree() {
		if (glObjectList.size() > 0) {
			return GLObjectTree.createTree(glObjectList);
		}
		return null;
	}
	
	/**
	 * just get a straight list of objects
	 * @return
	 */
	public List<GLObject> getGLObjectList() {
		return Collections.unmodifiableList(glObjectList);
	}
	

	/**
	 * start a step operation...
	 */
	public void step(GLStepType selectedItem) {
		startingStepStatus = lastStepStatus;
		currentStepType = selectedItem;
		
		queueCommand(new GLCommand(GLCommandName.NEXT));
		//manager.getSession().queueCommand(new GLCommand(GLCommandName.PRINTOBJ, ((GLObject)obj).getName()));
		// TODO Auto-generated method stub
		
	}

	/**
	 * install the breakpoints we have into the application
	 */
	public void InstallBreakpoints() {
		
		GLBreakpointList bpList = config.getBreakPointList();

		// TODO Finish this
		
		for (int i = 0; i < bpList.size(); i++) {
			GLBreakpointList.Property prop = bpList.getBreak(i);
//			prop.type
//			queueCommand(new GLCommand(GLCommandName.))
		}
	
		
		
	}

}
