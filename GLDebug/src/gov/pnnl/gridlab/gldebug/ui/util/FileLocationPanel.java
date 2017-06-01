package gov.pnnl.gridlab.gldebug.ui.util;

/**
 * <p>Title: </p>
 * <p>Description: </p>
 * <p>Copyright: Copyright (c) 2003</p>
 * <p>Company: Battelle Memorial Institute</p>
 * @author
 * @version 1.0
 */


/*
 * Title:
 * Description:
 *
 * Copyright 2001 Battelle Memorial Institute and OmniViz, Inc.
 * All rights reserved. This computer program contains the confidential
 * information of Battelle and OmniViz. Use or duplication in any form
 * is subject to conditions set forth in a separate license agreement.
 * Unauthorized use or duplication may violate the license agreement,
 * state, federal and/or international laws including the Copyright Laws
 * of the United States and of other international jurisdictions.
 *
 * @author  Jon McCall

 */


import  java.awt.*;
import java.awt.GridBagConstraints;
import javax.swing.*;
import java.awt.event.*;
import java.io.*;

import java.util.*;
import java.awt.dnd.*;
import java.awt.datatransfer.*;
import javax.swing.tree.*;

/**
 * this class implements a text field with a file browser button next to it
 * for imput of a file/directory location.
 */
public class FileLocationPanel extends JPanel {
    JTextField mtxtPath = new JTextField();
    JButton mbtnBrowse = new JButton();
    GridBagLayout gridBagLayout1 = new GridBagLayout();
    private String[] sFilterSuffixes = new String[0];
    private String sFilterDescription = "";
    private String mApproveButtonText = null;
    private int mMode=-1;
    private boolean mInTable=true;
    private transient Vector actionListeners;
    private String msLastPath = "";
    private String mDefaultDir;

    public FileLocationPanel () {
        this(false);
    }

    /**
     * if in a JTable, we don't want to show the border around the text area.  So
     * set to true if you are using this in a table.
     */
    public FileLocationPanel (boolean bInTable) {
        try {
            mInTable = bInTable;
            jbInit();
            setupDnD();
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }

    /**
     *
     */
    public void setEnabled (boolean b){
        mtxtPath.setEnabled(b);
        mbtnBrowse.setEnabled(b);
    }

    /**
     *
     */
    public void setEditable(boolean b){
        mtxtPath.setEditable(b);
        mbtnBrowse.setEnabled(b);
    }

    /**
     *
     */
    public void requestFocus() {
        mtxtPath.requestFocus();
    }


    /**
     *
     */
    public void setDefaultDir (String dir){
        mDefaultDir = dir;
    }


    /**
     * see the description of FileChooserEx.addFilter for the parameters
     */
    public void addFileFilter ( String [] suffix, String sDescription ) {
        sFilterSuffixes = suffix;
        sFilterDescription = sDescription;
    }

    /**
     * allow us to change from just picking files to also pick directories.
     * (chooser.setFileSelectionMode( JFileChooser.FILES_AND_DIRECTORIES ));
     */
    public void setFileSelectionMode (int mode) {
        mMode = mode;
    }

    /**
     *
     */
    public void setApproveButtonText (String s){
        mApproveButtonText = s;
    }




    void jbInit () throws Exception {
        if( mInTable )
            mtxtPath.setBorder(null);
        mtxtPath.setText("");
        mtxtPath.addFocusListener(new java.awt.event.FocusAdapter() {
            public void focusLost(FocusEvent e) {
                mtxtPath_focusLost(e);
            }
        });
        mtxtPath.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(ActionEvent e) {
                mtxtPath_actionPerformed(e);
            }
        });
        this.setLayout(gridBagLayout1);
        mbtnBrowse.setMaximumSize(new Dimension(19, 21));
        mbtnBrowse.setMinimumSize(new Dimension(19, 21));
        mbtnBrowse.setPreferredSize(new Dimension(19, 21));
        mbtnBrowse.setToolTipText("Browse");
        mbtnBrowse.setMargin(new Insets(2, 2, 2, 2));
        mbtnBrowse.setText("...");
        mbtnBrowse.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(ActionEvent e) {
                mbtnBrowse_actionPerformed(e);
            }
        });
        this.add(mtxtPath,     new GridBagConstraints(0, 0, 1, 1, 1.0, 1.0
            ,GridBagConstraints.NORTHWEST, GridBagConstraints.HORIZONTAL, new Insets(0, 0, 0, 0), 0, 0));
        this.add(mbtnBrowse,          new GridBagConstraints(1, 0, 1, 1, 0.0, 0.0
            ,GridBagConstraints.NORTH, GridBagConstraints.NONE, new Insets(0, 3, 0, 0), 0, 0));
    }

    public void setFileName( String name ) {
        mtxtPath.setText(name);
        msLastPath = name;
    }

    public String getFileName() {
        return mtxtPath.getText().trim();
    }

