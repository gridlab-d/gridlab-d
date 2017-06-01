/**
 *  GLDebug
 * <p>Title: Handles project settings.
 * </p>
 *
 * <p>Description:
 * </p>
 *
 * <p>Copyright: Copyright (C) 2008</p>
 *
 * <p>Company: Battelle Memorial Institute</p>
 *
 * @author Jon McCall
 * @version 1.0
 */
package gov.pnnl.gridlab.gldebug.ui;

import gov.pnnl.gridlab.gldebug.model.GLProjectSettings;
import gov.pnnl.gridlab.gldebug.model.GLProjectSettings.GLEnvironment;
import gov.pnnl.gridlab.gldebug.model.GLProjectSettings.XMLEncoding;
import gov.pnnl.gridlab.gldebug.ui.util.FileChooserEx;
import gov.pnnl.gridlab.gldebug.ui.util.FileLocationPanel;

import java.awt.BorderLayout;
import java.awt.Frame;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.GridLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.KeyEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;

import javax.swing.ButtonGroup;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;

import javax.swing.DefaultListModel;
import javax.swing.JComponent;
import javax.swing.JDialog;
import javax.swing.JFileChooser;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JPanel;
import javax.swing.JRadioButton;
import javax.swing.JScrollPane;
import javax.swing.JSpinner;
import javax.swing.JTextField;
import javax.swing.KeyStroke;
import javax.swing.ListSelectionModel;
import javax.swing.WindowConstants;

public class ProjectSettingsDlg extends JDialog {

	private JLabel selectOptionsToLabel;
	private JRadioButton radioButton_XMLEnc32;
	private JRadioButton radioButton_XMLEnc16;
	private JRadioButton radioButton_XMLEnc8;
	private JCheckBox warningModeCheckBox;
	private JCheckBox callModuleCheckCheckBox;
	private JCheckBox turnOnDebugCheckBox;
	private JCheckBox completeModuleDumpCheckBox;
	private JCheckBox performanceProfileingCheckBox;
	private JCheckBox quietTurnCheckBox;
	private JCheckBox verboseCheckBox;
	private boolean canceled = true;
	private GLProjectSettings origSettings;

	private JComboBox comboBoxEnvironment;
	private JSpinner spinnerThreadCount;
	private JTextField textTimeZone;
	private JTextField textGLPath;
	private JPanel tempOutput;
	private JList listModelFiles;
	private ButtonGroup buttonGroupXMLEnc = new ButtonGroup();
	private FileLocationPanel pnlWorkDir;
	private FileLocationPanel pnlOutputFile;
	
