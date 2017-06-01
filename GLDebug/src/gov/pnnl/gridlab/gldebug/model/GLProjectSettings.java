 /**
 * this class keeps track of configuration for the Gridlab session
 */
package gov.pnnl.gridlab.gldebug.model;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import org.jdom.Document;
import org.jdom.Element;
import org.jdom.input.SAXBuilder;
import org.jdom.output.XMLOutputter;
import org.jdom.output.Format;

import sun.security.action.GetLongAction;

public class GLProjectSettings {

	public enum GLEnvironment {
		BATCH, MATLAB;
		
		public String toString() {
			switch(this){
			case BATCH:
				return "Batch";
			case MATLAB:
				return "MatLab";
			}
			return "";
		}
		
		public static GLEnvironment getFromName(String name) {
			if(name.equals("Batch")){
				return BATCH;
			}
			else if(name.equals("MatLab")){
				return MATLAB;
			}
			return BATCH;
		}
	}
	
	public enum XMLEncoding {
		ENC_8, ENC_16, ENC_32;
		
		public int getVal() {
			switch(this){
				case ENC_8:
					return 8;
				case ENC_16:
					return 16;
				case ENC_32:
					return 32;
			}
			return -1;
		}

		public static XMLEncoding getForValue(int xml) {
			switch(xml){
				case 8:
					return ENC_8;
				case 16:
					return ENC_16;
				case 32:
					return ENC_32;
			}
			return ENC_8;
		}

	};
	
	/**
	 * fileName is the current project file name where all of these settings are saved.
	 */
	private String fileName;
	private String gridLabExe;
	private String workingDir;
	private String outputFileName;
	private List<String> modelFiles = new ArrayList<String>();
	private String glPath;
	
	/**
	 * Simulation Options the user can set
	 */
	private boolean debugger=true;
	private boolean debugMsgs=false;
	private boolean verboseMsgs=false;
	private boolean quiet=false;
	private boolean warnMsgs=false;
	private boolean profiling=false;
	private boolean moduleCheck=false;
	private boolean dumpAll=false;
	private String sessionId = "";
	private String pidFile = "";
	private int pid = 0;
	private XMLEncoding xmlEncoding = XMLEncoding.ENC_8;
	private int threadCnt = 0;
	private GLEnvironment environment = GLEnvironment.BATCH;
	
	/**
	 * a list of all breakpoints
	 */
	private GLBreakpointList breakPointList = new GLBreakpointList();
	/**
	 * a list of all watches
	 */
	private GLWatchList watchList = new GLWatchList();
	
	
	/* 
	 * have there been changes made to this project that need to be saved?
	 */
	private boolean dirty = true;
	// has this project been saved/loaded from disk?
	private boolean onDisk = false;
	
    public GLProjectSettings() {
    }

	public String getGridLabExe() {
		return gridLabExe;
	}

	public void setGridLabExe(String gridLabExe) {
		// gridlabexe is transient, doesn't count as a change
		////dirty = true;
		this.gridLabExe = gridLabExe;
	}

	public String getWorkingDir() {
		return workingDir;
	}

	public void setWorkingDir(String workingDir) {
		dirty = true;
		this.workingDir = workingDir;
	}
	
	/**
	 * return the environmental settings to use for the process launch
	 * @return
	 */
	public String[] getEnv() {

		// get the current system environment
	    Map<String, String> env = System.getenv();
	    List<String> envList = new ArrayList<String>(env.size());

	    for (String envName : env.keySet()) {
	    	if(glPath != null && envName.equalsIgnoreCase("GLPATH")){
	    		// use the supplied GLPATH instead
	    		envList.add("GLPATH=" + glPath);
	    	}
	    	else {
	    		envList.add(envName + "=" + env.get(envName));
	    	}
        }
	    
	    String[] newEnv = new String[envList.size()];
	    for (int i = 0; i < newEnv.length; i++) {
			newEnv[i] = envList.get(i);
		}
		return newEnv;
	}


    public boolean areRequiredParamsSet() {
        // check for required params
        if( workingDir == null || modelFiles.size() == 0 || gridLabExe == null) {
            System.err.println("Required param not set.");
            return false;
        }
        File workDir = new File(workingDir);
        if( !workDir.exists() ) {
            System.err.println("working dir does not exist.");
            return false;
        }
        
        return true;
    }
    
