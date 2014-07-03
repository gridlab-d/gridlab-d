package gov.pnnl.gridlab.gldebug.ui;

import gov.pnnl.gridlab.gldebug.model.GLObjectProperties;
import gov.pnnl.gridlab.gldebug.model.GLObjectProperties.Property;

import javax.swing.JTable;
import javax.swing.table.*;

public class DetailsTable extends JTable {

    private GLObjectProperties props;
	
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
	              return 3;
	          }

	          public Object getValueAt (int row, int col) {
	        	  Property item = (Property)props.getProps().get(row);
	              switch (col) {
	                  case 0:
	                      return item.getName();
	                  case 1:
	                      return item.getValue();
	                  case 2:
	                	  String val = item.getType();
	                      return (val != null ? val : "");
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
	                  case 2:
	                      return  "Type";
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

	              tc = table.getColumnModel().getColumn(2);
	              tc.setMaxWidth(150);
	              tc.setPreferredWidth(100);
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

	public void setProps(GLObjectProperties props) {
		this.props = props;
		refreshTable();
	}

	  
}
