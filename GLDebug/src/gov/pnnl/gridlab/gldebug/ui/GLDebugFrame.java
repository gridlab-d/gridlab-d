/**
 *  GLDebug
 * <p>Title: Main frame in the application.
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

import gov.pnnl.gridlab.gldebug.model.GLCommand;
import gov.pnnl.gridlab.gldebug.model.GLDebugManager;
import gov.pnnl.gridlab.gldebug.model.GLDebugSession;
import gov.pnnl.gridlab.gldebug.model.GLGlobalList;
import gov.pnnl.gridlab.gldebug.model.GLListener;
import gov.pnnl.gridlab.gldebug.model.GLObject;
import gov.pnnl.gridlab.gldebug.model.GLObjectProperties;
import gov.pnnl.gridlab.gldebug.model.GLObjectTree;
import gov.pnnl.gridlab.gldebug.model.GLProjectListener;
import gov.pnnl.gridlab.gldebug.model.GLProjectSettings;
import gov.pnnl.gridlab.gldebug.model.GLStatus;
import gov.pnnl.gridlab.gldebug.model.GLStepType;
import gov.pnnl.gridlab.gldebug.model.StepStatus;
import gov.pnnl.gridlab.gldebug.model.SimulationStatus;
import gov.pnnl.gridlab.gldebug.model.GLCommand.GLCommandName;
import gov.pnnl.gridlab.gldebug.model.GLObjectProperties.Property;
import gov.pnnl.gridlab.gldebug.model.GLProjectListener.ProjectState;
import gov.pnnl.gridlab.gldebug.ui.util.FileChooserEx;
import gov.pnnl.gridlab.gldebug.ui.util.RecentFileManager;
import gov.pnnl.gridlab.gldebug.ui.util.StyledJTextPane;
import gov.pnnl.gridlab.gldebug.ui.util.UserPreferences;
import gov.pnnl.gridlab.gldebug.ui.util.Utils;
import gov.pnnl.gridlab.util.ExecutionEvent;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.Rectangle;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.FocusAdapter;
import java.awt.event.FocusEvent;
import java.awt.event.InputEvent;
import java.awt.event.KeyEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.File;
import javax.swing.BoxLayout;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFormattedTextField;

import javax.swing.DefaultListModel;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSeparator;
import javax.swing.JSplitPane;
import javax.swing.JTabbedPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.JTree;
import javax.swing.KeyStroke;
import javax.swing.SpringLayout;
import javax.swing.SwingUtilities;
import javax.swing.border.EtchedBorder;
import javax.swing.border.TitledBorder;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;
import javax.swing.event.TreeSelectionEvent;
import javax.swing.event.TreeSelectionListener;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreePath;
import javax.swing.tree.TreeSelectionModel;

public class GLDebugFrame extends JFrame implements GLListener, GLProjectListener, TreeSelectionListener {

    private JTabbedPane mtabPaneDetails;
    private JButton mbtnWatchsAdd;
    private JTextField mtxtNewWatchObject;
    private JComboBox mcboxNewWatchPropery;
    private JLabel valueLabel_1;
    private JLabel typeLabel_1;
    private JPanel panel_9;
    private JButton mbtnWatchesRemove;
    private JButton mbtnWatchesDisableAll;
    private JButton mbtnWatchesEnableAll;
    private JPanel panel_8;
    private JTable mtblWatches;
    private JTable table;
    private JScrollPane scrollPane_6;
    private JButton mbtnBreakpointsRemove;
    private JButton mbtnBreakpointsDisableAll;
    private JButton mbtnBreakpointsEnableAll;
    private JPanel panel_6;
    private JTextField mtxtNewBreakValue;
    private JButton mbtnBreakpointsAdd;
    private JLabel valueLabel;
    private JComboBox mcboxNewBreakType;
    private JLabel typeLabel;
    private JPanel panel_5;
    private JTable mtblBreakpoints;
    private JScrollPane scrollPane_4;
    private JButton mbtnModifyGlobals;
    private GlobalsTable mtblGlobals;
    private JScrollPane scrollPane_3;
    private JMenuItem fileMenuCloseProject;
    private JMenuItem fileMenuSaveAs;
    private JMenuItem fileMenuSave;
    private JLabel label_Iteration;
    private JLabel iterationLabel;
    private JLabel lblDetailsObject;
    private JLabel label_HardEvents;
    private JLabel label_Rank;
    private JLabel label_Pass;
    private JLabel label_SyncStatus;
    private JLabel label_StepTime;
    private JLabel label_ObjectClock;
    private JLabel label_ObjectName;
    private StyledJTextPane txtErrorOutput;
    private JScrollPane scrollPane_2_3;
    private JPanel panel_4;
    private StyledJTextPane txtVerboseOutput;
    private JScrollPane scrollPane_2_2;
    private StyledJTextPane txtDebugOutput;
    private JScrollPane scrollPane_2_1;
    private JMenu fileMenu;
    private JMenuItem fileMenuExit;
    private JMenuItem debugMenuStep;
    private JMenuItem debugMenuStop;
    private JMenuItem debugMenuRunTo;
    private JMenuItem debugMenuRun;
    private JMenuItem debugMenuLoad;
    private JMenu debugMenu;
    private JMenu projectMenu;
    private GLDebugManager manager;
	private DefaultMutableTreeNode rootTreeNode = new DefaultMutableTreeNode(GLObjectTree.ROOT);
	private DefaultTreeModel treeModel = new DefaultTreeModel(rootTreeNode);
//	treeModel.addTreeModelListener(new MyTreeModelListener());

	
	
    private JLabel stepByLabel;
    private JButton stopButton;
    private JButton runToButton;
    private JButton runButton;
    private JButton stepButton;
    private StyledJTextPane txtConsoleOutput;
    
	private JLabel label_GlobalClock;
	private DetailsTable mtblDetails;
	private JTree tree;
	private JComboBox comboBox_1;
	private JTextField textField;
	private JComboBox comboBoxStepBy;
	private boolean doTreeUpdateOnSelection = true;
	/**
	 * Create the frame
	 */
	public GLDebugFrame() {
		super();
		addWindowListener(new WindowAdapter() {
			public void windowClosing(final WindowEvent e) {
				do_this_windowClosing(e);
			}
		});
		this.setTitle("GLDebug");
		getContentPane().setLayout(new BorderLayout());
		setBounds(100, 100, 902, 752);
		Rectangle rect = this.getBounds();
		Rectangle bounds = UserPreferences.instanceOf().getWindowLocation("Main", "mainForm", rect);
		this.setBounds(bounds);
		
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		final JMenuBar menuBar = new JMenuBar();
		setJMenuBar(menuBar);

		fileMenu = new JMenu();
		fileMenu.setMnemonic('F');
		fileMenu.setText("File");
		menuBar.add(fileMenu);

		final JMenuItem fileMenuNewProject = new JMenuItem();
		fileMenuNewProject.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_N, InputEvent.CTRL_MASK));
		fileMenuNewProject.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				fileMenuNewProject_actionPerformed(e);
			}
		});
		fileMenuNewProject.setMnemonic('N');
		fileMenuNewProject.setText("New Project...");
		fileMenu.add(fileMenuNewProject);

		final JMenuItem fileMenuOpenProject = new JMenuItem();
		fileMenuOpenProject.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_O, InputEvent.CTRL_MASK));
		fileMenuOpenProject.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				fileMenuOpenProject_actionPerformed(e);
			}
		});
		fileMenuOpenProject.setMnemonic('O');
		fileMenuOpenProject.setText("Open Project...");
		fileMenu.add(fileMenuOpenProject);

		fileMenuCloseProject = new JMenuItem();
		fileMenuCloseProject.setMnemonic('C');
		fileMenuCloseProject.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_fileMenuCloseProject_actionPerformed(e);
			}
		});
		fileMenuCloseProject.setText("Close Project");
		fileMenu.add(fileMenuCloseProject);

		fileMenu.addSeparator();

		fileMenuSave = new JMenuItem();
		fileMenuSave.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_S, InputEvent.CTRL_MASK));
		fileMenuSave.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				fileMenuSave_actionPerformed(e);
			}
		});
		fileMenuSave.setMnemonic('S');
		fileMenuSave.setText("Save");
		fileMenu.add(fileMenuSave);

		fileMenuSaveAs = new JMenuItem();
		fileMenuSaveAs.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				fileMenuSaveAs_actionPerformed(e);
			}
		});
		fileMenuSaveAs.setMnemonic('A');
		fileMenuSaveAs.setText("Save As...");
		fileMenu.add(fileMenuSaveAs);

		fileMenu.addSeparator();

		fileMenuExit = new JMenuItem();
		fileMenuExit.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				fileMenuExit_actionPerformed(e);
			}
		});
		fileMenuExit.setMnemonic('x');
		fileMenuExit.setText("Exit");
		fileMenu.add(fileMenuExit);

		projectMenu = new JMenu();
		projectMenu.setMnemonic('P');
		projectMenu.setText("Project");
		menuBar.add(projectMenu);

		final JMenuItem projectMenuSettings = new JMenuItem();
		projectMenuSettings.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				projectMenuSettings_actionPerformed(e);
			}
		});
		projectMenuSettings.setMnemonic('S');
		projectMenuSettings.setText("Settings...");
		projectMenu.add(projectMenuSettings);

		debugMenu = new JMenu();
		debugMenu.setMnemonic('D');
		debugMenu.setText("Debug");
		menuBar.add(debugMenu);

		debugMenuLoad = new JMenuItem();
		debugMenu.add(debugMenuLoad);
		debugMenuLoad.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_debugMenuLoad_actionPerformed(e);
			}
		});
		debugMenuLoad.setText("Load");

		debugMenu.addSeparator();

		debugMenuRun = new JMenuItem();
		debugMenuRun.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_debugMenuRun_actionPerformed(e);
			}
		});
		debugMenuRun.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_F5, 0));
		debugMenuRun.setText("Run");
		debugMenu.add(debugMenuRun);

		debugMenuRunTo = new JMenuItem();
		debugMenuRunTo.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_debugMenuRunTo_actionPerformed(e);
			}
		});
		debugMenuRunTo.setText("Run To...");
		debugMenu.add(debugMenuRunTo);

		debugMenuStop = new JMenuItem();
		debugMenuStop.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_debugMenuStop_actionPerformed(e);
			}
		});
		debugMenuStop.setText("Stop");
		debugMenu.add(debugMenuStop);

		debugMenuStep = new JMenuItem();
		debugMenuStep.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_debugMenuStep_actionPerformed(e);
			}
		});
		debugMenuStep.setAccelerator(KeyStroke.getKeyStroke(KeyEvent.VK_F11, 0));
		debugMenuStep.setText("Step");
		debugMenu.add(debugMenuStep);

		final JMenu toolsMenu = new JMenu();
		toolsMenu.setMnemonic('T');
		toolsMenu.setText("Tools");
		menuBar.add(toolsMenu);

		final JMenuItem toolsMenuProcesses = new JMenuItem();
		toolsMenuProcesses.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				toolsMenuProcesses_actionPerformed(e);
			}
		});
		toolsMenuProcesses.setMnemonic('P');
		toolsMenuProcesses.setText("Processes...");
		toolsMenu.add(toolsMenuProcesses);

		toolsMenu.addSeparator();

		final JMenuItem toolsMenuSettings = new JMenuItem();
		toolsMenuSettings.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_toolsMenuSettings_actionPerformed(e);
			}
		});
		toolsMenuSettings.setText("Settings...");
		toolsMenu.add(toolsMenuSettings);

		final JMenu helpMenu = new JMenu();
		helpMenu.setMnemonic('H');
		helpMenu.setText("Help");
		menuBar.add(helpMenu);

		final JMenuItem helpMenuAbout = new JMenuItem();
		helpMenuAbout.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				helpMenuAbout_actionPerformed(e);
			}
		});
		helpMenuAbout.setMnemonic('A');
		helpMenuAbout.setText("About GLDebug...");
		helpMenu.add(helpMenuAbout);

		final JPanel panel_status = new JPanel();
		final GridBagLayout gridBagLayout_1 = new GridBagLayout();
		gridBagLayout_1.columnWidths = new int[] {0,7,7,7};
		gridBagLayout_1.rowHeights = new int[] {0,7,7,7,7};
		panel_status.setLayout(gridBagLayout_1);
		getContentPane().add(panel_status, BorderLayout.NORTH);

		final JLabel objectNameLabel = new JLabel();
		objectNameLabel.setText("Object Name:");
		final GridBagConstraints gridBagConstraints_17 = new GridBagConstraints();
		gridBagConstraints_17.insets = new Insets(5, 0, 0, 0);
		gridBagConstraints_17.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_17.gridy = 0;
		gridBagConstraints_17.gridx = 0;
		panel_status.add(objectNameLabel, gridBagConstraints_17);

		label_ObjectName = new JLabel();
		label_ObjectName.setText("...");
		final GridBagConstraints gridBagConstraints_21 = new GridBagConstraints();
		gridBagConstraints_21.insets = new Insets(5, 5, 0, 0);
		gridBagConstraints_21.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_21.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_21.gridy = 0;
		gridBagConstraints_21.gridx = 1;
		panel_status.add(label_ObjectName, gridBagConstraints_21);

		final JLabel syncStatusLabel = new JLabel();
		syncStatusLabel.setText("Sync Status:");
		final GridBagConstraints gridBagConstraints_22 = new GridBagConstraints();
		gridBagConstraints_22.insets = new Insets(5, 0, 0, 0);
		gridBagConstraints_22.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_22.gridy = 0;
		gridBagConstraints_22.gridx = 2;
		panel_status.add(syncStatusLabel, gridBagConstraints_22);

		label_SyncStatus = new JLabel();
		label_SyncStatus.setText("...");
		final GridBagConstraints gridBagConstraints_23 = new GridBagConstraints();
		gridBagConstraints_23.weightx = 1;
		gridBagConstraints_23.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_23.insets = new Insets(5, 5, 0, 0);
		gridBagConstraints_23.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_23.gridy = 0;
		gridBagConstraints_23.gridx = 3;
		panel_status.add(label_SyncStatus, gridBagConstraints_23);

		final JLabel globalClockLabel = new JLabel();
		globalClockLabel.setText("Global Clock:");
		final GridBagConstraints gridBagConstraints_18 = new GridBagConstraints();
		gridBagConstraints_18.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_18.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_18.gridy = 1;
		gridBagConstraints_18.gridx = 0;
		panel_status.add(globalClockLabel, gridBagConstraints_18);

		label_GlobalClock = new JLabel();
		label_GlobalClock.setText("...");
		final GridBagConstraints gridBagConstraints_25 = new GridBagConstraints();
		gridBagConstraints_25.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_25.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_25.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_25.gridy = 1;
		gridBagConstraints_25.gridx = 1;
		panel_status.add(label_GlobalClock, gridBagConstraints_25);

		final JLabel passLabel = new JLabel();
		passLabel.setText("Pass:");
		final GridBagConstraints gridBagConstraints_28 = new GridBagConstraints();
		gridBagConstraints_28.insets = new Insets(0, 24, 0, 0);
		gridBagConstraints_28.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_28.gridy = 1;
		gridBagConstraints_28.gridx = 2;
		panel_status.add(passLabel, gridBagConstraints_28);

		label_Pass = new JLabel();
		label_Pass.setText("...");
		final GridBagConstraints gridBagConstraints_31 = new GridBagConstraints();
		gridBagConstraints_31.weightx = 1;
		gridBagConstraints_31.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_31.insets = new Insets(0, 5, 0, 5);
		gridBagConstraints_31.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_31.gridy = 1;
		gridBagConstraints_31.gridx = 3;
		panel_status.add(label_Pass, gridBagConstraints_31);

		final JLabel objectClockLabel = new JLabel();
		objectClockLabel.setText("Object Clock:");
		final GridBagConstraints gridBagConstraints_19 = new GridBagConstraints();
		gridBagConstraints_19.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_19.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_19.gridy = 2;
		gridBagConstraints_19.gridx = 0;
		panel_status.add(objectClockLabel, gridBagConstraints_19);

		label_ObjectClock = new JLabel();
		label_ObjectClock.setText("...");
		final GridBagConstraints gridBagConstraints_26 = new GridBagConstraints();
		gridBagConstraints_26.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_26.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_26.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_26.gridy = 2;
		gridBagConstraints_26.gridx = 1;
		panel_status.add(label_ObjectClock, gridBagConstraints_26);

		final JLabel rankLabel = new JLabel();
		rankLabel.setText("Rank:");
		final GridBagConstraints gridBagConstraints_29 = new GridBagConstraints();
		gridBagConstraints_29.insets = new Insets(0, 24, 0, 0);
		gridBagConstraints_29.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_29.gridy = 2;
		gridBagConstraints_29.gridx = 2;
		panel_status.add(rankLabel, gridBagConstraints_29);

		label_Rank = new JLabel();
		label_Rank.setText("...");
		final GridBagConstraints gridBagConstraints_32 = new GridBagConstraints();
		gridBagConstraints_32.weightx = 1;
		gridBagConstraints_32.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_32.insets = new Insets(0, 5, 0, 5);
		gridBagConstraints_32.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_32.gridy = 2;
		gridBagConstraints_32.gridx = 3;
		panel_status.add(label_Rank, gridBagConstraints_32);

		final JLabel stepToTimeLabel = new JLabel();
		stepToTimeLabel.setText("Step to Time:");
		final GridBagConstraints gridBagConstraints_20 = new GridBagConstraints();
		gridBagConstraints_20.insets = new Insets(0, 0, 0, 0);
		gridBagConstraints_20.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_20.gridy = 3;
		gridBagConstraints_20.gridx = 0;
		panel_status.add(stepToTimeLabel, gridBagConstraints_20);

		label_StepTime = new JLabel();
		label_StepTime.setText("...");
		final GridBagConstraints gridBagConstraints_27 = new GridBagConstraints();
		gridBagConstraints_27.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_27.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_27.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_27.weightx = 1;
		gridBagConstraints_27.gridy = 3;
		gridBagConstraints_27.gridx = 1;
		panel_status.add(label_StepTime, gridBagConstraints_27);

		final JLabel hardEventsLabel = new JLabel();
		hardEventsLabel.setText("Hard Events:");
		final GridBagConstraints gridBagConstraints_30 = new GridBagConstraints();
		gridBagConstraints_30.insets = new Insets(0, 0, 0, 0);
		gridBagConstraints_30.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_30.gridy = 3;
		gridBagConstraints_30.gridx = 2;
		panel_status.add(hardEventsLabel, gridBagConstraints_30);

		label_HardEvents = new JLabel();
		label_HardEvents.setText("...");
		final GridBagConstraints gridBagConstraints_24 = new GridBagConstraints();
		gridBagConstraints_24.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_24.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_24.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_24.weighty = 0;
		gridBagConstraints_24.weightx = 0;
		gridBagConstraints_24.gridy = 3;
		gridBagConstraints_24.gridx = 3;
		panel_status.add(label_HardEvents, gridBagConstraints_24);

		iterationLabel = new JLabel();
		iterationLabel.setText("Iteration:");
		final GridBagConstraints gridBagConstraints_37 = new GridBagConstraints();
		gridBagConstraints_37.anchor = GridBagConstraints.NORTHEAST;
		gridBagConstraints_37.gridy = 4;
		gridBagConstraints_37.gridx = 2;
		panel_status.add(iterationLabel, gridBagConstraints_37);

		label_Iteration = new JLabel();
		label_Iteration.setText("...");
		final GridBagConstraints gridBagConstraints_38 = new GridBagConstraints();
		gridBagConstraints_38.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_38.weighty = 1;
		gridBagConstraints_38.weightx = 1;
		gridBagConstraints_38.insets = new Insets(0, 5, 5, 0);
		gridBagConstraints_38.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_38.gridy = 4;
		gridBagConstraints_38.gridx = 3;
		panel_status.add(label_Iteration, gridBagConstraints_38);

		final JSplitPane splitPane = new JSplitPane();
		splitPane.setDividerSize(7);
		splitPane.setDividerLocation(300);
		splitPane.setOrientation(JSplitPane.VERTICAL_SPLIT);
		getContentPane().add(splitPane, BorderLayout.CENTER);

		final JSplitPane splitPane_top = new JSplitPane();
		splitPane_top.setDividerSize(7);
		splitPane_top.setDividerLocation(250);
		splitPane.setLeftComponent(splitPane_top);

		final JPanel panel_Tree = new JPanel();
		panel_Tree.setLayout(new GridBagLayout());
		splitPane_top.setLeftComponent(panel_Tree);

		final JPanel panel = new JPanel();
		panel.setLayout(new GridBagLayout());
		final GridBagConstraints gridBagConstraints_15 = new GridBagConstraints();
		gridBagConstraints_15.weighty = 1;
		gridBagConstraints_15.weightx = 1;
		gridBagConstraints_15.fill = GridBagConstraints.BOTH;
		gridBagConstraints_15.gridx = 0;
		gridBagConstraints_15.gridy = 0;
		gridBagConstraints_15.ipadx = 102;
		gridBagConstraints_15.ipady = 2;
		gridBagConstraints_15.insets = new Insets(5, 5, 0, 5);
		panel_Tree.add(panel, gridBagConstraints_15);

		final JLabel viewLabel = new JLabel();
		viewLabel.setText("View:");
		final GridBagConstraints gridBagConstraints_12 = new GridBagConstraints();
		gridBagConstraints_12.gridx = 0;
		gridBagConstraints_12.gridy = 0;
		gridBagConstraints_12.insets = new Insets(0, 0, 0, 0);
		panel.add(viewLabel, gridBagConstraints_12);

		comboBox_1 = new JComboBox();
		final GridBagConstraints gridBagConstraints_14 = new GridBagConstraints();
		gridBagConstraints_14.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_14.anchor = GridBagConstraints.WEST;
		gridBagConstraints_14.gridx = 1;
		gridBagConstraints_14.gridy = 0;
		gridBagConstraints_14.insets = new Insets(0, 5, 5, 0);
		panel.add(comboBox_1, gridBagConstraints_14);

		final JScrollPane scrollPane = new JScrollPane();
		final GridBagConstraints gridBagConstraints_13 = new GridBagConstraints();
		gridBagConstraints_13.fill = GridBagConstraints.BOTH;
		gridBagConstraints_13.weighty = 1;
		gridBagConstraints_13.weightx = 1;
		gridBagConstraints_13.gridx = 0;
		gridBagConstraints_13.gridy = 1;
		gridBagConstraints_13.gridwidth = 2;
		gridBagConstraints_13.ipady = -173;
		gridBagConstraints_13.insets = new Insets(0, 0, 0, 0);
		panel.add(scrollPane, gridBagConstraints_13);

		tree = new JTree(treeModel);
		tree.setRootVisible(false);
		tree.setShowsRootHandles(true);
		scrollPane.setViewportView(tree);

		final JPanel panel_2 = new JPanel();
		panel_2.setLayout(new GridBagLayout());
		final GridBagConstraints gridBagConstraints_16 = new GridBagConstraints();
		gridBagConstraints_16.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_16.gridx = 0;
		gridBagConstraints_16.gridy = 1;
		gridBagConstraints_16.ipadx = -44;
		gridBagConstraints_16.ipady = -1;
		gridBagConstraints_16.insets = new Insets(6, 5, 6, 6);
		panel_Tree.add(panel_2, gridBagConstraints_16);

		final JButton findButton = new JButton();
		findButton.setText("Find");
		final GridBagConstraints gridBagConstraints_11 = new GridBagConstraints();
		gridBagConstraints_11.anchor = GridBagConstraints.WEST;
		gridBagConstraints_11.gridx = 1;
		gridBagConstraints_11.gridy = 1;
		gridBagConstraints_11.insets = new Insets(6, 6, 0, 0);
		panel_2.add(findButton, gridBagConstraints_11);

		final JPanel panel_3 = new JPanel();
		panel_3.setLayout(new GridBagLayout());
		final GridBagConstraints gridBagConstraints_8 = new GridBagConstraints();
		gridBagConstraints_8.anchor = GridBagConstraints.WEST;
		gridBagConstraints_8.gridwidth = 2;
		gridBagConstraints_8.insets = new Insets(0, 0, 0, 0);
		gridBagConstraints_8.fill = GridBagConstraints.BOTH;
		gridBagConstraints_8.gridx = 0;
		gridBagConstraints_8.gridy = 0;
		gridBagConstraints_8.ipadx = 93;
		gridBagConstraints_8.ipady = 7;
		panel_2.add(panel_3, gridBagConstraints_8);

		final JButton addBreakButton = new JButton();
		addBreakButton.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_addBreakButton_actionPerformed(e);
			}
		});
		final GridBagConstraints gridBagConstraints_7 = new GridBagConstraints();
		gridBagConstraints_7.gridy = 0;
		gridBagConstraints_7.gridx = 0;
		panel_3.add(addBreakButton, gridBagConstraints_7);
		addBreakButton.setText("Add Break");

		final JButton addWatchButton = new JButton();
		final GridBagConstraints gridBagConstraints_10 = new GridBagConstraints();
		gridBagConstraints_10.insets = new Insets(0, 6, 0, 0);
		gridBagConstraints_10.anchor = GridBagConstraints.WEST;
		gridBagConstraints_10.weightx = 1;
		gridBagConstraints_10.gridy = 0;
		gridBagConstraints_10.gridx = 1;
		panel_3.add(addWatchButton, gridBagConstraints_10);
		addWatchButton.setText("Add Watch");

		textField = new JTextField();
		final GridBagConstraints gridBagConstraints_9 = new GridBagConstraints();
		gridBagConstraints_9.weightx = 1;
		gridBagConstraints_9.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_9.gridx = 0;
		gridBagConstraints_9.gridy = 1;
		gridBagConstraints_9.insets = new Insets(0, 0, 0, 0);
		panel_2.add(textField, gridBagConstraints_9);

		mtabPaneDetails = new JTabbedPane();
		mtabPaneDetails.addChangeListener(new ChangeListener() {
			public void stateChanged(final ChangeEvent e) {
				do_mtabPaneDetails_stateChanged(e);
			}
		});
		splitPane_top.setRightComponent(mtabPaneDetails);

		final JPanel panel_Details = new JPanel();
		final GridBagLayout gridBagLayout_2 = new GridBagLayout();
		gridBagLayout_2.rowHeights = new int[] {0,7,7};
		gridBagLayout_2.columnWidths = new int[] {0,7};
		panel_Details.setLayout(gridBagLayout_2);
		mtabPaneDetails.addTab("Details", null, panel_Details, null);

		final JLabel detailsLabel = new JLabel();
		detailsLabel.setText("Object:");
		final GridBagConstraints gridBagConstraints_33 = new GridBagConstraints();
		gridBagConstraints_33.gridy = 0;
		gridBagConstraints_33.gridx = 0;
		panel_Details.add(detailsLabel, gridBagConstraints_33);

		lblDetailsObject = new JLabel();
		lblDetailsObject.setText("...");
		final GridBagConstraints gridBagConstraints_34 = new GridBagConstraints();
		gridBagConstraints_34.insets = new Insets(0, 5, 0, 0);
		gridBagConstraints_34.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_34.anchor = GridBagConstraints.WEST;
		gridBagConstraints_34.gridy = 0;
		gridBagConstraints_34.gridx = 1;
		panel_Details.add(lblDetailsObject, gridBagConstraints_34);

		final JScrollPane scrollPane_1 = new JScrollPane();
		final GridBagConstraints gridBagConstraints_35 = new GridBagConstraints();
		gridBagConstraints_35.insets = new Insets(0, 0, 0, 0);
		gridBagConstraints_35.weightx = 1.0;
		gridBagConstraints_35.weighty = 1.0;
		gridBagConstraints_35.fill = GridBagConstraints.BOTH;
		gridBagConstraints_35.gridwidth = 2;
		gridBagConstraints_35.gridy = 1;
		gridBagConstraints_35.gridx = 0;
		panel_Details.add(scrollPane_1, gridBagConstraints_35);

		mtblDetails = new DetailsTable();
		scrollPane_1.setViewportView(mtblDetails);

		final JButton modifyDetailsButton = new JButton();
		modifyDetailsButton.setText("Modify");
		final GridBagConstraints gridBagConstraints_36 = new GridBagConstraints();
		gridBagConstraints_36.insets = new Insets(5, 5, 5, 0);
		gridBagConstraints_36.anchor = GridBagConstraints.WEST;
		gridBagConstraints_36.gridwidth = 2;
		gridBagConstraints_36.gridy = 2;
		gridBagConstraints_36.gridx = 0;
		panel_Details.add(modifyDetailsButton, gridBagConstraints_36);

		final JPanel panel_Globals = new JPanel();
		final GridBagLayout gridBagLayout_3 = new GridBagLayout();
		gridBagLayout_3.rowHeights = new int[] {0,7};
		panel_Globals.setLayout(gridBagLayout_3);
		mtabPaneDetails.addTab("Globals", null, panel_Globals, null);

		scrollPane_3 = new JScrollPane();
		final GridBagConstraints gridBagConstraints_39 = new GridBagConstraints();
		gridBagConstraints_39.fill = GridBagConstraints.BOTH;
		gridBagConstraints_39.weighty = 1;
		gridBagConstraints_39.weightx = 1;
		gridBagConstraints_39.gridy = 0;
		gridBagConstraints_39.gridx = 0;
		panel_Globals.add(scrollPane_3, gridBagConstraints_39);

		mtblGlobals = new GlobalsTable();
		scrollPane_3.setViewportView(mtblGlobals);

		mbtnModifyGlobals = new JButton();
		mbtnModifyGlobals.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_mbtnModifyGlobals_actionPerformed(e);
			}
		});
		mbtnModifyGlobals.setText("Modify");
		final GridBagConstraints gridBagConstraints_40 = new GridBagConstraints();
		gridBagConstraints_40.insets = new Insets(5, 5, 5, 0);
		gridBagConstraints_40.anchor = GridBagConstraints.WEST;
		gridBagConstraints_40.gridy = 1;
		gridBagConstraints_40.gridx = 0;
		panel_Globals.add(mbtnModifyGlobals, gridBagConstraints_40);

		final JPanel panel_Breakpoints = new JPanel();
		final GridBagLayout gridBagLayout_4 = new GridBagLayout();
		gridBagLayout_4.columnWidths = new int[] {0,7};
		gridBagLayout_4.rowHeights = new int[] {0,7};
		panel_Breakpoints.setLayout(gridBagLayout_4);
		mtabPaneDetails.addTab("Breakpoints", null, panel_Breakpoints, null);

		scrollPane_4 = new JScrollPane();
		final GridBagConstraints gridBagConstraints_41 = new GridBagConstraints();
		gridBagConstraints_41.fill = GridBagConstraints.BOTH;
		gridBagConstraints_41.weighty = 1;
		gridBagConstraints_41.weightx = 1;
		gridBagConstraints_41.gridy = 0;
		gridBagConstraints_41.gridx = 0;
		panel_Breakpoints.add(scrollPane_4, gridBagConstraints_41);

		mtblBreakpoints = new JTable();
		scrollPane_4.setViewportView(mtblBreakpoints);

		panel_6 = new JPanel();
		final GridBagLayout gridBagLayout_6 = new GridBagLayout();
		gridBagLayout_6.rowHeights = new int[] {0,7,7};
		panel_6.setLayout(gridBagLayout_6);
		final GridBagConstraints gridBagConstraints_48 = new GridBagConstraints();
		gridBagConstraints_48.fill = GridBagConstraints.BOTH;
		gridBagConstraints_48.gridy = 0;
		gridBagConstraints_48.gridx = 1;
		panel_Breakpoints.add(panel_6, gridBagConstraints_48);

		mbtnBreakpointsEnableAll = new JButton();
		final GridBagConstraints gridBagConstraints_49 = new GridBagConstraints();
		gridBagConstraints_49.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_49.gridx = 0;
		gridBagConstraints_49.gridy = 0;
		gridBagConstraints_49.insets = new Insets(5, 5, 5, 5);
		panel_6.add(mbtnBreakpointsEnableAll, gridBagConstraints_49);
		mbtnBreakpointsEnableAll.setText("Enable All");

		mbtnBreakpointsDisableAll = new JButton();
		mbtnBreakpointsDisableAll.setText("Disable All");
		final GridBagConstraints gridBagConstraints_50 = new GridBagConstraints();
		gridBagConstraints_50.insets = new Insets(0, 5, 0, 5);
		gridBagConstraints_50.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_50.gridy = 1;
		gridBagConstraints_50.gridx = 0;
		panel_6.add(mbtnBreakpointsDisableAll, gridBagConstraints_50);

		mbtnBreakpointsRemove = new JButton();
		mbtnBreakpointsRemove.setText("Remove");
		final GridBagConstraints gridBagConstraints_51 = new GridBagConstraints();
		gridBagConstraints_51.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_51.insets = new Insets(5, 5, 0, 5);
		gridBagConstraints_51.anchor = GridBagConstraints.NORTH;
		gridBagConstraints_51.weighty = 1;
		gridBagConstraints_51.weightx = 1;
		gridBagConstraints_51.gridy = 2;
		gridBagConstraints_51.gridx = 0;
		panel_6.add(mbtnBreakpointsRemove, gridBagConstraints_51);

		panel_5 = new JPanel();
		final GridBagLayout gridBagLayout_5 = new GridBagLayout();
		gridBagLayout_5.columnWidths = new int[] {0,7,7};
		gridBagLayout_5.rowHeights = new int[] {0,7};
		panel_5.setLayout(gridBagLayout_5);
		panel_5.setBorder(new TitledBorder(null, "New Breakpoint", TitledBorder.DEFAULT_JUSTIFICATION, TitledBorder.DEFAULT_POSITION, null, null));
		final GridBagConstraints gridBagConstraints_42 = new GridBagConstraints();
		gridBagConstraints_42.gridwidth = 2;
		gridBagConstraints_42.insets = new Insets(5, 5, 5, 5);
		gridBagConstraints_42.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_42.gridy = 1;
		gridBagConstraints_42.gridx = 0;
		panel_Breakpoints.add(panel_5, gridBagConstraints_42);

		typeLabel = new JLabel();
		typeLabel.setText("Type:");
		final GridBagConstraints gridBagConstraints_43 = new GridBagConstraints();
		gridBagConstraints_43.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_43.gridx = 0;
		gridBagConstraints_43.gridy = 0;
		gridBagConstraints_43.insets = new Insets(0, 0, 0, 0);
		panel_5.add(typeLabel, gridBagConstraints_43);

		valueLabel = new JLabel();
		valueLabel.setText("Value:");
		final GridBagConstraints gridBagConstraints_45 = new GridBagConstraints();
		gridBagConstraints_45.insets = new Insets(0, 10, 0, 0);
		gridBagConstraints_45.anchor = GridBagConstraints.WEST;
		gridBagConstraints_45.gridy = 0;
		gridBagConstraints_45.gridx = 1;
		panel_5.add(valueLabel, gridBagConstraints_45);

		mcboxNewBreakType = new JComboBox();
		final GridBagConstraints gridBagConstraints_44 = new GridBagConstraints();
		gridBagConstraints_44.insets = new Insets(0, 0, 0, 0);
		gridBagConstraints_44.anchor = GridBagConstraints.WEST;
		gridBagConstraints_44.gridy = 1;
		gridBagConstraints_44.gridx = 0;
		panel_5.add(mcboxNewBreakType, gridBagConstraints_44);

		mtxtNewBreakValue = new JTextField();
		final GridBagConstraints gridBagConstraints_46 = new GridBagConstraints();
		gridBagConstraints_46.weightx = 1;
		gridBagConstraints_46.insets = new Insets(0, 10, 0, 0);
		gridBagConstraints_46.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_46.gridy = 1;
		gridBagConstraints_46.gridx = 1;
		panel_5.add(mtxtNewBreakValue, gridBagConstraints_46);

		mbtnBreakpointsAdd = new JButton();
		mbtnBreakpointsAdd.setText("Add");
		final GridBagConstraints gridBagConstraints_47 = new GridBagConstraints();
		gridBagConstraints_47.insets = new Insets(0, 15, 5, 5);
		gridBagConstraints_47.weightx = 0;
		gridBagConstraints_47.gridy = 1;
		gridBagConstraints_47.gridx = 2;
		panel_5.add(mbtnBreakpointsAdd, gridBagConstraints_47);

		final JPanel panel_Watches = new JPanel();
		final GridBagLayout gridBagLayout_7 = new GridBagLayout();
		gridBagLayout_7.rowHeights = new int[] {0,7};
		gridBagLayout_7.columnWidths = new int[] {0,7};
		panel_Watches.setLayout(gridBagLayout_7);
		mtabPaneDetails.addTab("Watches", null, panel_Watches, null);

		scrollPane_6 = new JScrollPane();
		final GridBagConstraints gridBagConstraints_52 = new GridBagConstraints();
		gridBagConstraints_52.fill = GridBagConstraints.BOTH;
		gridBagConstraints_52.weighty = 1;
		gridBagConstraints_52.weightx = 1;
		panel_Watches.add(scrollPane_6, gridBagConstraints_52);

		table = new JTable();
		scrollPane_6.setViewportView(table);

		mtblWatches = new JTable();
		scrollPane_6.setViewportView(mtblWatches);

		panel_8 = new JPanel();
		panel_8.setLayout(new GridBagLayout());
		final GridBagConstraints gridBagConstraints_56 = new GridBagConstraints();
		gridBagConstraints_56.anchor = GridBagConstraints.NORTH;
		gridBagConstraints_56.gridy = 0;
		gridBagConstraints_56.gridx = 1;
		panel_Watches.add(panel_8, gridBagConstraints_56);

		mbtnWatchesEnableAll = new JButton();
		mbtnWatchesEnableAll.setText("Enable All");
		final GridBagConstraints gridBagConstraints_53 = new GridBagConstraints();
		gridBagConstraints_53.insets = new Insets(5, 5, 5, 5);
		gridBagConstraints_53.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_53.gridy = 0;
		gridBagConstraints_53.gridx = 0;
		panel_8.add(mbtnWatchesEnableAll, gridBagConstraints_53);

		mbtnWatchesDisableAll = new JButton();
		mbtnWatchesDisableAll.setText("Disable All");
		final GridBagConstraints gridBagConstraints_54 = new GridBagConstraints();
		gridBagConstraints_54.insets = new Insets(0, 5, 0, 5);
		gridBagConstraints_54.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_54.gridy = 1;
		gridBagConstraints_54.gridx = 0;
		panel_8.add(mbtnWatchesDisableAll, gridBagConstraints_54);

		mbtnWatchesRemove = new JButton();
		mbtnWatchesRemove.setText("Remove");
		final GridBagConstraints gridBagConstraints_55 = new GridBagConstraints();
		gridBagConstraints_55.insets = new Insets(5, 5, 0, 5);
		gridBagConstraints_55.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_55.anchor = GridBagConstraints.NORTH;
		gridBagConstraints_55.weighty = 1.0;
		gridBagConstraints_55.weightx = 1.0;
		gridBagConstraints_55.gridy = 2;
		gridBagConstraints_55.gridx = 0;
		panel_8.add(mbtnWatchesRemove, gridBagConstraints_55);

		panel_9 = new JPanel();
		panel_9.setBorder(new TitledBorder(null, "New Watch", TitledBorder.DEFAULT_JUSTIFICATION, TitledBorder.DEFAULT_POSITION, null, null));
		panel_9.setLayout(new GridBagLayout());
		final GridBagConstraints gridBagConstraints_62 = new GridBagConstraints();
		gridBagConstraints_62.gridwidth = 2;
		gridBagConstraints_62.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_62.gridy = 1;
		gridBagConstraints_62.gridx = 0;
		panel_Watches.add(panel_9, gridBagConstraints_62);

		typeLabel_1 = new JLabel();
		typeLabel_1.setText("Object:");
		final GridBagConstraints gridBagConstraints_57 = new GridBagConstraints();
		gridBagConstraints_57.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_57.gridy = 0;
		gridBagConstraints_57.gridx = 0;
		panel_9.add(typeLabel_1, gridBagConstraints_57);

		valueLabel_1 = new JLabel();
		valueLabel_1.setText("Property:");
		final GridBagConstraints gridBagConstraints_58 = new GridBagConstraints();
		gridBagConstraints_58.insets = new Insets(0, 10, 0, 0);
		gridBagConstraints_58.anchor = GridBagConstraints.WEST;
		gridBagConstraints_58.gridy = 0;
		gridBagConstraints_58.gridx = 1;
		panel_9.add(valueLabel_1, gridBagConstraints_58);

		mtxtNewWatchObject = new JTextField();
		final GridBagConstraints gridBagConstraints_60 = new GridBagConstraints();
		gridBagConstraints_60.insets = new Insets(0, 0, 0, 0);
		gridBagConstraints_60.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_60.weightx = 1.0;
		gridBagConstraints_60.gridy = 1;
		gridBagConstraints_60.gridx = 0;
		panel_9.add(mtxtNewWatchObject, gridBagConstraints_60);

		mcboxNewWatchPropery = new JComboBox();
		final GridBagConstraints gridBagConstraints_59 = new GridBagConstraints();
		gridBagConstraints_59.insets = new Insets(0, 10, 0, 0);
		gridBagConstraints_59.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_59.weightx = 1;
		gridBagConstraints_59.anchor = GridBagConstraints.WEST;
		gridBagConstraints_59.gridy = 1;
		gridBagConstraints_59.gridx = 1;
		panel_9.add(mcboxNewWatchPropery, gridBagConstraints_59);

		mbtnWatchsAdd = new JButton();
		mbtnWatchsAdd.setText("Add");
		final GridBagConstraints gridBagConstraints_61 = new GridBagConstraints();
		gridBagConstraints_61.insets = new Insets(0, 15, 5, 5);
		gridBagConstraints_61.gridy = 1;
		gridBagConstraints_61.gridx = 2;
		panel_9.add(mbtnWatchsAdd, gridBagConstraints_61);

		final JTabbedPane tabbedPane = new JTabbedPane();
		splitPane.setRightComponent(tabbedPane);

		final JPanel panel_ConsoleMsgs = new JPanel();
		panel_ConsoleMsgs.setLayout(new BorderLayout());
		tabbedPane.addTab("Console", null, panel_ConsoleMsgs, null);

		final JScrollPane scrollPane_2 = new JScrollPane();
		panel_ConsoleMsgs.add(scrollPane_2);

		txtConsoleOutput = new StyledJTextPane();
		txtConsoleOutput.setFont(new Font("Courier New", Font.PLAIN, 12));
		txtConsoleOutput.setEditable(false);
		scrollPane_2.setViewportView(txtConsoleOutput);

		final JPanel panel_DebugMsgs = new JPanel();
		panel_DebugMsgs.setLayout(new BorderLayout());
		panel_DebugMsgs.setName("Debug");
		tabbedPane.addTab("Debug", null, panel_DebugMsgs, null);

		scrollPane_2_1 = new JScrollPane();
		panel_DebugMsgs.add(scrollPane_2_1);

		txtDebugOutput = new StyledJTextPane();
		txtDebugOutput.setFont(new Font("Courier New", Font.PLAIN, 12));
		txtDebugOutput.setEditable(false);
		scrollPane_2_1.setViewportView(txtDebugOutput);

		final JPanel panel_VerboseMsgs = new JPanel();
		panel_VerboseMsgs.setLayout(new BoxLayout(panel_VerboseMsgs, BoxLayout.X_AXIS));
		tabbedPane.addTab("Verbose", null, panel_VerboseMsgs, null);

		scrollPane_2_2 = new JScrollPane();
		panel_VerboseMsgs.add(scrollPane_2_2);

		txtVerboseOutput = new StyledJTextPane();
		txtVerboseOutput.setFont(new Font("Courier New", Font.PLAIN, 12));
		txtVerboseOutput.setEditable(false);
		scrollPane_2_2.setViewportView(txtVerboseOutput);

		panel_4 = new JPanel();
		panel_4.setLayout(new BorderLayout());
		tabbedPane.addTab("Error", null, panel_4, null);

		scrollPane_2_3 = new JScrollPane();
		panel_4.add(scrollPane_2_3);

		txtErrorOutput = new StyledJTextPane();
		txtErrorOutput.setFont(new Font("Courier New", Font.PLAIN, 12));
		txtErrorOutput.setEditable(false);
		scrollPane_2_3.setViewportView(txtErrorOutput);

		final JPanel panel_1 = new JPanel();
		final GridBagLayout gridBagLayout = new GridBagLayout();
		gridBagLayout.columnWidths = new int[] {7,7,0,0,0,0};
		panel_1.setLayout(gridBagLayout);
		getContentPane().add(panel_1, BorderLayout.SOUTH);

		final JButton exitButton = new JButton();
		exitButton.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_exitButton_actionPerformed(e);
			}
		});
		exitButton.setText("Exit");
		final GridBagConstraints gridBagConstraints_6 = new GridBagConstraints();
		gridBagConstraints_6.anchor = GridBagConstraints.EAST;
		gridBagConstraints_6.weightx = 1.0;
		gridBagConstraints_6.gridx = 6;
		gridBagConstraints_6.gridy = 0;
		gridBagConstraints_6.ipadx = 20;
		gridBagConstraints_6.insets = new Insets(10, 24, 10, 10);
		panel_1.add(exitButton, gridBagConstraints_6);

		stopButton = new JButton();
		stopButton.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_stopButton_actionPerformed(e);
			}
		});
		stopButton.setText("Stop");
		final GridBagConstraints gridBagConstraints_5 = new GridBagConstraints();
		gridBagConstraints_5.gridx = 5;
		gridBagConstraints_5.gridy = 0;
		gridBagConstraints_5.ipadx = 20;
		gridBagConstraints_5.insets = new Insets(10, 6, 10, 0);
		panel_1.add(stopButton, gridBagConstraints_5);

		runToButton = new JButton();
		runToButton.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_runToButton_actionPerformed(e);
			}
		});
		runToButton.setText("Run To...");
		final GridBagConstraints gridBagConstraints_4 = new GridBagConstraints();
		gridBagConstraints_4.gridx = 4;
		gridBagConstraints_4.gridy = 0;
		gridBagConstraints_4.insets = new Insets(10, 6, 10, 0);
		panel_1.add(runToButton, gridBagConstraints_4);

		runButton = new JButton();
		runButton.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_runButton_actionPerformed(e);
			}
		});
		runButton.setText("Run");
		final GridBagConstraints gridBagConstraints_3 = new GridBagConstraints();
		gridBagConstraints_3.gridx = 3;
		gridBagConstraints_3.gridy = 0;
		gridBagConstraints_3.ipadx = 25;
		gridBagConstraints_3.insets = new Insets(10, 24, 10, 0);
		panel_1.add(runButton, gridBagConstraints_3);

		stepButton = new JButton();
		stepButton.addActionListener(new ActionListener() {
			public void actionPerformed(final ActionEvent e) {
				do_stepButton_actionPerformed(e);
			}
		});
		stepButton.setText("Step");
		final GridBagConstraints gridBagConstraints_2 = new GridBagConstraints();
		gridBagConstraints_2.gridx = 2;
		gridBagConstraints_2.gridy = 0;
		gridBagConstraints_2.ipadx = 20;
		gridBagConstraints_2.insets = new Insets(10, 6, 10, 0);
		panel_1.add(stepButton, gridBagConstraints_2);

		stepByLabel = new JLabel();
		stepByLabel.setText("Step By:");
		final GridBagConstraints gridBagConstraints_1 = new GridBagConstraints();
		gridBagConstraints_1.anchor = GridBagConstraints.WEST;
		gridBagConstraints_1.gridx = 0;
		gridBagConstraints_1.gridy = 0;
		gridBagConstraints_1.insets = new Insets(10, 10, 10, 0);
		panel_1.add(stepByLabel, gridBagConstraints_1);

		comboBoxStepBy = new JComboBox();
		final GridBagConstraints gridBagConstraints = new GridBagConstraints();
		gridBagConstraints.anchor = GridBagConstraints.WEST;
		gridBagConstraints.gridx = 1;
		gridBagConstraints.gridy = 0;
		gridBagConstraints.ipadx = 18;
		gridBagConstraints.ipady = 4;
		gridBagConstraints.insets = new Insets(10, 6, 10, 0);
		panel_1.add(comboBoxStepBy, gridBagConstraints);
		//
		
		setupGUI();
	}
	
	private void setupGUI() {
		
        // add recent files to the file list
        RecentFileManager.getSingleton().assignToMenu(fileMenu, fileMenuExit);
		
        mtblDetails.setupTable();
        mtblGlobals.setupTable();
        
        manager = GLDebugManager.instance();
        manager.getSession().addListener(this);
        
        tree.getSelectionModel().setSelectionMode(TreeSelectionModel.SINGLE_TREE_SELECTION);
        tree.addTreeSelectionListener(this);
        
        ProjectHandler.addListener(this);

        // fill in the step by choices
        GLStepType[] types = GLStepType.getTypes();
        for (int i = 0; i < types.length; i++) {
            comboBoxStepBy.addItem(types[i]);        
		}
        comboBoxStepBy.setSelectedItem(GLStepType.OBJECT);
        
		enableControls();
	}
	
	private void enableControls() {
		GLStatus status = manager.getSession().getStatus();
		boolean haveProject = (manager.getProjectSettings() != null);

		boolean glExeRunning = manager.getSession().isGridlabRunning();
		boolean glWaiting = (glExeRunning && status == GLStatus.STOPPED);
		boolean simRunning = (glExeRunning && status == GLStatus.RUNNING);
		
		stepButton.setEnabled(glWaiting);
		runButton.setEnabled(glWaiting);
		runToButton.setEnabled(glWaiting);
		comboBoxStepBy.setEnabled(glWaiting);
		stepByLabel.setEnabled(glWaiting);
		stopButton.setEnabled(simRunning);

		
		debugMenuStep.setEnabled(glWaiting);
		debugMenuRun.setEnabled(glWaiting);
		debugMenuRunTo.setEnabled(glWaiting);
		debugMenuStop.setEnabled(simRunning);
		
		projectMenu.setEnabled(manager.hasProject());
		debugMenu.setEnabled(manager.hasProject());
		
		fileMenuSave.setEnabled(haveProject);
		fileMenuSaveAs.setEnabled(haveProject);
		fileMenuCloseProject.setEnabled(haveProject);
		
	}

	
	/**
	 * start a new project
	 * @param e
	 */
    private void fileMenuNewProject_actionPerformed(ActionEvent e) {
    	ProjectHandler.newProject();
    	enableControls();
	}

    
    private void fileMenuOpenProject_actionPerformed(ActionEvent e) {
    	ProjectHandler.openProject();
    	enableControls();
	}

    private void fileMenuSave_actionPerformed(ActionEvent e) {
    	ProjectHandler.saveProject();
    	enableControls();
	}

    private void fileMenuSaveAs_actionPerformed(ActionEvent e) {
    	ProjectHandler.saveProjectAs();
    	enableControls();
	}

	protected void do_fileMenuCloseProject_actionPerformed(final ActionEvent e) {
		ProjectHandler.closeProject();
    	enableControls();
	}
    
    private void fileMenuExit_actionPerformed(ActionEvent e) {
    	if( ProjectHandler.closeProject() ){
    		processWindowEvent(new WindowEvent(this, WindowEvent.WINDOW_CLOSING));
    	}
	}
    
    private void projectMenuSettings_actionPerformed(ActionEvent e) {
    	ProjectHandler.editProjectSettings();
    	enableControls();
	}

    /**
     * make sure gridlab.exe path is set (and other settings required to run) 
     * 
     * @return true if everything is ok
     */
    private boolean ensureGridLabExeSettings() {
		if(manager.getSession().getGlExePath() == null || manager.getSession().getGlExePath().length() == 0){
			int val = JOptionPane.showConfirmDialog(this, "The path to the GridLabd.exe must be defined before you can debug the model.\n"
					                            + "Would you like to edit the settings that now?",
					                            "Project Settings", JOptionPane.YES_NO_OPTION, JOptionPane.QUESTION_MESSAGE );
			if( val == JOptionPane.YES_OPTION ){
				do_toolsMenuSettings_actionPerformed(null);
			}
		}
		return (manager.getSession().getGlExePath() != null || manager.getSession().getGlExePath().length() == 0);		
	}
    
    private void toolsMenuProcesses_actionPerformed(ActionEvent e) {
	}
    
    private void helpMenuAbout_actionPerformed(ActionEvent e) {
	}
	protected void do_addBreakButton_actionPerformed(final ActionEvent e) {
	}
	
	///////////// implement interface GLListener {
	public void notifyClockChange(final String msg){
    	SwingUtilities.invokeLater(new Runnable() {
    		public void run() {
    			label_GlobalClock.setText(msg);
    		}
    	});
	}
	
	public void notifyEvent(final ExecutionEvent evt) {
    	SwingUtilities.invokeLater(new Runnable() {
    		public void run() {
    			addMessageOutput(evt);
    		}
    	});
    }
	/**
	 * notifies of current status
	 */
	public void notifyStatusChanged(GLStatus status, gov.pnnl.gridlab.gldebug.model.GLCommand cmd) {
		System.out.println("Status changed " + status + "  cmd=" + cmd.getCmdName());
		GLCommandName cmdType = cmd.getCmdName();
		switch (status) {
			case NONE:
				debugMenuLoad.setText("Load");
				
				break;
			case RUNNING:
				debugMenuLoad.setText("Re-Load");
				break;
			case STOPPED:
				debugMenuLoad.setText("Re-Load");
				if( manager.getSession().isGridlabRunning() ){
					if( cmdType == GLCommandName.LOAD || cmdType == GLCommandName.RUN ){
						// ask for current context after a project load or when run stops
						manager.getSession().queueCommand(new GLCommand(GLCommandName.CONTEXT));
						// make sure globals are up-to-date
						updateGlobals();
					}
					if( cmdType == GLCommandName.LOAD ){
						// ask for list of objects after a project load
						manager.getSession().queueCommand(new GLCommand(GLCommandName.LIST));
						// also set the breakpoints/watches
						manager.getSession().InstallBreakpoints();
					}
					if( cmdType == GLCommandName.LIST || cmdType == GLCommandName.RUN ){
						// when it stops, display the current item
						manager.getSession().queueCommand(new GLCommand(GLCommandName.PRINTCURR));
					}
				}
				else {
					// gridlab is no longer running...
					notifySimulationStatus(new SimulationStatus());
					clearStatusText();
					
				}
				break;
		}

		enableControls();
	}
	

	private void updateGlobals() {
		int index =  mtabPaneDetails.getSelectedIndex();
		String title = mtabPaneDetails.getTitleAt(index);
		if( title.equals("Globals")) {
			// if showing the globals tab, do the refresh
			manager.getSession().queueCommand(new GLCommand(GLCommandName.GLOBALS_LIST));
		}
		
	}

	/**
	 * handle commands finishing
	 */
	public void notifyGLCommandResults(final GLCommand cmd) {
		SwingUtilities.invokeLater(new Runnable() {
			public void run() {
				switch(cmd.getCmdName()){
		   		    case PRINTCURR:
					case PRINTOBJ:
						updateDetailsPane((GLObjectProperties)cmd.getOutput(), 
										    cmd.getCmdName() == GLCommandName.PRINTCURR);
						break;
		
					case GLOBALS_LIST:
						updateGlobalsPane((GLGlobalList)cmd.getOutput());
						break;
						
					case LIST:
						updateGLObjectListTree(cmd.getArgs() == null);
						
		    			StepStatus status = (StepStatus)cmd.getOutput();
	    			    notifyStepStatus(status);
						break;
		
					case LOAD:
					case NEXT:
		    			StepStatus stepStatus = (StepStatus)cmd.getOutput();
	    			    notifyStepStatus(stepStatus);
						break;
					
					case CONTEXT:
		    			SimulationStatus simStatus = (SimulationStatus)cmd.getOutput();
		    			notifySimulationStatus(simStatus);
		    			break;
				}
			}

		});
	}

	
	/**
	 * update the status fields
	 * @param status
	 */
	private void notifySimulationStatus(SimulationStatus status) {
		label_GlobalClock.setText(status.getGlobalClock());
		label_HardEvents.setText(Integer.toString(status.getHardEvents()));
		// updated from the "print" command
//		label_ObjectClock.setText("..."); 
		label_ObjectName.setText(status.getObject());
		label_Pass.setText(status.getPass());
		label_Rank.setText(Integer.toString(status.getRank()));
		label_StepTime.setText(status.getStepToTime());
		label_SyncStatus.setText(status.getSyncStatus());
	}
	
	/**
	 * notifies of a single object update
	 * @param status
	 */
	private void notifyStepStatus(StepStatus status){
		if( status != null ){
			label_GlobalClock.setText(status.getGlobalClock());
			label_ObjectName.setText(status.getObjectName());
			label_Pass.setText(status.getPass());
			label_Rank.setText(Integer.toString(status.getRank()));
			label_Iteration.setText(Integer.toString(status.getIteration()));
			
			if( status.isUpdateFocus() ){
				// select the object given in the tree
				if( rootTreeNode.getChildCount() > 0 ){
					selectItemInTree(status.getObjectName(), true);
				}
				else {
					// no tree, just update the details tab
					updateObjectDetails(status.getObjectName());
				}
			}
			
		}
	}
	
	/**
	 * update the text and labels in the details panel
	 * @param props
	 * @param updateObjClock
	 */
	private void updateDetailsPane(GLObjectProperties props, boolean updateObjClock) {
		
		if(props == null){
			lblDetailsObject.setText("");
			if( updateObjClock ){
				label_ObjectClock.setText("");
			}
		}
		else {
			// update the details
			System.out.println("Output : " + props.getObjectName() + "  " + props.getProps().size());
			lblDetailsObject.setText(props.getObjectName());

			if( updateObjClock ){
				// get the "clock" property if available and display at top.
				Property clock = props.getProperty("clock");
				if(clock != null){
					label_ObjectClock.setText(clock.getValue());
				}
				else {
					label_ObjectClock.setText("");
				}
			}
			
			// select the current object in the tree
			selectItemInTree(props.getName(), false);
			
		}
		mtblDetails.setProps(props);
		
	}

	/**
	 * update the text and labels in the globals panel
	 * @param props
	 * @param updateObjClock
	 */
	private void updateGlobalsPane(GLGlobalList props) {
		
		mtblGlobals.setGLGlobals(props);
		
	}
	
	
	/**
	 * select the item by name in the tree.
	 * If updateDetails is true we will signal a "print object" command to update it.
	 * @param objName
	 * @param updateDetails
	 */
	private void selectItemInTree(String objName, boolean updateDetails) {
		// check if already selected
		DefaultMutableTreeNode lastSelected = (DefaultMutableTreeNode)tree.getLastSelectedPathComponent();
		boolean needSelection = true;
		if( lastSelected != null ) {
			GLObject globj = (GLObject)lastSelected.getUserObject();
			if( globj.getName() == objName ) {
				needSelection = false;
			}
		}
		
		if( needSelection ){
			DefaultMutableTreeNode node = findTreeNodeByName(rootTreeNode, objName);
			if( node != null ){
				if(!updateDetails) {
					doTreeUpdateOnSelection = false;
				}
				// now select it
	            TreePath path = new TreePath(node.getPath());
	            tree.setSelectionPath(path);
	            tree.scrollPathToVisible(path);

				doTreeUpdateOnSelection = true;
			}
		}
	}

	/**
	 * search through the current tree and find the node with this object name
	 * @param objectName
	 * @return
	 */
	private DefaultMutableTreeNode findTreeNodeByName(DefaultMutableTreeNode startNode, String objectName) {
		Object obj = startNode.getUserObject();
		if( obj instanceof String ) {
			if( ((String)obj).equals(objectName)) {
				return startNode;
			}
		}
		else if ( obj instanceof GLObject ) {
			if( ((GLObject)obj).getName().equals(objectName) ) {
				return startNode;
			}
		}
		
		// if we get here, check all the children
		for (int i = 0; i < startNode.getChildCount(); i++) {
			DefaultMutableTreeNode node = findTreeNodeByName((DefaultMutableTreeNode)startNode.getChildAt(i), objectName);
			if( node != null ){
				return node;
			}
		}
		return null;
	}

	/**
	 * clear everything on the display.  Useful when project is closed.
	 */
	private void clearOutputText() {
    	txtConsoleOutput.setText("");
    	txtDebugOutput.setText("");
    	txtVerboseOutput.setText("");
    	txtErrorOutput.setText("");
    	removeAllTreeNodes();
    	updateDetailsPane(null, true);
    	updateGlobalsPane(null);
	}
	
	/**
	 * clear text dealing with current status
	 */
	private void clearStatusText() {
    	updateDetailsPane(null, true);
    	updateGlobalsPane(null);

		label_Pass.setText("");
		label_Rank.setText("");
		label_HardEvents.setText("");
		label_Iteration.setText("");
	}
	
	/**
	 * updates the tree with latest gl objects.
	 * @param refresh true we redo the whole tree, if false we just update the status of each object. 
	 */
	private void updateGLObjectListTree(boolean refresh) {
		if( refresh ){
			DefaultMutableTreeNode lastSelected = (DefaultMutableTreeNode)tree.getLastSelectedPathComponent();
			removeAllTreeNodes();
			
			if( manager.getSession().isGridlabRunning() ) {
				// now add the nodes
				GLObjectTree objTree = manager.getSession().getGLObjectTree();
				if(objTree != null){
					addChildNodes(rootTreeNode, GLObjectTree.ROOT, objTree, 0);
				}
			}
			
			if(lastSelected != null){
				GLObject globj = (GLObject)lastSelected.getUserObject();
				selectItemInTree(globj.getName(), true);
			}
		}
		else {
			// just update the nodes in the existing tree
			if( manager.getSession().isGridlabRunning() ) {
				//long time = System.currentTimeMillis();
				// now update the nodes
				java.util.List<GLObject> objList = manager.getSession().getGLObjectList();
				if(objList != null){
					for (GLObject obj : objList) {
						DefaultMutableTreeNode node = findTreeNodeByName(rootTreeNode, obj.getName());
						if( node != null){
							node.setUserObject(obj);
						}
					}
				}
				//System.out.println(" update list took " + (System.currentTimeMillis() - time) + " ms");
			}
			
		}
		
		enableControls();
	}

		/**
		 * recursive call to update all the nodes to the tree
		 */
		private void updateChildNodes (DefaultMutableTreeNode parent, String mapTreeParentName, GLObjectTree objTree, int level) {
			java.util.List<GLObject> children = objTree.get(mapTreeParentName);
			
			if( children != null ){
				for (GLObject child : children) {
					// add the new node
					DefaultMutableTreeNode childNode = new DefaultMutableTreeNode(child);
					treeModel.insertNodeInto(childNode, parent, parent.getChildCount());
		
					// make the node visible
					if( level < 2 ){
			            TreePath path = new TreePath(childNode.getPath());
			            tree.scrollPathToVisible(path);
					}
		            
		            // recurse to children
		            addChildNodes(childNode, child.getName(), objTree, level + 1);
				}
			}
			
		}
		
		
	private void removeAllTreeNodes() {
		// remove all nodes
		rootTreeNode.removeAllChildren();
		treeModel.reload();
	}
	
	/**
	 * recursive call to add all the nodes to the tree
	 */
	private void addChildNodes (DefaultMutableTreeNode parent, String mapTreeParentName, GLObjectTree objTree, int level) {
		java.util.List<GLObject> children = objTree.get(mapTreeParentName);
		
		if( children != null ){
			for (GLObject child : children) {
				// add the new node
				DefaultMutableTreeNode childNode = new DefaultMutableTreeNode(child);
				treeModel.insertNodeInto(childNode, parent, parent.getChildCount());
	
				// make the node visible
				if( level < 2 ){
		            TreePath path = new TreePath(childNode.getPath());
		            tree.scrollPathToVisible(path);
				}
	            
	            // recurse to children
	            addChildNodes(childNode, child.getName(), objTree, level + 1);
			}
		}
		
	}
	
	
	////////////// implement GLProjectListener
	public void notifyProjectChanged(GLProjectSettings proj, ProjectState state){
		switch(state) {
		    case NEW_PROJECT:
				this.setTitle("GLDebug - " + proj.getFileName());
				// On new project, auto open the project settings dialog
				projectMenuSettings_actionPerformed(null);
			    break;
			    
		    case PROJECT_CLOSED:
				this.setTitle("GLDebug");
				clearOutputText();
				break;
				
		    case PROJECT_OPENED:
				this.setTitle("GLDebug - " + proj.getFileName());
				clearOutputText();
				
				// after loading the project, start gridlab
	    		if(manager.getProjectSettings().getModelFiles().size() > 0){
		    		if(ensureGridLabExeSettings()){
	    				loadModel();
		    		}	
	    		}
				break;
				
		    case PROJECT_SAVED:
				this.setTitle("GLDebug - " + proj.getFileName());
				break;
				
		    case PROJECT_SETTINGS_CHANGE:
	    		// if we have model files, prompt the user to load/reload this project
	    		if(proj.getModelFiles().size() > 0){
		    		if(ensureGridLabExeSettings()){
		    			String msg = "";
		    			if(manager.getSession().isGridlabRunning()){
		    				msg = "Changes will not take affect until you reload the model(s).\n"
		    					+ "Reload now?";
		    			}
		    			else {
		    				msg = "Would you like to load the model(s) now?";
		    			}
		    			
		    			int val = JOptionPane.showConfirmDialog(this, msg,
		                        "Project Settings", JOptionPane.YES_NO_OPTION, JOptionPane.QUESTION_MESSAGE );
		    			if( val == JOptionPane.YES_OPTION ){
		    				loadModel();
		    			}
		    		}	
	    		}
		    	break;
		}
		
		enableControls();
	}
	
    
    private void addMessageOutput(ExecutionEvent evt) {
    	//long time = System.currentTimeMillis();
    	boolean error = false;
    	String msg = evt.getMessage();
    	
    	if( msg.startsWith("DEBUG")) {
    		txtDebugOutput.append(msg);
    	}
    	else if( msg.startsWith("ERROR") || msg.startsWith("FATAL")) {
    		error = true;
			txtErrorOutput.appendStyled(msg, Color.RED);
		}
		else if( msg.trim().startsWith("... ")){
    		txtVerboseOutput.append(msg);
		}

    	// everything goes to Console output
    	if(error){
        	txtConsoleOutput.appendStyled(/*evt.getMessageType() + ": " + */msg, Color.RED);
    	}
    	else {
        	txtConsoleOutput.append(/*evt.getMessageType() + ": " + */msg);// + "\r\n");
    	}
    	
    	//System.out.println("addMessageOutput took " + (System.currentTimeMillis() - time));
//    	String msg = evt.getMessage();
//    	if( msg.startsWith("Processing") ) {
//    		// ignore processing messages
//    		return;
//    	}
//    	
//    	switch (evt.getMessageType()) {
//        case ExecutionEvent.STDERR:
//        case ExecutionEvent.ERROR:
//        	// keep track of errors, we will only count things as errors if they start with these strings
//        	if( msg.startsWith("ERROR") || msg.startsWith("FATAL")) {
//        		errorMsgBuffer.append(evt.getMessage());
//        	}
//        	else {
//        		//switch these messages from error to output
//        		// This lets us deal with --verbose output which shows up on the error stream
//    			evt = new ExecutionEvent(evt.getSource(), evt.getMessage(), ExecutionEvent.STDOUT);
//        	}
//            break;
//        case ExecutionEvent.FINISHED:
//        case ExecutionEvent.HALTED:
//        	// if we had any errors, email
//        	if( errorMsgBuffer.length() > 0){
//        		Log.logError("GridLabId: " + gridLabId + "\n" + errorMsgBuffer.toString(), null, config.getSessionId());
//        	}
//            break;
//        }
    	
	}
	
    /**
     * this executes gridlab with the project parameters
     */
    private void loadModel() {
    	clearOutputText();
		manager.reloadProject();
		
	}

    /**
     * handles load/reload of project - executes gridlabd.exe
     */
	protected void do_debugMenuLoad_actionPerformed(final ActionEvent e) {
		loadModel();
	
	}
	protected void do_stopButton_actionPerformed(final ActionEvent e) {
		manager.getSession().postBreak();
	}
	protected void do_runButton_actionPerformed(final ActionEvent e) {
		//manager.getSession().run();
		manager.getSession().queueCommand(new GLCommand(GLCommandName.RUN));
	}
	protected void do_exitButton_actionPerformed(final ActionEvent e) {
		processWindowEvent(new WindowEvent(this, WindowEvent.WINDOW_CLOSING));
	}
	protected void do_debugMenuRun_actionPerformed(final ActionEvent e) {
		do_runButton_actionPerformed(e);
	}
	protected void do_debugMenuRunTo_actionPerformed(final ActionEvent e) {
		do_runToButton_actionPerformed(e);
	}
	protected void do_debugMenuStop_actionPerformed(final ActionEvent e) {
		do_stopButton_actionPerformed(e);
	}
	protected void do_debugMenuStep_actionPerformed(final ActionEvent e) {
		do_stepButton_actionPerformed(e);
	}
	protected void do_runToButton_actionPerformed(final ActionEvent e) {
		
	}
	
	protected void do_toolsMenuSettings_actionPerformed(final ActionEvent e) {
		SettingsDlg dlg = new SettingsDlg(this);
		dlg.setGridlabExePath(manager.getSession().getGlExePath());
		dlg.setVisible(true);
		if( !dlg.isCanceled()){
			if(!dlg.getGridlabExePath().equals(manager.getSession().getGlExePath())) {
				manager.getSession().setGlExePath(dlg.getGridlabExePath());
			}
		}
		
	}
	protected void do_stepButton_actionPerformed(final ActionEvent e) {
		manager.getSession().step((GLStepType)comboBoxStepBy.getSelectedItem());
		enableControls();
	}
	
	protected void do_this_windowClosing(final WindowEvent e) {
		UserPreferences.instanceOf().saveWindowLocation(this, "Main", "mainForm");
	}

	/**
	 * handle tree nodes selected
	 */
	public void valueChanged(TreeSelectionEvent e) {
		if(doTreeUpdateOnSelection) {
			DefaultMutableTreeNode node = (DefaultMutableTreeNode)tree.getLastSelectedPathComponent();
			
			if (node != null){
				Object obj = node.getUserObject();
				if( obj instanceof GLObject ) {
					System.out.println("Selected: " + obj);
					updateObjectDetails(((GLObject)obj).getName());
				}
			}
		}
		
	}

	/**
	 * request the details of the selected object
	 * @param name
	 */
	private void updateObjectDetails(String name) {
		manager.getSession().queueCommand(new GLCommand(GLCommandName.PRINTOBJ, name));
	}
	
	protected void do_mbtnModifyGlobals_actionPerformed(final ActionEvent e) {
	}
	
	/**
	 * called when the selected tab changes (details, globals, watches...)
	 * @param e
	 */
	protected void do_mtabPaneDetails_stateChanged(final ChangeEvent e) {
		// make sure globals are up to date
		updateGlobals();
		
	}
			
}
