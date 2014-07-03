package gov.pnnl.gridlab.util;

import java.util.*;
import java.io.*;
import java.lang.*;

/**
 * A class that manages the running of an external process on a separate thread.
 * This permits the monitoring and controlling the of running long running processes
 * from a different thread.
 */
public final class ExecutionCommand extends Thread {

  public static final int DEFAULT_BUFFER_SIZE = 1024;

  private List<ExecutionListener> mListeners = new ArrayList<ExecutionListener>();

  private String[] mCmdArray;
  private String[] mEnvp;
  private File mDir;

  private boolean mStarted = false;
  private boolean mStdout_done = false;
  private boolean mStderr_done = false;
  private boolean mStop = false;
  private int mBufSize = DEFAULT_BUFFER_SIZE;

  private String mExecErrorMessage;
  private boolean mDone;
  private int mExitStatus = -1;
  private BufferedOutputStream mOutput;

  /**
   * Configures the object to execute the specified string command in a
   * separate process. The command argument is parsed into a String array of tokens
   * (cmdarray) using a StringTokenizer created by the call: new StringTokenizer(command)
   * with no further modifications of the character categories. This constructor
   * is equivalent to using the constructor <code>ExecutionCommand(cmdarray, null)</code>.
   * <p>
   * @param command - a specified system command.
   */
  public ExecutionCommand(String command) {
    this(command, null);
  }

  /**
   * Configures the object to execute the specified string command in a separate
   * process with the specified environment.
   * <p>
   * The command argument is parsed into a String array of tokens (cmdarray)
   * using a StringTokenizer created by the call: new StringTokenizer(command)
   * with no further modifications of the character categories. This constructor
   * is equivalent to using the constructor
   * <code>ExecutionCommand(cmdarray, envp, null)</code>.
   * <p>
   * The environment variable settings are specified by <tt>envp</tt>.
   * If <tt>envp</tt> is <tt>null</tt>, in the run method the subprocess inherits the
   * environment settings of the current process.
   *
   * @param command - a specified system command.
   * @param envp - array of strings, each element of which
   *             has environment variable settings in format
   *             <i>name</i>=<i>value</i>.
   */
  public ExecutionCommand(String command, String[] envp) {
    this(command, envp, null);
  }

  /**
   * Configures the object to execute the specified string command in a separate
   * process with the specified environment and working directory.
   * <p>
   * The command argument is parsed into a String array of tokens (cmdarray)
   * using a StringTokenizer created by the call: new StringTokenizer(command)
   * with no further modifications of the character categories. This constructor
   * is equivalent to using the constructor
   * <code>ExecutionCommand(cmdarray, envp, dir)</code>.
   * <p>
   * The environment variable settings are specified by <tt>envp</tt>.
   * If <tt>envp</tt> is <tt>null</tt>, in the run method the subprocess inherits the
   * environment settings of the current process.
   *
   * @param command - a specified system command.
   * @param envp - array of strings, each element of which
   *             has environment variable settings in format
   *             <i>name</i>=<i>value</i>.
   * @param dir - the working directory for the system command.
   */
  public ExecutionCommand(String command, String[] envp, File dir) {
    this(tokenizeCommand(command), envp, dir);
  }

  /**
   * Configures the object to execute the specified command and arguments in
   * a separate process.
   * <p>
   * The command specified by the tokens in <code>cmdarray</code> is
   * executed in the run method as a command in a separate process. This
   * constructor is equivalent to using the constructor
   * <code>ExecutionCommand(cmdarray, null)</code>.
   * <p>
   * @param      cmdarray   array containing the command to call and
   *                        its arguments.
   */
  public ExecutionCommand(String[] cmdarray) {
    this(cmdarray, null);
  }

