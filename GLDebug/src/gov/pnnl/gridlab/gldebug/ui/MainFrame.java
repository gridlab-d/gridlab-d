package gov.pnnl.gridlab.gldebug.ui;

import gov.pnnl.gridlab.gldebug.model.GLDebugSession;
import gov.pnnl.gridlab.gldebug.model.GLProjectSettings;
import gov.pnnl.gridlab.util.ExecutionEvent;
import gov.pnnl.gridlab.util.ExecutionListener;

import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.WindowEvent;
import java.io.File;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.SwingUtilities;

import sun.misc.Signal;
import sun.misc.SignalHandler;

public class MainFrame extends JFrame  implements ExecutionListener{
    private MainFrame mThis;
    private GLDebugSession session;
    private GLProjectSettings config;
	private JTextArea textAreaInput;
	private JTextArea textAreaOutput;
	/**
	 * Launch the application
	 * @param args
	 */
	public static void main(String args[]) {
		
		try {
			MainFrame frame = new MainFrame();
			frame.setVisible(true);

			Signal.handle(new Signal("INT"), new SignalHandler () {
			      public void handle(Signal sig) {
			          System.out.println(
			            "Aaarggh, a user is trying to interrupt me!!");
			        }
			      });
			
		
		} catch (Exception e) {
			e.printStackTrace();
		}
	}

