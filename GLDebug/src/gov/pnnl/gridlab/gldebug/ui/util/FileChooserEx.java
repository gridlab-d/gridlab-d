package gov.pnnl.gridlab.gldebug.ui.util;


/**
 * <p>Title: </p>
 * <p>Description:
 *  makes adding things like filters/preview to a JFileChooser easy
* </p>
 * <p>Copyright: Copyright (c) 2004</p>
 * <p>Company: Battelle Memorial Institute</p>
 * @author Jon McCall
 * @version 1.0
 */
import java.beans.*;
import java.io.*;
import java.net.*;
import java.util.*;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;
import javax.swing.filechooser.FileFilter;

/**
 * Makes it easy to add filters.  Also includes text file preview.
 */
public class FileChooserEx extends JFileChooser {
    private static FileChooserEx mFileChooserEx=null;
    private File[] mSelectedFiles = null;

    private static String mOriginalApproveText = null;
    private static String mOriginalTitle = null;
    private boolean mbCheckForFileExistance = false;


    /**
     * Add a file filter: <BR>
     * Example: addFilter( "txt", "Text Files (*.txt)" )
     */
    public void addFilter( String suffix, String sDescription ) {
        GenericFilter filter = new GenericFilter( new String[] {suffix}, sDescription );
        this.addChoosableFileFilter(filter);
    }

    /**
     * Add a file filter for multiple file types: <BR>
     * Example: addFilter( new String[] { "txt", "doc" }, "Doc files (*.txt, *.doc)" ) ;
     */
    public void addFilter( String [] suffix, String sDescription ) {
        GenericFilter filter = new GenericFilter( suffix, sDescription );
        this.addChoosableFileFilter(filter);
    }

    /**
     * returns a string that can be saved in a preferences file that specifies the
     * list of file filters and the default filter.
     *
     * Example: "Dat files(*.dat)<txt|Text files(*.txt)<dat|Dat files(*.dat)"
     */
    public String getFiltersDefinition () {
        StringBuffer sb = new StringBuffer();

        // get the default filter
        sb.append(this.getSelectedFilter());

        // now add all the filter definitions
        String []filters = this.getFilters();
        for (int i=0; i < filters.length ; i++ ) {
            sb.append("<");
            sb.append(filters[i]);
        }
        return sb.toString();
    }

    /**
     * sets the filters and the last filter used (use getFilterList).
     */
    public void setFilterDefinition (String filters) {
        StringTokenizer tok = new StringTokenizer(filters, "<");
        int cnt=0;
        String defaultFilter = "";

        while (tok.hasMoreTokens()) {
            String item = tok.nextToken();
            if( cnt++ == 0 ){
                defaultFilter = item;
            }
            else {
                StringTokenizer subtok = new StringTokenizer(item, "|");
                int subcnt = subtok.countTokens();
                if( subcnt > 1 ) {
                    String suffixes[] = new String[subcnt - 1];
                    for (int i=0; i < suffixes.length; i++ ) {
                        suffixes[i] = subtok.nextToken();
                    }

                    String desc = subtok.nextToken();
                    addFilter(suffixes, desc);
                }
            }
        }

        this.selectFilter(defaultFilter);
    }



    /**
     * tries to select a file filter based on the description.  This becomes the
     * filter the used when the dialog is opened.
     */
    public void selectFilter (String sDescription) {
        javax.swing.filechooser.FileFilter [] filters = this.getChoosableFileFilters();

        for (int i=0; i < filters.length; i++ ) {
            if( filters[i].getDescription().equals(sDescription) ) {
                this.setFileFilter(filters[i]);
                return;
            }
        }
    }


    /**
     * returns a list of all filters used (that we added, does not include *.* filter).
     * The format is:
     *      ext1|ext2|ext3|description
     */
    private String[] getFilters () {

        javax.swing.filechooser.FileFilter [] filters = this.getChoosableFileFilters();

        java.util.List items = new ArrayList<String>();
        for (int i=0; i < filters.length; i++ ) {
            if( filters[i] instanceof GenericFilter )
                items.add( ((GenericFilter)filters[i]).toString());
        }

        String values[] = new String[items.size()];
        for (int i=0; i < values.length; i++ ) {
            values[i] = (String)items.get(i);
        }

        return values;
    }

    /**
     * returns the currently selected file filter description
     */
    private String getSelectedFilter () {
        return this.getFileFilter().getDescription();
    }

