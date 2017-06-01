package gov.pnnl.gridlab.gldebug.model;

import java.util.ArrayList;
import java.util.List;

/**
 * GLGlobalList maintains a list of Globals, name value pairs
 */
public class GLGlobalList {
	private List<Property> props = new ArrayList<Property>();
	
	public GLGlobalList(){
	}
	
	public class Property {
		String name;
		String value;
		public String getName() {
			return name;
		}
		public void setName(String name) {
			this.name = name;
		}
		public String getValue() {
			return value;
		}
		public void setValue(String value) {
			this.value = value;
		}
	}
	
	/**
	 * add a GLOBAL to the list
	 */
	public void addGlobal(String name, String value) {
		
		Property prop = new Property();
		prop.setName(name);
		prop.setValue(value);
		
		props.add(prop);
	}
	
	public List<Property> getProps() {
		return props;
	}


	public Property getGlobal(String string) {
		for (Property  prop : props) {
			if(prop.name.equals(string)){
				return prop;
			}
		}
		return null;
	}

}
