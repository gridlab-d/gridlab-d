package gov.pnnl.gridlab.gldebug.model;

import java.util.ArrayList;
import java.util.List;

/**
 * GLObjectProperties maintains information about the properties of the object.
 */
public class GLObjectProperties {
	private String objectName; 
	private String name;
	private List<Property> props = new ArrayList<Property>();
	
	public GLObjectProperties(){
	}
	
	public class Property {
		String name;
		String value;
		String type;
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
		public String getType() {
			return type;
		}
		public void setType(String type) {
			this.type = type;
		}
	}
	
	/**
	 * add a new property.  Type can be null.
	 */
	public void addProperty(String name, String value, String type) {
		
		Property prop = new Property();
		prop.setName(name);
		prop.setType(type);
		prop.setValue(value);
		
		props.add(prop);
		
		if(name.equals("name")){
			this.name = value;
		}
		
	}
	
	public void addSimpleProp(String string, String string2) {
		// TODO Auto-generated method stub
		
	}

	public List<Property> getProps() {
		return props;
	}


	public Property getProperty(String string) {
		for (Property  prop : props) {
			if(prop.name.equals(string)){
				return prop;
			}
		}
		return null;
	}

	/**
	 * get the object name 
	 * @return
	 */
	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}
	
	/**
	 * gets the instance name of the object
	 * @return
	 */
	public String getObjectName() {
		return objectName;
	}

	public void setObjectName(String objectName) {
		this.objectName = objectName;
	}


}