  /**
   * Configures the object to execute the specified command and arguments in
   * a separate process with the specified environment.
   * <p>
   * The command specified by the tokens in <code>cmdarray</code> is
   * executed in the run method as a command in a separate process.
   * <p>
   * The environment variable settings are specified by <tt>envp</tt>.
   * If <tt>envp</tt> is <tt>null</tt>, in the run method the subprocess inherits the
   * environment settings of the current process.
   * <p>
   * This constructor is equivalent to using the constructor
   * <code>ExecutionCommand(cmdarray, envp, null)</code>.
   * <p>
   * @param cmdarray - array containing the command to call and
   *                   its arguments.
   * @param envp - array of strings, each element of which
   *             has environment variable settings in format
   *             <i>name</i>=<i>value</i>.
   */
  public ExecutionCommand(String[] cmdarray, String[] envp) {
    this(cmdarray, envp, null);
  }

  /**
   * Configures the object to execute the specified command and arguments in
   * a separate process with the specified environment and working directory.
   * <p>
   * The command specified by the tokens in <code>cmdarray</code> is
   * executed in the run method as a command in a separate process.
   * <p>
   * The environment variable settings are specified by <tt>envp</tt>.
   * If <tt>envp</tt> is <tt>null</tt>, in the run method the subprocess inherits the
   * environment settings of the current process.
   * <p>
   * This constructor is equivalent to using the constructor
   * <code>ExecutionCommand(cmdarray, envp, null)</code>.
   * <p>
   * @param cmdarray - array containing the command to call and
   *                   its arguments.
   * @param envp - array of strings, each element of which
   *             has environment variable settings in format
   *             <i>name</i>=<i>value</i>.
   * @param dir - the working directory to be used by the system command.
   */
  public ExecutionCommand(String[] cmdarray, String[] envp, File dir) {
    mCmdArray = cmdarray;
    mEnvp = envp;
    mDir = dir;
  }

  /**
   * Set a standard error message to be returned via a TaskEvent to
   * all registered TaskListeners in the event of an error while trying to
   * run the external process.
   * <p>
   * @param s - a string containing the error message.
   * <p>
   * @see com.omniviz.api.event.ExecuteEvent
   * @see com.omniviz.api.event.ExecutionListener
   */
  public void setExecErrorMessage(String s) {
    mExecErrorMessage = s;
  }

  private static String[] tokenizeCommand(String command) {

    StringTokenizer st = new StringTokenizer(command);
    int count = st.countTokens();
    String[] cmdarray = new String[count];

    st = new StringTokenizer(command);

    count = 0;
    while (st.hasMoreTokens()) {
      cmdarray[count++] = st.nextToken();
    }

    return cmdarray;
  }

  private String getCommand() {
    StringBuffer sb = new StringBuffer();
    int n = mCmdArray != null ? mCmdArray.length : 0;
    for(int i=0; i<n; i++) {
      if(i > 0) sb.append(' ');
      sb.append(mCmdArray[i]);
    }
    return sb.toString();
  }


  /**
   * Start the process.  ExecutionListeners should have been added before calling
   * this method.
   * <p>
   * @see com.omniviz.api.event.ExecutionListener
   */
  public void start() {
    if(!mStarted) {
      mStarted = true;
      super.start();
    }
  }

  /**
   * Halt a running process.
   */
  public synchronized void haltCommand() {
    if(this.isAlive()) {
      mStop = true;
      notify(); // Break the run() loop.
    }
  }