	/**
	 * Create the frame
	 */
	public MainFrame() {
		super();
		mThis = this;
		final GridBagLayout gridBagLayout = new GridBagLayout();
		gridBagLayout.columnWidths = new int[] {0,7,0};
		getContentPane().setLayout(gridBagLayout);
		setBounds(100, 100, 500, 375);
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		final JScrollPane scrollPane = new JScrollPane();
		final GridBagConstraints gridBagConstraints_1 = new GridBagConstraints();
		gridBagConstraints_1.fill = GridBagConstraints.BOTH;
		gridBagConstraints_1.gridx = 0;
		gridBagConstraints_1.gridy = 1;
		gridBagConstraints_1.gridwidth = 3;
		gridBagConstraints_1.ipadx = 373;
		gridBagConstraints_1.ipady = 65;
		gridBagConstraints_1.insets = new Insets(0, 10, 0, 0);
		getContentPane().add(scrollPane, gridBagConstraints_1);

		textAreaInput = new JTextArea();
		scrollPane.setViewportView(textAreaInput);

		final JScrollPane scrollPane_1 = new JScrollPane();
		final GridBagConstraints gridBagConstraints_3 = new GridBagConstraints();
		gridBagConstraints_3.weighty = 1.0;
		gridBagConstraints_3.fill = GridBagConstraints.BOTH;
		gridBagConstraints_3.gridx = 0;
		gridBagConstraints_3.gridy = 3;
		gridBagConstraints_3.gridwidth = 3;
		gridBagConstraints_3.ipadx = 373;
		gridBagConstraints_3.ipady = 107;
		gridBagConstraints_3.insets = new Insets(0, 10, 0, 0);
		getContentPane().add(scrollPane_1, gridBagConstraints_3);

		textAreaOutput = new JTextArea();
		scrollPane_1.setViewportView(textAreaOutput);

		final JLabel inputLabel = new JLabel();
		inputLabel.setText("Input:");
		final GridBagConstraints gridBagConstraints = new GridBagConstraints();
		gridBagConstraints.anchor = GridBagConstraints.WEST;
		gridBagConstraints.gridx = 0;
		gridBagConstraints.gridy = 0;
		gridBagConstraints.ipadx = 35;
		gridBagConstraints.insets = new Insets(10, 10, 0, 0);
		getContentPane().add(inputLabel, gridBagConstraints);

		final JLabel outputLabel = new JLabel();
		outputLabel.setText("Output:");
		final GridBagConstraints gridBagConstraints_2 = new GridBagConstraints();
		gridBagConstraints_2.anchor = GridBagConstraints.WEST;
		gridBagConstraints_2.gridx = 0;
		gridBagConstraints_2.gridy = 2;
		gridBagConstraints_2.ipadx = 25;
		gridBagConstraints_2.insets = new Insets(6, 10, 0, 0);
		getContentPane().add(outputLabel, gridBagConstraints_2);

		final JButton sendButton = new JButton();
		sendButton.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				sendCommand();
			}
		});
		sendButton.setText("Send");
		final GridBagConstraints gridBagConstraints_6 = new GridBagConstraints();
		gridBagConstraints_6.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_6.gridx = 3;
		gridBagConstraints_6.gridy = 1;
		gridBagConstraints_6.ipadx = 27;
		gridBagConstraints_6.insets = new Insets(0, 6, 0, 10);
		getContentPane().add(sendButton, gridBagConstraints_6);

		final JButton breakButton = new JButton();
		breakButton.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				breakDebugger();
			}
		});
		breakButton.setText("Break");
		final GridBagConstraints gridBagConstraints_4 = new GridBagConstraints();
		gridBagConstraints_4.gridx = 0;
		gridBagConstraints_4.gridy = 4;
		gridBagConstraints_4.ipadx = 38;
		gridBagConstraints_4.insets = new Insets(10, 10, 10, 0);
		getContentPane().add(breakButton, gridBagConstraints_4);

		final JButton startButton = new JButton();
		startButton.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				startDebugging();
			}
		});
		startButton.setText("Start");
		final GridBagConstraints gridBagConstraints_7 = new GridBagConstraints();
		gridBagConstraints_7.insets = new Insets(0, 10, 0, 0);
		gridBagConstraints_7.gridy = 4;
		gridBagConstraints_7.gridx = 1;
		getContentPane().add(startButton, gridBagConstraints_7);

		final JButton closeButton = new JButton();
		closeButton.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				processWindowEvent(new WindowEvent(mThis, WindowEvent.WINDOW_CLOSING));
			}
		});
		closeButton.setText("Close");
		final GridBagConstraints gridBagConstraints_5 = new GridBagConstraints();
		gridBagConstraints_5.anchor = GridBagConstraints.SOUTHEAST;
		gridBagConstraints_5.gridx = 2;
		gridBagConstraints_5.gridy = 4;
		gridBagConstraints_5.gridwidth = 2;
		gridBagConstraints_5.ipadx = 40;
		gridBagConstraints_5.insets = new Insets(10, 10, 10, 10);
		getContentPane().add(closeButton, gridBagConstraints_5);
		//
	}

	/**
	 * 
	 */
	private void startDebugging() {
//		String appDir = "C:\\source\\GridLab\\GridlabD\\source\\VS2005\\Win32\\Debug\\";
//		String modelDir= "C:\\source\\GridLab\\GridlabD\\source\\test\\input\\";
//		config = new GridLabConfig(modelDir, modelDir);
//		config.setDebugger(true);
//		config.setGlPath(appDir);
//		config.setGridLabExe(appDir + "gridlabd.exe");
//		config.setModelDir(modelDir);
//		config.setModelFile("res_integrated_test.glm");

		String appDir = "C:\\source\\GridLab\\GridlabD\\source\\VS2005\\Win32\\Debug\\";
		String modelDir= "C:\\source\\GridLab\\GridlabD\\source\\test\\input\\";
//		config = new GLProjectSettings(modelDir, modelDir);
//		config.setDebugger(true);
//		//config.setVerboseMsgs(true);
//		config.setGlPath(appDir);
//		config.setGridLabExe(appDir + "gridlabd.exe");
//		config.setModelDir(modelDir);
//		config.setModelFile("res_integrated_test.glm");		
//		
//		session.setConfig(config);
//		session.startSimulation();
		
		
	}

	/**
	 * 
	 */
	private void sendCommand() {
		String msg = textAreaInput.getText().trim();
		System.out.print("Sending: "+ msg);
		session.postOuput(msg);
		
	}

	/**
	 * 
	 */
	private void breakDebugger() {
		session.postBreak();
	}
	
	
    /**
     * implements ExecutionListener
     */
    public void executeEvent(final ExecutionEvent evt){
    	String msg = evt.getMessage();
    	if( msg.startsWith("DEBUG: global_clock = ") ) {
    		// ignore processing messages
    		//System.out.println("Ignoring msg: " + msg);
    		return;
    	}
    	
    	SwingUtilities.invokeLater(new Runnable() {
    		public void run() {
    			addMessageOutput(evt);
    		}
    	});
    }
    
    private void addMessageOutput(ExecutionEvent evt) {
    	textAreaOutput.append(evt.getMessageType() + ": " + evt.getMessage());// + "\r\n");
    	textAreaOutput.setCaretPosition(textAreaOutput.getText().length());
//    	
//    	switch (evt.getMessageType()) {
//        case ExecutionEvent.STDERR:
//        case ExecutionEvent.ERROR:
//        	// keep track of errors
//        	errorMsgBuffer.append(evt.getMessage());
//            break;
//        case ExecutionEvent.FINISHED:
//        case ExecutionEvent.HALTED:
////        	// if we had any errors, email
////        	if( errorMsgBuffer.length() > 0){
////        		Log.logError("GridLabId: " + gridLabId + "\n" + errorMsgBuffer.toString(), null, config.getSessionId());
////        	}
//            break;
//        }
//
//    	//Log.logInfo(evt.getMessage(), config.getSessionId());
//        mQueue.add(evt);
    	
    	
	}
	
}