    /**
     * override approve selection to support changing the file filter.
     * If we find the "*." character sequence at the start of the file name, we assume this is a file
     * filter and add it to the list of filters.
     */
    public void approveSelection() {
        String name = this.getSelectedFile().getName();
        if( name.startsWith("*.") && name.length() > 2 ) {
            String ext = name.substring(2);
            String desc =  ext + " Files (" + name + ")";
            addFilter(ext, desc);
        }
        else {
            //    the following code is an attempt to fix bug
            //    Bug Id 4356160
            //    http://developer.java.sun.com/developer/bugParade/bugs/4356160.html
            //    according to the suggested work around, however, the work around does not work and instead
            //    throws an exception when you attempt to get the link location.
//            // check if it is really a link to a file or a folder
//            if (File.separatorChar == '\\') {
//                File selectedFile = getSelectedFile();
//                if (selectedFile.exists() && selectedFile.getPath().endsWith(".lnk")) {
//                    File linkedTo = null;
//                    try {
//                        ShellFolder folder = ShellFolder.getShellFolder(selectedFile);
//                        System.out.println("folder.getAbsoluteFile()=" + folder.getAbsoluteFile());
//                        System.out.println("folder.isLink()" + folder.isLink());
//                        System.out.println("folder.getParent()" + folder.getParent());
//
//                        ShellFolder link = folder.getLinkLocation();
//                        //linkedTo = ShellFolder.getShellFolder(selectedFile).getLinkLocation();
//                    }
//                    catch (FileNotFoundException ignore) {
//                        ignore.printStackTrace();
//                    }
//                    if (linkedTo != null) {
//                        if (linkedTo.isDirectory()) {
//                            setCurrentDirectory(linkedTo);
//                            return;
//                        }
//                        else if (!linkedTo.equals(selectedFile)) {
//                            // actually link to the real file
//                            setSelectedFile(linkedTo);
//                        }
//                    }
//                }
//            }


            if( mbCheckForFileExistance ) {
                if( !this.getSelectedFile().exists() ) {
                    JOptionPane.showMessageDialog(this, "That file does not exist");
                    return;
                }
            }

            super.approveSelection();
        }
    }

//    /**
//     * check if the file is a link to a windows folder...
//     */
//    public boolean isLinkToFolder (File file){
//        if (File.separatorChar == '\\') {
//
//            File selectedFile = getSelectedFile();
//            if (selectedFile.getPath().endsWith(".lnk")) {
//                File linkedTo = null;
//                try {
//                    linkedTo = ShellFolder.getShellFolder(selectedFile).getLinkLocation();
//                } catch (FileNotFoundException ignore) {
//                }
//                if (linkedTo != null) {
//                    if (linkedTo.isDirectory()) {
//                        setCurrentDirectory(linkedTo);
//                        return;
//                    } else if (!linkedTo.equals(selectedFile)) {
//                        setSelectedFile(linkedTo);
//                    }
//                }
//            }
//        }
//        return false;
//    }


    /**
     * override this method to cache the selected files
     */
    public void setSelectedFiles(File[] selectedFiles) {
        super.setSelectedFiles(selectedFiles);
        mSelectedFiles = selectedFiles;
    }


    /**
     * override this method so that we can actually select files in the
     * chooser dialog.
     */
    public synchronized int showDialog(Component parent, String approveButtonText) {
        if( mSelectedFiles != null ) {
            // start a thread that monitors when the chooser becomes active
            MyMonitor mon = new MyMonitor(mSelectedFiles, this);
            try {
                mon.start();
                wait(100);
            }
            catch (Exception ex) {
                ex.printStackTrace();
            }
        }
        return super.showDialog(parent, approveButtonText );
    }




    /**
     * turn text file preview on
     */
    public void addTextFilePreviewer() {
        this.setAccessory( new TextPreviewPanel(this) );
    }

    private FileChooserEx() {
        mOriginalApproveText = super.getApproveButtonText();
        mOriginalTitle = super.getDialogTitle();
    }

    /**
     *  implement reuse for this class -
     *  since it's a modal dialog, as long as you set it up right
     *  before use (ie. it is shown) we are OK to share this class.
     */
    public static FileChooserEx fetchFileChooserEx () {
        // create a new one first time through
        if( mFileChooserEx == null ) {
            mFileChooserEx = new FileChooserEx();
        }
        // restore to initialized state
        mFileChooserEx.setAccessory( null );
        mFileChooserEx.setApproveButtonToolTipText(null);
        mFileChooserEx.resetChoosableFileFilters();
        mFileChooserEx.setFileSelectionMode( JFileChooser.FILES_ONLY );
        mFileChooserEx.setSelectedFile( null );
        mFileChooserEx.setApproveButtonText(mOriginalApproveText);
        mFileChooserEx.setDialogTitle(mOriginalTitle);
        mFileChooserEx.mbCheckForFileExistance = false;
        mFileChooserEx.setMultiSelectionEnabled(false);
        mFileChooserEx.mSelectedFiles = null;
        return mFileChooserEx;
    }