//    /**
//     * opens the file dialog
//     */
//    public void browseForFile (){
//        browseLocal();
//    }


    /**
     * handle the file browsing
     */
    void mbtnBrowse_actionPerformed(ActionEvent evt) {
        browseLocal();
    }


    private void browseLocal() throws HeadlessException {
        FileChooserEx chooser = FileChooserEx.fetchFileChooserEx();
        if( mApproveButtonText != null ) {
            chooser.setApproveButtonText(mApproveButtonText);
        }

        if( getFileName().length() > 0  ) {
            File f = new File(getFileName());
            chooser.setCurrentDirectory( f );
            chooser.setName( getFileName() );
            chooser.setSelectedFile(f);
        }
        else if( mDefaultDir != null && mDefaultDir.length() > 0 ){
            File f = new File(mDefaultDir);
            chooser.setCurrentDirectory(f);
        }

        if( mMode > -1 ){
            chooser.setFileSelectionMode(mMode);
            if( mMode == JFileChooser.DIRECTORIES_ONLY ) {
                // change the text of the approve button to select
                chooser.setApproveButtonText("Select");
                chooser.setDialogTitle("Select a directory");
                if( getFileName().length() > 0  ) {
                    // move up one directory then select the dir
                    chooser.changeToParentDirectory();
                    File dir = new File(getFileName());
                    chooser.setSelectedFile(dir);
                }
            }
        }

        if( sFilterSuffixes.length > 0 ) {
            chooser.addFilter( sFilterSuffixes, sFilterDescription );
        }

        int retVal = chooser.showOpenDialog(this);
        if (chooser.getSelectedFile() != null && retVal == FileChooserEx.APPROVE_OPTION ) {
            setFileName( chooser.getSelectedFile().getAbsolutePath() );
            fireActionPerformed(new ActionEvent(this, ActionEvent.ACTION_PERFORMED, ""));
        }
    }
    public synchronized void removeActionListener(ActionListener l) {
        if (actionListeners != null && actionListeners.contains(l)) {
            Vector v = (Vector) actionListeners.clone();
            v.removeElement(l);
            actionListeners = v;
        }
    }
    public synchronized void addActionListener(ActionListener l) {
        Vector v = actionListeners == null ? new Vector(2) : (Vector) actionListeners.clone();
        if (!v.contains(l)) {
            v.addElement(l);
            actionListeners = v;
        }
    }
    protected void fireActionPerformed(ActionEvent e) {
        if (actionListeners != null) {
            Vector listeners = actionListeners;
            int count = listeners.size();
            for (int i = 0; i < count; i++) {
                ((ActionListener) listeners.elementAt(i)).actionPerformed(e);
            }
        }
    }

    void mtxtPath_actionPerformed(ActionEvent e) {
        fireActionPerformed(new ActionEvent(this, ActionEvent.ACTION_PERFORMED, ""));
    }

    void mtxtPath_focusLost(FocusEvent e) {
        String path = mtxtPath.getText();
        if( msLastPath == null || !msLastPath.equals(path) ) {
            msLastPath = path;
            fireActionPerformed(new ActionEvent(this, ActionEvent.ACTION_PERFORMED, ""));
        }
    }


    /**
     *  setup so we can do drag and drop of files to the dataset list
     */
    public void setupDnD () {
        DropTargetListener dropListener = new DropTargetListener() {
            public void dragEnter(DropTargetDragEvent dtde) {
                dragCheck(dtde);
            }
            public void dragOver(DropTargetDragEvent dtde){
                dragCheck(dtde);
            }
            public void dropActionChanged(DropTargetDragEvent dtde){
                dragCheck(dtde);
            }
            public void dragExit(DropTargetEvent dte){
            }
            public void drop(DropTargetDropEvent dtde){
                if( !dtde.isDataFlavorSupported(DataFlavor.javaFileListFlavor) ) {
                    dtde.rejectDrop();
                }
                else {
                    try {
                        // get the files dropped
                        dtde.acceptDrop(DnDConstants.ACTION_COPY_OR_MOVE);
                        Transferable tr = dtde.getTransferable();
                        java.util.List fileList = (java.util.List)tr.getTransferData(DataFlavor.javaFileListFlavor);

                        // only take the first file
                        if( fileList.size() > 0 ) {
                            File file = (File) fileList.get(0);
                            setFile(file);
                        }

                        // tell source that we are done
                        dtde.getDropTargetContext().dropComplete(true);
                    }
                    catch (Exception ex) {
                        System.err.println("Error (DataSetEditor.drop): " + ex.toString() );

                    }
                }
            }

        };

        // have to add both the text area as a drop target.
        DropTarget dt = new DropTarget (mtxtPath, DnDConstants.ACTION_COPY_OR_MOVE, dropListener, true);

    }

    /**
     *
     */
    private void setFile (File file){
        mtxtPath.setText(file.getAbsolutePath());

    }
    
    /**
     * override
     */
    public void setToolTipText(String t) {
    	super.setToolTipText(t);
		mtxtPath.setToolTipText(t);
	}


    /**
     * check if the drag event is acceptable or not
     */
    private void dragCheck (DropTargetDragEvent dtde ) {
        if( !dtde.isDataFlavorSupported(DataFlavor.javaFileListFlavor) ) {
            dtde.rejectDrag();
        }
        else {
            dtde.acceptDrag(DnDConstants.ACTION_COPY_OR_MOVE);
        }
    }

}



