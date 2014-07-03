package gov.pnnl.gridlab.gldebug.ui;

import gov.pnnl.gridlab.gldebug.model.GLGlobalList;
import gov.pnnl.gridlab.gldebug.model.GLGlobalList.Property;

import javax.swing.JTable;
import javax.swing.table.*;

public class GlobalsTable extends JTable {

    private GLGlobalList props;
	
	   /**
	   *
	   */
	  public void setupTable (){
	      //mtbImages.setRowSelectionAllowed(false);
	      //mtblFonts.getSelectionModel().setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
	      this.getTableHeader().setReorderingAllowed(false);
	      final JTable table = this;

	      // table model
	      this.setModel(new AbstractTableModel() {

	          public int getRowCount () {
	       	   if( props == null || table.isEnabled() == false)
	                  return 0;
	              return props.getProps().size();
	          }

	          public int getColumnCount () {
	              return 2;
	          }

	          public Object getValueAt (int row, int col) {
	        	  Property item = (Property)props.getProps().get(row);
	              switch (col) {
	                  case 0:
	                      return item.getName();
	                  case 1:
	                      return item.getValue();
	              }
	              return "";
	          }

	          /**
	           *
	           */
	          public void setValueAt (Object value, int row, int col){
//	              System.out.println("col=" + col + "  value=" + (value == null ? "null" : value.getClass()));
//	              if(col == 0) {
////						enableControls();
////						refreshTable();
//	              }
//	              else {
//	                  super.setValueAt(value, row, col);
//	              }
	          }

	          public String getColumnName (int col) {
	              switch (col) {
	                  case 0:
	                      return  "Name";
	                  case 1:
	                      return  "Value";
	              }
	              return  "";
	          }

	          public boolean isCellEditable (int row, int col) {
	              if( col == 0 ) {
	                  return true;
	              }
	              return false;
	          }

	          public void fireTableStructureChanged () {
	              super.fireTableStructureChanged();
	              TableColumn tc;
	              tc = table.getColumnModel().getColumn(0);
	              tc.setMaxWidth(250);
	              tc.setPreferredWidth(150);
	         }

	      });

//	      // Selection listener
//	      ListSelectionModel lselmod = this.getSelectionModel();
//	      lselmod.addListSelectionListener(new ListSelectionListener() {
//	          public void valueChanged (ListSelectionEvent e) {
//	          }
//	      });

	      ((AbstractTableModel)this.getModel()).fireTableStructureChanged();
	  }
	
	  /**
	   *
	   */
	  public void refreshTable (){
	      int rows[] = getSelectedRows();
	      ((AbstractTableModel)this.getModel()).fireTableDataChanged();
	      for (int i=0; i < rows.length; i++ ) {
	          if( rows[i] < this.getRowCount() ) {
	       	   this.getSelectionModel().addSelectionInterval(rows[i], rows[i]);
	          }
	      }
	  }	

	  /**
	   * make sure currently editing cells get their values commited
	   */
	  public void acceptChanges() {
	      if (this.isEditing()) {
		   	   this.getCellEditor().stopCellEditing();
	      }
	  }

	public void setGLGlobals(GLGlobalList props) {
		this.props = props;
		refreshTable();
	}

	  
}