    /**
     * Override the default size so that the preview does not cause problems.
     */
    public Dimension getPreferredSize() {
        // the default turns out to be about 508x327, up it a little.
        return new Dimension( 625, 335 );
    }

    /**
     * utility function to make sure a file name chosen has the extension given.<br>
     * Useful when picking a save file name to make sure the user has given an
     * appropriate extension, or it not, make sure we have the extension. <P>
     *
     * returns the file name string with extension.<P>
     *
     * Example: String fname = ensureFileExtension( "bob", ".txt", true ) <BR>
     *  returns fname = "bob.txt"<P>
     */
    public static String ensureFileExtension ( String fileName, String ext, boolean caseSensitive ) {
        if( fileName.length() < ext.length() )
            return fileName + ext;

        String retName=fileName;
        String existingExt = fileName.substring( fileName.length() - ext.length() );


        if( existingExt.compareToIgnoreCase(ext) == 0 ){
            if( caseSensitive ) {
                if( !existingExt.equals(ext) ){
                    retName = fileName.substring( 0, fileName.length() - ext.length() ) + ext;
                }
            }
        }
        else {
            retName = fileName + ext;
        }
        return retName;
    }


    /**
     * if set, then the file/directory must exist for the approve button to
     * work.
     */
    public void setCheckForFileExistance(boolean mbCheckForFileExistance) {
        this.mbCheckForFileExistance = mbCheckForFileExistance;
    }

}

/**
 * Handles the file extension filtering
 */
class GenericFilter extends javax.swing.filechooser.FileFilter {
    private String [] mSuffix=null;
    private String mDescription;

    public GenericFilter( String [] sSuffix, String sDescription ) {
        mSuffix = (String [])sSuffix.clone();
        mDescription = sDescription;
    }

    public boolean accept(File f) {
        boolean acceptIt = f.isDirectory();

        if( !acceptIt ) {
          String suffix = getSuffix(f);
          if( suffix != null ){
              for( int i=0; i < mSuffix.length; i++ ) {
                   acceptIt = suffix.equals(mSuffix[i]);
                   if( acceptIt )
                       break;
              }
          }
        }
        return acceptIt;
    }

    public String getDescription() {
        return mDescription;
    }

    private String getSuffix(File f) {
        String s = f.getPath();
        String suffix = null;
        int i = s.lastIndexOf('.');

        if (i > 0 && i < s.length() - 1 ) {
           suffix = s.substring(i+1).toLowerCase();
        }
        return suffix;
    }

    /**
     * build a special formatted string
     * "exe|dat|dat||my special files(*.exe, *.dat)"
     */
    public String toString () {
        String ext = "";
        StringBuffer sb = new StringBuffer();
        for (int i=0; i < mSuffix.length; i++ ) {
            sb.append(ext);
            sb.append(mSuffix[i]);
            if( i == 0 ) {
                ext = "|";
            }
        }

        sb.append("|");
        sb.append(mDescription);
        return sb.toString();
    }

}

/**
 * class to create a text previewer panel
 */
class TextPreviewPanel extends JPanel {
    private TextPreviewer mPreviewer = new TextPreviewer();
    private JLabel label = new JLabel("File Preview", SwingConstants.CENTER);
    private JFileChooser mChooser;
////
//     private boolean bShown=false;

    public TextPreviewPanel(JFileChooser chooser) {
        mChooser = chooser;
        setPreferredSize(new Dimension(350,0));
        setBorder(BorderFactory.createEtchedBorder());
        setLayout( new BorderLayout() );

        label.setBorder(BorderFactory.createEtchedBorder());
        add(label, BorderLayout.NORTH);
        add(mPreviewer, BorderLayout.CENTER);

        mChooser.addPropertyChangeListener( new PropertyChangeListener() {
            public void propertyChange(PropertyChangeEvent e) {
                if( e.getPropertyName().equals( JFileChooser.SELECTED_FILE_CHANGED_PROPERTY) ||
                    e.getPropertyName().equals( JFileChooser.SELECTED_FILES_CHANGED_PROPERTY) ||
                       e.getPropertyName().equals( JFileChooser.DIRECTORY_CHANGED_PROPERTY ) ) {

                    File f = null;
                    if( mChooser.getSelectedFiles().length > 1 ) {
                        // try to select the last file picked
                        f = mChooser.getSelectedFiles()[mChooser.getSelectedFiles().length-1];
                    }
                    else if( e.getNewValue() instanceof File ) {
                        f = (File)e.getNewValue();
                    }

                    if( f != null ) {
                        if( f.isFile() )
                            label.setText( "File Preview: " + f.getName() );
                        else
                            label.setText( "File Preview" );
                        mPreviewer.configure(f);
                    }
                }
            }
        });
    }

}

