package gov.pnnl.gridlab.gldebug.ui;

import gov.pnnl.gridlab.gldebug.ui.util.*;
import java.awt.*;
import javax.swing.*;
import java.awt.event.*;
import java.io.File;


public class SettingsDlg extends JDialog {
	
	private FileLocationPanel pnlGridlabExe;
	private JLabel gridlabexePathLabel;
	private JPanel panel1 = new JPanel();
	private JPanel mpnlOkCancel = new JPanel();
	private boolean canceled = true;
	private JButton mbtnOK = new JButton();
	private JButton mbtnCancel = new JButton();
	private GridBagLayout gridBagLayout2 = new GridBagLayout();
	private JPanel mpnlMain = new JPanel();
	private GridBagLayout gridBagLayout3 = new GridBagLayout();
	
	public SettingsDlg(Frame frame) {
		super(frame, "", true);
		try {
			jbInit();
			
			// setup default buttons
			getRootPane().setDefaultButton(mbtnOK);
			KeyStroke stroke = KeyStroke.getKeyStroke(KeyEvent.VK_ESCAPE, 0);
			rootPane.registerKeyboardAction(new ActionListener() {
				public void actionPerformed(ActionEvent e) {
					mbtnCancel_actionPerformed(e);
				}
			}
			, stroke, JComponent.WHEN_IN_FOCUSED_WINDOW);
			
			pnlGridlabExe.addFileFilter(new String[] { "exe" }, "Executable Files (*.exe)");
			
			
			pack();
			this.setSize(400, 150);
			Utils.centerOver(this, frame);
		}
		catch(Exception ex) {
			ex.printStackTrace();
		}
	}
	
	
	private void jbInit() throws Exception {
		panel1.setLayout(gridBagLayout2);
		mbtnOK.setText("OK");
		mbtnOK.addActionListener(new java.awt.event.ActionListener() {
			public void actionPerformed(ActionEvent e) {
				mbtnOK_actionPerformed(e);
			}
		});
		mbtnCancel.setText("Cancel");
		mbtnCancel.addActionListener(new java.awt.event.ActionListener() {
			public void actionPerformed(ActionEvent e) {
				mbtnCancel_actionPerformed(e);
			}
		});
		this.setDefaultCloseOperation(javax.swing.WindowConstants.DISPOSE_ON_CLOSE);
		this.setModal(true);
		this.setTitle("GLDebug Settings");
		mpnlOkCancel.setLayout(gridBagLayout3);
		getContentPane().add(panel1);
		panel1.add(mpnlOkCancel,    new GridBagConstraints(0, 3, 1, 1, 1.0, 0.0
				,GridBagConstraints.CENTER, GridBagConstraints.BOTH, new Insets(12, 12, 12, 12), 0, 0));
		mpnlOkCancel.add(mbtnOK,         new GridBagConstraints(0, 0, 1, 1, 1.0, 0.0
				,GridBagConstraints.EAST, GridBagConstraints.NONE, new Insets(0, 12, 0, 0), 0, 0));
		mpnlOkCancel.add(mbtnCancel,       new GridBagConstraints(1, 0, 1, 1, 0.0, 0.0
				,GridBagConstraints.CENTER, GridBagConstraints.NONE, new Insets(0, 6, 0, 0), 0, 0));
		panel1.add(mpnlMain,   new GridBagConstraints(0, 0, 1, 1, 1.0, 1.0
				,GridBagConstraints.CENTER, GridBagConstraints.BOTH, new Insets(0, 0, 0, 0), 0, 0));
		final GridBagLayout gridBagLayout = new GridBagLayout();
		gridBagLayout.columnWidths = new int[] {0,7};
		mpnlMain.setLayout(gridBagLayout);
		final GridBagConstraints gridBagConstraints = new GridBagConstraints();
		gridBagConstraints.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints.gridx = 0;
		gridBagConstraints.gridy = 0;
		gridBagConstraints.insets = new Insets(10, 10, 0, 0);
		gridlabexePathLabel = new JLabel();
		gridlabexePathLabel.setText("Gridlab.exe Path:");
		mpnlMain.add(gridlabexePathLabel, gridBagConstraints);

		final GridBagConstraints gridBagConstraints_1 = new GridBagConstraints();
		gridBagConstraints_1.insets = new Insets(10, 5, 0, 10);
		gridBagConstraints_1.anchor = GridBagConstraints.NORTHWEST;
		gridBagConstraints_1.fill = GridBagConstraints.HORIZONTAL;
		gridBagConstraints_1.weighty = 1.0;
		gridBagConstraints_1.weightx = 1.0;
		gridBagConstraints_1.gridy = 0;
		gridBagConstraints_1.gridx = 1;
		pnlGridlabExe = new FileLocationPanel();
		JPanel tempPanel = new JPanel();
		tempPanel.setLayout(new BorderLayout());
		tempPanel.add(pnlGridlabExe);
		
		mpnlMain.add(tempPanel, gridBagConstraints_1);
		
	}
	
	void mbtnOK_actionPerformed(ActionEvent e) {
		String path = getGridlabExePath();
		if(path.length() == 0) {
			JOptionPane.showMessageDialog(this, "Path is required.");
			return;
		}
		File f = new File(path);
		if(!f.exists() || f.isDirectory()) {// || !f.canExecute()) {
			JOptionPane.showMessageDialog(this, "Please enter a valid path.");
			return;
		}
		
		canceled = false;
		processWindowEvent(new WindowEvent(this, WindowEvent.WINDOW_CLOSING));
	}
	
	void mbtnCancel_actionPerformed(ActionEvent e) {
		canceled = true;
		processWindowEvent(new WindowEvent(this, WindowEvent.WINDOW_CLOSING));
	}
	
	/**
	 *
	 */
	public boolean isCanceled() {
		return canceled;
	}


	public String getGridlabExePath() {
		return pnlGridlabExe.getFileName().trim();
	}


	public void setGridlabExePath(String gridlabExePath) {
		pnlGridlabExe.setFileName(gridlabExePath);
	}
	
}