    public String buildCommandLine() {
        //String exe = workingDir + File.separator + "gridlabd.exe";
    	try {
			File tempFile = File.createTempFile("gldbg", ".pid");
			pidFile = tempFile.getAbsolutePath();
			pid = 0;
			tempFile.deleteOnExit();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
    	String outFile = quoteIt(outputFileName);
    	String opts = buildCommandOptions();
    	String pid = "--pidfile=" + quoteIt(pidFile);
    	
        String cmd = quoteIt(gridLabExe)+ opts + " -o " + outFile + " " + pid;
        for (int i = 0; i < modelFiles.size(); i++) {
            cmd += " " +  quoteIt(modelFiles.get(i));
		}

        return cmd;    	
    }
    
    /**
     * if the string contains a space, surround it with quotes
     * @param s
     * @return
     */
    private String quoteIt(String s) {
    	if( s.contains(" ")){
    	   return "\"" + s + "\"";
    	}
    	return s;
    }
    
    /**
     * build command line options
     * @return
     */
    private String buildCommandOptions() {
    	StringBuilder sb = new StringBuilder();
//    	debugger = true;
//    	debugMsgs=true;
//    	verboseMsgs=true;
//    	quiet=true;
//    	warnMsgs=true;
//    	profiling=true;
    	
    	if( debugMsgs ) {
    		sb.append(" --debug");
    	}
    	if( verboseMsgs ) {
    		sb.append(" --verbose");
    	}
    	if( quiet ) {
    		sb.append(" --quiet");
    	}
    	if( warnMsgs ) {
    		sb.append(" --warn");
    	}
    	if( profiling ) {
    		sb.append(" --profile");
    	}
    	if( moduleCheck ) {
    		sb.append(" --check");
    	}
    	if( dumpAll ) {
    		sb.append(" --dumpall");
    	}

		switch(xmlEncoding){
			case ENC_8:
				sb.append(" --xmlencoding 8");
				break;
			case ENC_16:
				sb.append(" --xmlencoding 16");
				break;
			case ENC_32:
				sb.append(" --xmlencoding 32");
				break;
	    }
		
		if( threadCnt > 0 ){
			sb.append(" --threadcount " + String.valueOf(threadCnt));
		}
		
		if( environment == GLEnvironment.MATLAB ){
			sb.append(" --environment matlab");
		}

		// debugger should always be on
    	if( debugger ) {
    		sb.append(" --debugger");
    	}
		
    	// this is always on, makes all output show up on std out (so we don't have separate streams)
    	sb.append(" --bothstdout");
		
    	return sb.toString();
	}

	public List<String> getModelFiles() {
		return modelFiles;
	}

	public void setModelFiles(List<String> list) {
		dirty = true;
		this.modelFiles = list;
	}


	public String getOutputFileName() {
		return outputFileName;
	}

	public void setOutputFileName(String outputFileName) {
		dirty = true;
		this.outputFileName = outputFileName;
	}

	public boolean isDebugMsgs() {
		return debugMsgs;
	}

	public void setDebugMsgs(boolean debugMsgs) {
		dirty = true;
		this.debugMsgs = debugMsgs;
	}

	public boolean isVerboseMsgs() {
		return verboseMsgs;
	}

	public void setVerboseMsgs(boolean verboseMsgs) {
		dirty = true;
		this.verboseMsgs = verboseMsgs;
	}

	public boolean isQuiet() {
		return quiet;
	}

	public void setQuiet(boolean quiet) {
		dirty = true;
		this.quiet = quiet;
	}

	public boolean isWarnMsgs() {
		return warnMsgs;
	}

	public void setWarnMsgs(boolean warnMsgs) {
		dirty = true;
		this.warnMsgs = warnMsgs;
	}

	public boolean isProfiling() {
		return profiling;
	}

	public void setProfiling(boolean profiling) {
		dirty = true;
		this.profiling = profiling;
	}

	public String getGlPath() {
		return glPath;
	}

	public void setGlPath(String glPath) {
		dirty = true;
		this.glPath = glPath;
	}

	public String getSessionId() {
		return sessionId;
	}

	public void setSessionId(String sessionId) {
		dirty = true;
		this.sessionId = sessionId;
	}

	public boolean isDebugger() {
		return debugger;
	}

	public void setDebugger(boolean debugger) {
		dirty = true;
		this.debugger = debugger;
	}
	
	/**
	 * get the process id.  When gridlab runs we tell it to write out it's pid to a file
	 * that we can later read from.
	 * @return
	 */
	public int getPID() {
		if( pid != 0 ){
			return pid;
		}
		
		File file = new File(pidFile);
		if( file.exists()) {
			 try {
				BufferedReader in = new BufferedReader(new FileReader(file));
				String line = in.readLine();
				if(line != null){
				   pid = Integer.parseInt(line);
				}
				return pid;
				
			} catch (FileNotFoundException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		return 0;
	}
	
	public String getBreakCmd() {
		int pid = getPID();
		if( pid > 0 ){
			//return "kill -2 " + pid;
			return "kill -SIGINT " + pid;
		}
		return null;
	}

	public String getKillCmd() {
		int pid = getPID();
		if( pid > 0 ){
			//return "kill -2 " + pid;
			return "kill -SIGTERM " + pid;
		}
		return null;
	}
	
	
    /**
    *
    */
   public static GLProjectSettings load (String fileName) throws Exception{
           File file = new File(fileName);
           if (!file.exists()) {
        	   throw new Exception("File does not exist: " + file );
           }

           // Load the file
           // get a builder with no validation
           SAXBuilder builder = new SAXBuilder(false);
           Document doc = builder.build(file);
           Element root = doc.getRootElement();

           // now read in viewer sources
           if( !root.getName().equals("GridLabDebugProject") ) {
               throw new Exception("Error: not a GridLab Debug Project file.");
           }
           
           Element elem = root.getChild("settings");
           
           // create the list from the xml child contents
           GLProjectSettings settings = new GLProjectSettings();

           if( elem.getAttribute("workingDir") != null ) settings.setWorkingDir(elem.getAttribute("workingDir").getValue());
           if( elem.getAttribute("output") != null ) settings.setOutputFileName(elem.getAttribute("output").getValue());
           if( elem.getAttribute("glpath") != null ) settings.setGlPath(elem.getAttribute("glpath").getValue());
           
           // load all models
           List<String> models = new ArrayList<String>();
           Element elemModels = elem.getChild("models");
           if( elemModels != null ){
	           Iterator iter = elemModels.getChildren().iterator();
	           while (iter.hasNext()) {
	               Element child = (Element)iter.next();
	               models.add(child.getAttribute("model").getValue());
	           }
	           settings.setModelFiles(models);
           }
           
           // load various params
           settings.setVerboseMsgs(elem.getAttribute("verbose").getBooleanValue());
           if(elem.getAttribute("debugger") != null) settings.setDebugger(elem.getAttribute("debugger").getBooleanValue());
           if(elem.getAttribute("debugMsgs") != null) settings.setDebugMsgs(elem.getAttribute("debugMsgs").getBooleanValue());
           if(elem.getAttribute("quiet") != null) settings.setQuiet(elem.getAttribute("quiet").getBooleanValue());
           if(elem.getAttribute("warnMsgs") != null) settings.setWarnMsgs(elem.getAttribute("warnMsgs").getBooleanValue());
           if(elem.getAttribute("moduleCheck") != null) settings.setModuleCheck(elem.getAttribute("moduleCheck").getBooleanValue());
           if(elem.getAttribute("dumpAll") != null) settings.setDumpAll(elem.getAttribute("dumpAll").getBooleanValue());
           if(elem.getAttribute("profiling") != null) settings.setProfiling(elem.getAttribute("profiling").getBooleanValue());
           if(elem.getAttribute("xmlEncoding") != null) {
        	   int xml = elem.getAttribute("xmlEncoding").getIntValue();
               settings.setXmlEncoding(XMLEncoding.getForValue(xml));
           }
           if(elem.getAttribute("threadCount") != null) settings.setThreadCnt(elem.getAttribute("threadCount").getIntValue());
           if(elem.getAttribute("environment") != null) settings.setEnvironment(
        		                                GLEnvironment.getFromName(elem.getAttribute("environment").getValue()));
           
           // load breakpoints saved
           Element elemBreakpoints = root.getChild(GLBreakpointList.XML_ELEMENT);
           if(elemBreakpoints != null){
        	   settings.breakPointList = GLBreakpointList.createFromXML(elemBreakpoints);
           }

           // load watches saved
           Element elemWatches = root.getChild(GLWatchList.XML_ELEMENT);
           if(elemBreakpoints != null){
        	   settings.watchList = GLWatchList.createFromXML(elemWatches);
           }
           
           //TODO more settings need to be loaded here
           
           settings.fileName = fileName;
           settings.dirty = false;
           settings.onDisk = true;
           return settings;

   }

   
   /**
   *
   */
  public boolean save (String fileName){
      try {
          Document doc = null;
          Element root = null;

          root = new Element("GridLabDebugProject");
          doc = new Document(root);

          // create the xml doc
          Element elem = this.getXMLElement();
          root.addContent(elem);
          root.addContent(breakPointList.getXMLElement());
          root.addContent(watchList.getXMLElement());

          // save to file
          File file = new File(fileName);
          FileOutputStream out = new FileOutputStream(file);
          XMLOutputter xmlOut = new XMLOutputter();
          xmlOut.getFormat().setIndent(" ");
          xmlOut.getFormat().setLineSeparator(System.getProperty("line.separator"));
          xmlOut.getFormat().setTextMode(Format.TextMode.PRESERVE);

          xmlOut.output(doc, out);

          out.flush();
          out.close();

          this.fileName = fileName;
          dirty = false;
          onDisk = true;
          
          return true;

      } catch (Exception ex) {
          System.err.println("Error saving GridLab debug project settings.  " + ex.toString() );
          ex.printStackTrace();
      }
      return false;
  }
  
   private Element getXMLElement() {
	   Element elem = new Element("settings");
	   
	   if( workingDir != null ) elem.setAttribute("workingDir", workingDir );
	   if( outputFileName != null ) elem.setAttribute("output", outputFileName);
	   if( glPath != null ) elem.setAttribute("glpath", glPath);
       
	   Element elemModels = new Element("models");
	   elem.addContent(elemModels);
       for (String modelFile : modelFiles) {
		  Element child = new Element("model");
		  child.setAttribute("model", modelFile);
		  elemModels.addContent(child);
	   }
	   
       elem.setAttribute("verbose", Boolean.toString(verboseMsgs));
       elem.setAttribute("debugger", Boolean.toString(debugger));
       elem.setAttribute("debugMsgs", Boolean.toString(debugMsgs));
       elem.setAttribute("quiet", Boolean.toString(quiet));
       elem.setAttribute("warnMsgs", Boolean.toString(warnMsgs));
       elem.setAttribute("moduleCheck", Boolean.toString(moduleCheck));
       elem.setAttribute("dumpAll", Boolean.toString(dumpAll));
       elem.setAttribute("profiling", Boolean.toString(profiling));
       
       elem.setAttribute("xmlEncoding", Integer.toString(xmlEncoding.getVal()));
       elem.setAttribute("threadCount", Integer.toString(threadCnt));

       elem.setAttribute("environment", environment.toString());
       
       // TODO more attributes here
       
       return elem;
   }
   
   

	public String getFileName() {
		return fileName;
	}
	
	
	public void setFileName(String fileName) {
		dirty = true;
		this.fileName = fileName;
	}

	public boolean isDirty() {
		return dirty ;
	}

	public boolean isOnDisk() {
		return onDisk;
	}

	public boolean isModuleCheck() {
		return moduleCheck;
	}

	public void setModuleCheck(boolean moduleCheck) {
		dirty = true;
		this.moduleCheck = moduleCheck;
	}

	public boolean isDumpAll() {
		return dumpAll;
	}

	public void setDumpAll(boolean dumpAll) {
		dirty = true;
		this.dumpAll = dumpAll;
	}

	public XMLEncoding getXmlEncoding() {
		return xmlEncoding;
	}

	public void setXmlEncoding(XMLEncoding xmlEncoding) {
		dirty = true;
		this.xmlEncoding = xmlEncoding;
	}

	public int getThreadCnt() {
		return threadCnt;
	}

	public void setThreadCnt(int threadCnt) {
		dirty = true;
		this.threadCnt = threadCnt;
	}

	public GLEnvironment getEnvironment() {
		return environment;
	}

	public void setEnvironment(GLEnvironment environment) {
		dirty = true;
		this.environment = environment;
	}

	public GLBreakpointList getBreakPointList() {
		return breakPointList;
	}

	public GLWatchList getWatchList() {
		return watchList;
	}
   
}