class TextPreviewer extends JComponent {
    private JTextArea textArea = new JTextArea();
    private JScrollPane scrollPane = new JScrollPane(textArea);

    public TextPreviewer() {
        textArea.setEditable(false);

        setBorder(BorderFactory.createEtchedBorder());
        setLayout(new BorderLayout() );
        add(scrollPane, BorderLayout.CENTER);
    }

    public void configure(File file) {
        textArea.setText(contentsOfFile(file));

        SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                JViewport vp = scrollPane.getViewport();
                vp.setViewPosition(new Point(0,0));
                textArea.setCaretPosition(0);
            }
        });
    }

    static String contentsOfFile( File file) {
        String s = new String();
        char[] buff = new char[32000];
        InputStream is;
        InputStreamReader reader;
        URL url;

        try {
            if( file.isFile() ) {
                reader = new FileReader(file);
                int nch;
                //while(( nch = reader.read(buff, 0, buff.length)) != -1 ) {
                //    s = s + new String(buff, 0, nch);
                //}

                // just show first little bit of the file
                nch = reader.read(buff, 0, buff.length);
                s = new String(buff, 0, nch);
                if( reader.read() != -1 )
                    s = s + "\n...";
            }
        }
        catch (Exception ex) {
            s = "Unable to view file";
        }
        return s;
    }
}

/**
 * The purpose of this class is to select files in a JFileChooser.
 * I know, a lot of work just to do that, but it was annoying.  Anyway,
 * this class runs as a separate thread, waiting for a new dialog to
 * become active.  Once that happens, we try to find a JList in the dialog
 * containing Files.  If we find this list, we then attempt to select the
 * items files that should be selected.
 */
class MyMonitor extends Thread implements AWTEventListener {
    private File[] mSelectedFiles = null;
    Object signal = new Object();
    FileChooserEx chooser;

    public MyMonitor(File[]files, FileChooserEx chooser) {
        mSelectedFiles = files;
        this.chooser = chooser;
    }

    public void run() {
        // listen for window events
        Toolkit.getDefaultToolkit().addAWTEventListener(this, AWTEvent.WINDOW_EVENT_MASK);

        // now wait for the signal
        try {
            synchronized (signal) {
                signal.wait();
            }
            sleep(150);
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }

        // stop listening
        Toolkit.getDefaultToolkit().removeAWTEventListener(this);

        // the dialog has opened, select the files (on the EventThread)
        SwingUtilities.invokeLater(new Runnable() {
            public void run() {
                selectFilesInChooser();
            }
        });
    }

     // implement AWTEventListener
     public void eventDispatched(AWTEvent event) {
        if( event instanceof WindowEvent ) {
            WindowEvent wevt = (WindowEvent)event;
            if( wevt.getID() == WindowEvent.WINDOW_ACTIVATED ) {
                // the window was activiated, signal
                synchronized(signal) {
                    signal.notify();
                }
            }
        }
     }

     /**
      * select the file chooser items
      */
     private void selectFilesInChooser () {
        Map newSelects = new HashMap(mSelectedFiles.length);
        for (int i=0; i < mSelectedFiles.length; i++ ) {
            newSelects.put(mSelectedFiles[i], null);
        }


        // Get all the JLists from the file chooser.
        Vector v = new Vector();
        getJListsInContainer(v, chooser);

        // Now figure out which JList has the selected files.
        JList theFileList = null;
        Enumeration enumer = v.elements();
        while (enumer.hasMoreElements() && (theFileList == null)) {
            JList list = (JList)enumer.nextElement();
            // Assume the first one containing Files is it.
            if (containsFiles(list))
                theFileList = list;
        }


        if (theFileList != null) {
            theFileList.clearSelection();
            // found the JList
            for (int i=0; i < theFileList.getModel().getSize(); i++ ) {
                if( newSelects.containsKey(theFileList.getModel().getElementAt(i)) ) {
                    // add this row to the selection
                    theFileList.addSelectionInterval(i, i);
                }
            }
        }
    }

   // Check to see if a JList contains some Files.
   private static boolean containsFiles (JList list) {
      ListModel model = list.getModel();
      int n = (model != null ? model.getSize() : 0);
      if (n > 0) {
         for (int i = 0; i < n; i++) {
            Object ob = model.getElementAt(i);
            if (ob instanceof File)
               return  true;
         }
      }
      return  false;
   }

   // Recursively find all the JLists in a container.
   private static void getJListsInContainer (Vector v, Container c) {
      Component[] comps = c.getComponents();
      if (comps != null) {
         for (int i = 0; i < comps.length; i++) {
            if (comps[i] instanceof JList)
               v.add(comps[i]);
            else if (comps[i] instanceof Container)
               getJListsInContainer(v, (Container)comps[i]);
         }
      }
   }



}