  /**
   * Launches the process.  This method should never be called directly.
   * Always call <tt>start()</tt> or <tt>startCommand()</tt>.  If launching
   * the process throws a SecurityException or IOException, the error
   * information will be propagated in an ExecuteEvent with an ERROR status.
   * <p>
   * @see ExecuteEvent
   */
  public synchronized void run() {

    Process process = null;
    ExecutionEvent terminalEvent = null; // Final event to be posted.

    try {
      process = Runtime.getRuntime().exec(mCmdArray, mEnvp, mDir);
    } catch (Throwable t) { // SecurityException, IOException, NoSuchMethodError

      t.printStackTrace();

      String errMsg = null;

      if(mExecErrorMessage != null) {
        errMsg = mExecErrorMessage;
      } else {
        String msg = t.getMessage();
        if(msg == null) {
          msg = t.toString();
        }
        errMsg = "Error executing " + getCommand() + ": " + msg;
      }

      terminalEvent = new ExecutionEvent(this, errMsg, ExecutionEvent.ERROR);
    }

    if(terminalEvent == null) { // That is, no exception was thrown from the exec.

      // Trap stdout and stderr responses on separate threads. They have to be
      // on separate threads, because their polling methods block.
      ExecResponse responseStd = new ExecResponse(this, process.getInputStream(), mBufSize, true);
      ExecResponse responseErr = new ExecResponse(this, process.getErrorStream(), mBufSize, false);
      OutputStream out = process.getOutputStream();
      mOutput = new BufferedOutputStream(out);

      // Two ways to break out of this loop --
      // 1. haltCommand() is called and sets stop to true.
      // 2. The process finishes normally. The ExecResponse class
      //    read a null line and sets done to true.
      //
      while(!mStop && !outputFinished()) {
        // Anything that changes stop or done calls notify() to break the wait().
        try {     // Run loop will hang here until haltCommand() or setDone(true)
                  // calls notify().

          wait(); // Releases the lock. Preferred method of "suspending" a
                  // thread since suspend()/resume() were deprecated.
        } catch (InterruptedException x) {
        	System.out.println("Wait for process to finish was interrupted.");
        }
      }
      
      if(mStop) { // Only true if halted.

        // Kill the foul daemon -- don't want to halt the java thread but leave
        // the external process running.
        process.destroy();
        // Stop the response threads.  They'd probably read nulls and
        // stop on their own, but let's be anal about it.
        if(responseStd.isAlive()) responseStd.quit();
        if(responseErr.isAlive()) responseErr.quit();

        terminalEvent = new ExecutionEvent(this, "Process halted", ExecutionEvent.HALTED);

        mStdout_done = true;
        mStderr_done = true;

      } else { // done must be true -- process finished normally.

        String os = System.getProperty("os.name");

        // Have only tested this on Windows NT and Solaris (SunOS).  For now, assume
        // versions of windows use exitValue(), and versions of unix
        // use waitFor().

        if(os != null && os.startsWith("Windows")) { // Windows
        // Will generate an InterruptedException on Solaris (SunOS) --
        // use waitFor instead.
          mExitStatus = process.exitValue();
        } else { // Unix
          try {
          // Will hang on Windows NT, probably because the process has
          // already exited -- use exitValue() instead.
            mExitStatus = process.waitFor();
          } catch (InterruptedException ie) {

            ie.printStackTrace();

            String errMsg = null;

            if(mExecErrorMessage != null) {
              errMsg = mExecErrorMessage;
            } else {
              errMsg = "Error executing " + getCommand();
            }

            terminalEvent = new ExecutionEvent(this, errMsg, ExecutionEvent.ERROR);
          }
        }

        if(terminalEvent == null)
          terminalEvent = new ExecutionEvent(this, "Process finished",
            ExecutionEvent.FINISHED);
      }
    }

    // Set done to true before posting the event, because the event procedures
    // may call isDone().
    mDone = true;
    if(terminalEvent != null) postEvent(terminalEvent);
  }

  /**
   * Has the process finished?  If true, then <tt>getExitStatus()</tt> may
   * safely be called.
   */
  public boolean isDone() { return mDone; }

  private synchronized boolean outputFinished() {
    return mStdout_done && mStderr_done;
  }

  private synchronized void setOutputFinished(boolean isStdout) {
    if(isStdout) mStdout_done = true;
    else mStderr_done = true;
    if(outputFinished()) notify(); // To break out of run() loop.
  }

  /**
   * Get the exit status returned by the process.  This should not be called
   * unless <tt>isDone()</tt> returns true.
   */
  public int getExitStatus() { return mExitStatus; }

  /**
   * Add an ExecutionListener to receive ExecuteEvents as the process is
   * running.
   * <p>
   * @param l - the ExecutionListener to add
   * <p>
   * @see com.omniviz.api.event.ExecutionListener
   * @see com.omniviz.api.event.ExecuteEvent
   */
  public synchronized void addExecutionListener(ExecutionListener l) {
    if(!mListeners.contains(l)) mListeners.add(l);
  }