	/**
	 * Create the dialog
	 */
	public ProjectSettingsDlg(Frame parent) {
		super(parent, "", true);
		setModal(true);
		setDefaultCloseOperation(WindowConstants.DISPOSE_ON_CLOSE);
		final GridBagLayout gridBagLayout_3 = new GridBagLayout();
		gridBagLayout_3.rowHeights = new int[] {0,0,7};
		getContentPane().setLayout(gridBagLayout_3);
		setTitle("Project Settings");
		setBounds(100, 100, 622, 543);

		final JPanel panel = new JPanel();
		final GridBagLayout gridBagLayout_2 = new GridBagLayout();
		gridBagLayout_2.rowHeights = new int[] {0,7,7,7,7,7};
		gridBagLayout_2.columnWidths = new int[] {0,7,7,7};
		panel.setLayout(gridBagLayout_2);
		final GridBagConstraints gridBagConstraints_26 = new GridBagConstraints();
		gridBagConstraints_26.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_26.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_26.gridx = 0;
		gridBagConstraints_26.gridy = 0;
		gridBagConstraints_26.gridwidth = 2;
		gridBagConstraints_26.insets = new Insets(5, 5, 0, 5);
		getContentPane().add(panel, gridBagConstraints_26);

		final JLabel modelFilesLabel = new JLabel();
		modelFilesLabel.setText("Model Files:");
		final GridBagConstraints gridBagConstraints_11 = new GridBagConstraints();
		gridBagConstraints_11.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_11.gridy = 0;
		gridBagConstraints_11.gridx = 0;
		panel.add(modelFilesLabel, gridBagConstraints_11);

		final JScrollPane scrollPane = new JScrollPane();
		final GridBagConstraints gridBagConstraints_12 = new GridBagConstraints();
		gridBagConstraints_12.gridheight = 2;
		gridBagConstraints_12.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_12.gridwidth = 2;
		gridBagConstraints_12.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_12.weightx = 1.0;
		gridBagConstraints_12.gridy = 0;
		gridBagConstraints_12.gridx = 1;
		panel.add(scrollPane, gridBagConstraints_12);

		listModelFiles = new JList();
		listModelFiles.setSelectionMode(ListSelectionModel.MULTIPLE_INTERVAL_SELECTION);
		scrollPane.setViewportView(listModelFiles);

		final JButton addButton = new JButton();
		addButton.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_addButton_actionPerformed(e);
			}
		});
		addButton.setText("Add...");
		final GridBagConstraints gridBagConstraints_13 = new GridBagConstraints();
		gridBagConstraints_13.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_13.ipadx = 5;
		gridBagConstraints_13.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_13.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_13.gridy = 0;
		gridBagConstraints_13.gridx = 3;
		panel.add(addButton, gridBagConstraints_13);

		final JButton removeButton = new JButton();
		removeButton.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_removeButton_actionPerformed(e);
			}
		});
		removeButton.setText("Remove");
		final GridBagConstraints gridBagConstraints_16 = new GridBagConstraints();
		gridBagConstraints_16.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_16.insets = new Insets(5, 5, 0, 5);
		gridBagConstraints_16.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_16.gridy = 1;
		gridBagConstraints_16.gridx = 3;
		panel.add(removeButton, gridBagConstraints_16);

		final JLabel workingDirLabel = new JLabel();
		workingDirLabel.setToolTipText("Working directory is usually set to the directory of the model files.");
		workingDirLabel.setText("Working Dir:");
		final GridBagConstraints gridBagConstraints_32 = new GridBagConstraints();
		gridBagConstraints_32.anchor = GridBagConstraints.EAST;
		gridBagConstraints_32.gridy = 2;
		gridBagConstraints_32.gridx = 0;
		panel.add(workingDirLabel, gridBagConstraints_32);

		final JPanel tempWorkDirPanel = new JPanel();
		tempWorkDirPanel.setLayout(new BorderLayout());
		final GridBagConstraints gridBagConstraints_33 = new GridBagConstraints();
		gridBagConstraints_33.insets = new Insets(5, 5, 0, 0);
		gridBagConstraints_33.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_33.gridwidth = 2;
		gridBagConstraints_33.gridy = 2;
		gridBagConstraints_33.gridx = 1;
		panel.add(tempWorkDirPanel, gridBagConstraints_33);

		pnlWorkDir = new FileLocationPanel();
		pnlWorkDir.setToolTipText("Working directory is usually set to the directory of the model files.");
		tempWorkDirPanel.add(pnlWorkDir);

		final JLabel outputLabel = new JLabel();
		outputLabel.setText("Output:");
		final GridBagConstraints gridBagConstraints_14 = new GridBagConstraints();
		gridBagConstraints_14.insets = new Insets(5, 0, 0, 0);
		gridBagConstraints_14.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_14.gridy = 3;
		gridBagConstraints_14.gridx = 0;
		panel.add(outputLabel, gridBagConstraints_14);

		tempOutput = new JPanel();
		tempOutput.setLayout(new BorderLayout());
		final GridBagConstraints gridBagConstraints_15 = new GridBagConstraints();
		gridBagConstraints_15.insets = new Insets(5, 5, 0, 0);
		gridBagConstraints_15.gridwidth = 2;
		gridBagConstraints_15.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_15.gridy = 3;
		gridBagConstraints_15.gridx = 1;
		panel.add(tempOutput, gridBagConstraints_15);

		pnlOutputFile = new FileLocationPanel();
		tempOutput.add(pnlOutputFile);

		final JLabel environmentLabel = new JLabel();
		environmentLabel.setText("Environment:");
		final GridBagConstraints gridBagConstraints_17 = new GridBagConstraints();
		gridBagConstraints_17.insets = new Insets(5, 0, 0, 0);
		gridBagConstraints_17.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_17.gridy = 4;
		gridBagConstraints_17.gridx = 0;
		panel.add(environmentLabel, gridBagConstraints_17);

		comboBoxEnvironment = new JComboBox();
		final GridBagConstraints gridBagConstraints_25 = new GridBagConstraints();
		gridBagConstraints_25.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_25.gridy = 4;
		gridBagConstraints_25.gridx = 1;
		panel.add(comboBoxEnvironment, gridBagConstraints_25);

		final JLabel threadCountLabel = new JLabel();
		threadCountLabel.setText("Thread Count:");
		final GridBagConstraints gridBagConstraints_18 = new GridBagConstraints();
		gridBagConstraints_18.insets = new Insets(5, 0, 0, 0);
		gridBagConstraints_18.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_18.gridy = 5;
		gridBagConstraints_18.gridx = 0;
		panel.add(threadCountLabel, gridBagConstraints_18);

		spinnerThreadCount = new JSpinner();
		final GridBagConstraints gridBagConstraints_24 = new GridBagConstraints();
		gridBagConstraints_24.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_24.insets = new Insets(5, 5, 0, 0);
		gridBagConstraints_24.gridy = 5;
		gridBagConstraints_24.gridx = 1;
		panel.add(spinnerThreadCount, gridBagConstraints_24);

		final JLabel use0ForLabel = new JLabel();
		use0ForLabel.setText("Use 0 for as many as useful.");
		final GridBagConstraints gridBagConstraints_21 = new GridBagConstraints();
		gridBagConstraints_21.insets = new Insets(5, 5, 0, 0);
		gridBagConstraints_21.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_21.gridy = 5;
		gridBagConstraints_21.gridx = 2;
		panel.add(use0ForLabel, gridBagConstraints_21);

		final JLabel glpathLabel = new JLabel();
		glpathLabel.setText("GLPATH:");
		final GridBagConstraints gridBagConstraints_19 = new GridBagConstraints();
		gridBagConstraints_19.insets = new Insets(5, 0, 0, 0);
		gridBagConstraints_19.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_19.gridy = 6;
		gridBagConstraints_19.gridx = 0;
		panel.add(glpathLabel, gridBagConstraints_19);

		textGLPath = new JTextField();
		final GridBagConstraints gridBagConstraints_22 = new GridBagConstraints();
		gridBagConstraints_22.insets = new Insets(5, 5, 0, 0);
		gridBagConstraints_22.gridwidth = 2;
		gridBagConstraints_22.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_22.gridy = 6;
		gridBagConstraints_22.gridx = 1;
		panel.add(textGLPath, gridBagConstraints_22);

		final JLabel timeZoneLabel = new JLabel();
		timeZoneLabel.setText("Time Zone:");
		final GridBagConstraints gridBagConstraints_20 = new GridBagConstraints();
		gridBagConstraints_20.insets = new Insets(5, 0, 0, 0);
		gridBagConstraints_20.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_20.gridy = 7;
		gridBagConstraints_20.gridx = 0;
		panel.add(timeZoneLabel, gridBagConstraints_20);

		textTimeZone = new JTextField();
		final GridBagConstraints gridBagConstraints_23 = new GridBagConstraints();
		gridBagConstraints_23.insets = new Insets(5, 5, 0, 0);
		gridBagConstraints_23.gridwidth = 2;
		gridBagConstraints_23.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_23.weighty = 1.0;
		gridBagConstraints_23.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_23.gridy = 7;
		gridBagConstraints_23.gridx = 1;
		panel.add(textTimeZone, gridBagConstraints_23);

		final JPanel panel_1 = new JPanel();
		final GridBagLayout gridBagLayout = new GridBagLayout();
		gridBagLayout.rowHeights = new int[] {7,7,7,7,7,7,7};
		panel_1.setLayout(gridBagLayout);
		final GridBagConstraints gridBagConstraints_27 = new GridBagConstraints();
		gridBagConstraints_27.fill = GridBagConstraints.BOTH;
		gridBagConstraints_27.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_27.gridx = 0;
		gridBagConstraints_27.gridy = 1;
		gridBagConstraints_27.insets = new Insets(5, 5, 5, 0);
		getContentPane().add(panel_1, gridBagConstraints_27);

		selectOptionsToLabel = new JLabel();
		selectOptionsToLabel.setText("Select Options To Enable:");
		final GridBagConstraints gridBagConstraints_34 = new GridBagConstraints();
		gridBagConstraints_34.insets = new Insets(10, 0, 0, 0);
		gridBagConstraints_34.anchor = GridBagConstraints.WEST;
		gridBagConstraints_34.gridx = 0;
		gridBagConstraints_34.gridy = 0;
		panel_1.add(selectOptionsToLabel, gridBagConstraints_34);

		warningModeCheckBox = new JCheckBox();
		warningModeCheckBox.setText("Warning Mode");
		final GridBagConstraints gridBagConstraints = new GridBagConstraints();
		gridBagConstraints.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints.weightx = 1.0;
		gridBagConstraints.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints.gridy = 1;
		gridBagConstraints.gridx = 0;
		panel_1.add(warningModeCheckBox, gridBagConstraints);

		callModuleCheckCheckBox = new JCheckBox();
		callModuleCheckCheckBox.setText("Module Check Functions");
		final GridBagConstraints gridBagConstraints_1 = new GridBagConstraints();
		gridBagConstraints_1.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_1.weightx = 1.0;
		gridBagConstraints_1.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_1.gridy = 2;
		gridBagConstraints_1.gridx = 0;
		panel_1.add(callModuleCheckCheckBox, gridBagConstraints_1);

		turnOnDebugCheckBox = new JCheckBox();
		turnOnDebugCheckBox.setText("Debug Messages");
		//turnOnDebugCheckBox.setEnabled(false);
		final GridBagConstraints gridBagConstraints_2 = new GridBagConstraints();
		gridBagConstraints_2.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_2.weightx = 1.0;
		gridBagConstraints_2.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_2.gridy = 3;
		gridBagConstraints_2.gridx = 0;
		panel_1.add(turnOnDebugCheckBox, gridBagConstraints_2);

		completeModuleDumpCheckBox = new JCheckBox();
		completeModuleDumpCheckBox.setText("Module Dump on Exit");
		final GridBagConstraints gridBagConstraints_3 = new GridBagConstraints();
		gridBagConstraints_3.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_3.weightx = 1.0;
		gridBagConstraints_3.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_3.gridy = 4;
		gridBagConstraints_3.gridx = 0;
		panel_1.add(completeModuleDumpCheckBox, gridBagConstraints_3);

		performanceProfileingCheckBox = new JCheckBox();
		performanceProfileingCheckBox.setText("Performance Profiling");
		final GridBagConstraints gridBagConstraints_4 = new GridBagConstraints();
		gridBagConstraints_4.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_4.weightx = 1.0;
		gridBagConstraints_4.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_4.gridy = 5;
		gridBagConstraints_4.gridx = 0;
		panel_1.add(performanceProfileingCheckBox, gridBagConstraints_4);

		quietTurnCheckBox = new JCheckBox();
		quietTurnCheckBox.setToolTipText("Only Error and Fatal Messages Displayed");
		quietTurnCheckBox.setText("Quiet Mode (Only Error and Fatal Messages Displayed)");
		//quietTurnCheckBox.setEnabled(false);
		final GridBagConstraints gridBagConstraints_5 = new GridBagConstraints();
		gridBagConstraints_5.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_5.weightx = 1.0;
		gridBagConstraints_5.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_5.gridy = 6;
		gridBagConstraints_5.gridx = 0;
		panel_1.add(quietTurnCheckBox, gridBagConstraints_5);

		verboseCheckBox = new JCheckBox();
		verboseCheckBox.setText("Verbose Mode");
		final GridBagConstraints gridBagConstraints_6 = new GridBagConstraints();
		gridBagConstraints_6.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_6.weightx = 1.0;
		gridBagConstraints_6.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_6.weighty = 1.0;
		gridBagConstraints_6.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_6.gridy = 7;
		gridBagConstraints_6.gridx = 0;
		panel_1.add(verboseCheckBox, gridBagConstraints_6);

		final JPanel panel_2 = new JPanel();
		final GridBagLayout gridBagLayout_1 = new GridBagLayout();
		gridBagLayout_1.rowHeights = new int[] {0,7,7,7};
		panel_2.setLayout(gridBagLayout_1);
		final GridBagConstraints gridBagConstraints_28 = new GridBagConstraints();
		gridBagConstraints_28.weightx = 1.0;
		gridBagConstraints_28.weighty = 1.0;
		gridBagConstraints_28.fill = GridBagConstraints.BOTH;
		gridBagConstraints_28.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_28.gridx = 1;
		gridBagConstraints_28.gridy = 1;
		gridBagConstraints_28.insets = new Insets(5, 20, 5, 5);
		getContentPane().add(panel_2, gridBagConstraints_28);

		final JLabel xmlEncodingLabel = new JLabel();
		xmlEncodingLabel.setText("XML Encoding:");
		final GridBagConstraints gridBagConstraints_7 = new GridBagConstraints();
		gridBagConstraints_7.insets = new Insets(10, 0, 0, 0);
		gridBagConstraints_7.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_7.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_7.gridy = 0;
		gridBagConstraints_7.gridx = 0;
		panel_2.add(xmlEncodingLabel, gridBagConstraints_7);

		radioButton_XMLEnc8 = new JRadioButton();
		radioButton_XMLEnc8.setSelected(true);
		buttonGroupXMLEnc.add(radioButton_XMLEnc8);
		radioButton_XMLEnc8.setText("8");
		final GridBagConstraints gridBagConstraints_8 = new GridBagConstraints();
		gridBagConstraints_8.insets = new Insets(0, 10, 0, 0);
		gridBagConstraints_8.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_8.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_8.gridy = 1;
		gridBagConstraints_8.gridx = 0;
		panel_2.add(radioButton_XMLEnc8, gridBagConstraints_8);

		radioButton_XMLEnc16 = new JRadioButton();
		buttonGroupXMLEnc.add(radioButton_XMLEnc16);
		radioButton_XMLEnc16.setText("16");
		final GridBagConstraints gridBagConstraints_9 = new GridBagConstraints();
		gridBagConstraints_9.insets = new Insets(0, 10, 0, 0);
		gridBagConstraints_9.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_9.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_9.gridy = 2;
		gridBagConstraints_9.gridx = 0;
		panel_2.add(radioButton_XMLEnc16, gridBagConstraints_9);

		radioButton_XMLEnc32 = new JRadioButton();
		buttonGroupXMLEnc.add(radioButton_XMLEnc32);
		radioButton_XMLEnc32.setText("32");
		final GridBagConstraints gridBagConstraints_10 = new GridBagConstraints();
		gridBagConstraints_10.insets = new Insets(0, 10, 0, 0);
		gridBagConstraints_10.weighty = 1;
		gridBagConstraints_10.weightx = 1;
		gridBagConstraints_10.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_10.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_10.gridy = 3;
		gridBagConstraints_10.gridx = 0;
		panel_2.add(radioButton_XMLEnc32, gridBagConstraints_10);

		final JPanel panel_3 = new JPanel();
		final GridBagLayout gridBagLayout_4 = new GridBagLayout();
		gridBagLayout_4.columnWidths = new int[] {0,7};
		panel_3.setLayout(gridBagLayout_4);
		final GridBagConstraints gridBagConstraints_29 = new GridBagConstraints();
		gridBagConstraints_29.insets = new Insets(5, 0, 5, 0);
		gridBagConstraints_29.gridwidth = 2;
		gridBagConstraints_29.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_29.gridy = 2;
		gridBagConstraints_29.gridx = 0;
		getContentPane().add(panel_3, gridBagConstraints_29);

		final JButton okButton = new JButton();
		okButton.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_okButton_actionPerformed(e);
			}
		});
		okButton.setText("OK");
		this.getRootPane().setDefaultButton(okButton);
		
		final GridBagConstraints gridBagConstraints_31 = new GridBagConstraints();
		gridBagConstraints_31.anchor = GridBagConstraints.EAST;
		gridBagConstraints_31.weightx = 1.0;
		gridBagConstraints_31.insets = new Insets(0, 0, 0, 5);
		panel_3.add(okButton, gridBagConstraints_31);

		final JButton cancelButton = new JButton();
		cancelButton.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_cancelButton_actionPerformed(e);
			}
		});
		cancelButton.setText("Cancel");
		final GridBagConstraints gridBagConstraints_30 = new GridBagConstraints();
		gridBagConstraints_30.anchor = GridBagConstraints.EAST;
		gridBagConstraints_30.insets = new Insets(0, 0, 0, 5);
		gridBagConstraints_30.gridy = 0;
		gridBagConstraints_30.gridx = 1;
		panel_3.add(cancelButton, gridBagConstraints_30);
		//
		
        KeyStroke stroke = KeyStroke.getKeyStroke(KeyEvent.VK_ESCAPE, 0);
        rootPane.registerKeyboardAction(new ActionListener() {
            public void actionPerformed(ActionEvent e) {
            	do_cancelButton_actionPerformed(e);
            }
        }
        , stroke, JComponent.WHEN_IN_FOCUSED_WINDOW);

        
        setupGUI();
	}
	
	private void setupGUI() {
		pnlOutputFile.addFileFilter(new String[] { "txt" }, "Text Files (*.txt)");
		pnlOutputFile.addFileFilter(new String[] { "xml" }, "XML Files (*.xml)");

		pnlWorkDir.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
		
		comboBoxEnvironment.addItem(GLEnvironment.BATCH);
		comboBoxEnvironment.addItem(GLEnvironment.MATLAB);
		
		// make sure the list is editable
	    DefaultListModel model = new DefaultListModel();
	    listModelFiles.setModel(model);
		
	}
	
	protected void do_okButton_actionPerformed(final ActionEvent e) {
		canceled = false;
		//TODO more here, validate fields
		commitProjectSettings();
		processWindowEvent(new WindowEvent(this, WindowEvent.WINDOW_CLOSING));
	}
	protected void do_cancelButton_actionPerformed(final ActionEvent e) {
		canceled = true;
		processWindowEvent(new WindowEvent(this, WindowEvent.WINDOW_CLOSING));
	}
	public boolean isCanceled() {
		return canceled;
	}
	protected void do_addButton_actionPerformed(final ActionEvent e) {
		
        FileChooserEx chooser = FileChooserEx.fetchFileChooserEx();
        chooser.setMultiSelectionEnabled(true);

//        if( m.length() > 0  ) {
//            File f = new File(getFileName());
//            chooser.setCurrentDirectory( f );
//            chooser.setName( getFileName() );
//            chooser.setSelectedFile(f);
//        }
//        else if( mDefaultDir != null && mDefaultDir.length() > 0 ){
//            File f = new File(mDefaultDir);
//            chooser.setCurrentDirectory(f);
//        }

        chooser.addFilter( new String[]{"glm", "xml"}, "Model Files (*.glm, *.xml)" );

        int retVal = chooser.showOpenDialog(this);
        if (chooser.getSelectedFile() != null && retVal == FileChooserEx.APPROVE_OPTION ) {
        	File[] files = chooser.getSelectedFiles();
        	for (File file : files) {
				((DefaultListModel)listModelFiles.getModel()).addElement(file.getAbsolutePath());
			}
        }
		
	}
	protected void do_removeButton_actionPerformed(final ActionEvent e) {
		int[] selected = listModelFiles.getSelectedIndices();
		for (int i = selected.length - 1; i > -1; i--) {
			((DefaultListModel)listModelFiles.getModel()).remove(selected[i]);
		}
	}
	
	public void setGLProjectSettings(GLProjectSettings gls) {
		origSettings = gls;
		// set values in the controls....
		
		pnlWorkDir.setFileName(gls.getWorkingDir());
		pnlOutputFile.setFileName(gls.getOutputFileName());
		textGLPath.setText(gls.getGlPath());
		
		((DefaultListModel)listModelFiles.getModel()).clear();
		for (String modelFile : gls.getModelFiles()) {
			((DefaultListModel)listModelFiles.getModel()).addElement(modelFile);
		}
		
		verboseCheckBox.setSelected(gls.isVerboseMsgs());
		warningModeCheckBox.setSelected(gls.isWarnMsgs());
		quietTurnCheckBox.setSelected(gls.isQuiet());
		turnOnDebugCheckBox.setSelected(gls.isDebugMsgs());
		performanceProfileingCheckBox.setSelected(gls.isProfiling());
		completeModuleDumpCheckBox.setSelected(gls.isDumpAll());
		callModuleCheckCheckBox.setSelected(gls.isModuleCheck());
		switch(gls.getXmlEncoding()){
			case ENC_8:
				radioButton_XMLEnc8.setSelected(true);
				break;
			case ENC_16:
				radioButton_XMLEnc16.setSelected(true);
				break;
			case ENC_32:
				radioButton_XMLEnc32.setSelected(true);
				break;
		}
		
		spinnerThreadCount.setValue(gls.getThreadCnt());
		comboBoxEnvironment.setSelectedItem(gls.getEnvironment());
		
		//TODO more as we get them...
		
		
	}
	
	/**
	 * commit the changes back to the original
	 */
	private void commitProjectSettings() {
		origSettings.setWorkingDir(pnlWorkDir.getFileName());
		origSettings.setOutputFileName(pnlOutputFile.getFileName());
		origSettings.setGlPath(textGLPath.getText());

		java.util.List<String> modelList = origSettings.getModelFiles();
		modelList.clear();
		for (int i = 0; i < listModelFiles.getModel().getSize(); i++) {
			modelList.add((String)((DefaultListModel)listModelFiles.getModel()).getElementAt(i));
		}
		
		origSettings.setVerboseMsgs(verboseCheckBox.isSelected());
		// turning off DEBUG messages is not a good idea.  Some command won't work (help, where, print...)
		origSettings.setDebugMsgs(turnOnDebugCheckBox.isSelected());
		// same with QUIET - LIST command won't work
		origSettings.setQuiet(quietTurnCheckBox.isSelected());
		
		origSettings.setWarnMsgs(warningModeCheckBox.isSelected());
		origSettings.setProfiling(performanceProfileingCheckBox.isSelected());		
		origSettings.setDumpAll(completeModuleDumpCheckBox.isSelected());
		origSettings.setModuleCheck(callModuleCheckCheckBox.isSelected());
		if(radioButton_XMLEnc8.isSelected()){
			origSettings.setXmlEncoding(GLProjectSettings.XMLEncoding.ENC_8);
		}
		else if(radioButton_XMLEnc16.isSelected()){
			origSettings.setXmlEncoding(GLProjectSettings.XMLEncoding.ENC_16);
		}
		else if(radioButton_XMLEnc32.isSelected()){
			origSettings.setXmlEncoding(GLProjectSettings.XMLEncoding.ENC_32);
		}
		
		origSettings.setThreadCnt((Integer)spinnerThreadCount.getValue());
		origSettings.setEnvironment((GLEnvironment)comboBoxEnvironment.getSelectedItem());
		
	}

}

