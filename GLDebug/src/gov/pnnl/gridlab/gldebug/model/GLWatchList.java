package gov.pnnl.gridlab.gldebug.model;

import gov.pnnl.gridlab.gldebug.model.GLBreakpointList.Property;

import java.util.ArrayList;
import java.util.List;

import org.jdom.Element;

/**
 * GLWatchList maintains a list of watches
 */
public class GLWatchList {
	public static final String XML_ELEMENT = "watches";

	private List<Property> props = new ArrayList<Property>();
	
	public GLWatchList(){
	}
	
	public class Property {
		String object;
		String property;
		transient String value;
		boolean enabled = true;
		public String getObject() {
			return object;
		}
		public void setObject(String name) {
			this.object = name;
		}
		public void setProperty(String name) {
			this.property = name;
		}
		public String getProperty() {
			return property;
		}
		public String getValue() {
			return value;
		}
		public void setValue(String value) {
			this.value = value;
		}
		public void setEnabled(boolean b){
			enabled = b;
		}
		public boolean isEnabled(){
			return enabled;
		}
		
	}
	
	/**
	 * add a GLOBAL to the list
	 */
	public void addWatch(String object, String property, boolean enabled) {
		
		Property prop = new Property();
		prop.setObject(object);
		prop.setProperty(property);
		prop.setValue("");
		prop.setEnabled(enabled);
		
		props.add(prop);
	}
	
	public List<Property> getProps() {
		return props;
	}

	
	/**
	 * get the breakpoints currently defined and return as an XML element ready to serialize.
	 * @return
	 */
   public Element getXMLElement() {
	   Element elem = new Element(XML_ELEMENT);
	   
	   for (Property prop : props) {
		  Element child = new Element("watch");
		  child.setAttribute("object", prop.object);
		  child.setAttribute("property", prop.property);
		  child.setAttribute("enabled", String.valueOf(prop.enabled));
		  elem.addContent(child);
	   }
       return elem;
   }
   
   /**
    * create this list from xml
    * @param elem
    * @return
    */
   public static GLWatchList createFromXML(Element elem) throws Exception{
	   if( !elem.getName().equals(XML_ELEMENT)){
		   throw new Exception("Invalid XML watch list.");
	   }
	   
	   GLWatchList list = new GLWatchList();
	   List<Element> kids = elem.getChildren();
	   for (Element child : kids) {
		   if( !child.getName().equals("watch")){
			   throw new Exception("Invalid XML watch list.");
		   }
		   String obj = child.getAttributeValue("object");
		   String prop = child.getAttributeValue("property");
		   boolean enabled = child.getAttribute("enabled").getBooleanValue();
		   
		   list.addWatch(obj, prop, enabled);
	   }
	   
	   return list;
   }

}