  /**
   * Remove an ExecutionListener.
   * <p>
   * @param l - the ExecutionListener to remove.
   * <p>
   * @see com.omniviz.api.event.ExecutionListener
   * @see com.omniviz.api.event.ExecuteEvent
   */
  public synchronized void removeExecutionListener(ExecutionListener l) {
    mListeners.remove(l);
  }

  private void postEvent(ExecutionEvent evt) {
    ExecutionListener[] listeners = getListeners();
    for(int i=0; i<listeners.length; i++) {
      listeners[i].executeEvent(evt);
    }
  }

  private synchronized ExecutionListener[] getListeners() {
    ExecutionListener[] listeners = new ExecutionListener[mListeners.size()];
    mListeners.toArray(listeners);
    return listeners;
  }

  // Nested class to read the output of the process on a
  // separate thread.
  //
  class ExecResponse extends Thread {

    private ExecutionCommand mExec;
    private InputStream input; // The output stream of the process.
    private boolean standard;  // Stdout or stderr?
    private boolean quit;      // Boolean flag to know whether to quit.
    private int bufferSize;

    public ExecResponse(ExecutionCommand exec, InputStream in, int bufSize, boolean std) {
      mExec = exec;
      input = in;
      standard = std;
      bufferSize = bufSize;
      this.start();
    }

    // Use quit() rather than calling stop() on the thread. According to
    // Eckel it's a kinder and gentler way.  And the other way will be
    // deprecated soon.
    //
    public void quit() { quit = true; }

    public void run() {

      char[] buffer = new char[bufferSize];
      int index = 0;

      BufferedInputStream in = null;

      try {

          in = new BufferedInputStream(input);
          int statusType = (standard ? ExecutionEvent.STDOUT : ExecutionEvent.STDERR);

          int n;
          char c;
          char lastChar = ' ';
          boolean haveReturn = false;

          while(!quit) {
            n = in.read();
            if(n == -1) { // -1 when EOF is reached.
                //System.out.println((standard ? "stdout" : "stderr") + " read EOF");
              quit = true;
            } else {
              c = (char) n;
              //System.out.print(".");
              
           	  // detect \r by it's self (instead of \r\n pairs) and send the buffer
              if( haveReturn && c != '\n') {
              	buffer[index++] = '\n'; // append a return first
                  postEvent(new ExecutionEvent(mExec, new String(buffer, 0, index), statusType));
                  index = 0;
              }
              buffer[index++] = c;
              haveReturn = (c == '\r');
              
              // check for the "GLD> " prompt
              if( lastChar == '>' && c == ' '){
            	  String msg = new String(buffer, 0, index).trim();
            	  if( msg.compareTo("GLD>") == 0){
                      postEvent(new ExecutionEvent(mExec, msg + "\r\n", statusType));
                      index = 0;
            	  }
              }
              
              // check if the buffer is filled or a new line is present
              if(index == buffer.length || c == '\n') { // buffer's filled up.
                postEvent(new ExecutionEvent(mExec, new String(buffer, 0, index), statusType));
                index = 0;
              }
              
              lastChar = c;
            }
          }

        if(index > 0) { // Send an event for anything left in the buffer.
          postEvent(new ExecutionEvent(mExec, new String(buffer, 0, index), statusType));
        }

      } catch(IOException e) {
          e.printStackTrace();
      }

      setOutputFinished(standard); // Breaks ExecutionCommand's run() loop when both
      // the stdout and stderr ExecResponse's call this.
    }
  }
  
  /**
   * write output to the process
   * @param msg
   * @throws IOException 
   */
  public void postOutputMessage(String msg) {
	  try {
		  mOutput.write(msg.getBytes());
		  mOutput.write("\n".getBytes());
		  mOutput.flush();
		
	} catch (IOException e) {
		e.printStackTrace();
	}
  }
  
}


