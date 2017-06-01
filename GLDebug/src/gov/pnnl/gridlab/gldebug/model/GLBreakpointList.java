package gov.pnnl.gridlab.gldebug.model;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.jdom.Element;

/**
 * GLBreakpointList maintains a list of breakpoints
 */
public class GLBreakpointList {
	public static final String XML_ELEMENT = "breakpoints";
	private List<Property> props = new ArrayList<Property>();
	
	public GLBreakpointList(){
	}
	
	
	public class Property {
		GLBreakpointType type;
		String value;
		boolean enabled = true;
		public GLBreakpointType getType() {
			return type;
		}
		public void setType(GLBreakpointType name) {
			this.type = name;
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
	 * add a breakpoint to the list, enabled by default
	 */
	public void addBreak(GLBreakpointType type, String value) {
		addBreak(type, value, true);
	}
	/**
	 * add a breakpoint, specify if enabled.
	 * @param type
	 * @param value
	 * @param enabled
	 */
	public void addBreak(GLBreakpointType type, String value, boolean enabled) {
		
		Property prop = new Property();
		prop.setType(type);
		prop.setValue(value);
		prop.setEnabled(enabled);
		
		props.add(prop);
	}
	
	public Property getBreak(int index) {
		return props.get(index);
	}
	
	public int size() {
		return props.size();
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
		  Element child = new Element("breakpoint");
		  child.setAttribute("type", prop.type.toString());
		  if(prop.value != null){
			  child.setAttribute("value", prop.value);
		  }
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
   public static GLBreakpointList createFromXML(Element elem) throws Exception{
	   if( !elem.getName().equals(XML_ELEMENT)){
		   throw new Exception("Invalid XML breakpoint list.");
	   }
	   
	   GLBreakpointList list = new GLBreakpointList();
	   List<Element> kids = elem.getChildren();
	   for (Element child : kids) {
		   if( !child.getName().equals("breakpoint")){
			   throw new Exception("Invalid XML breakpoint list.");
		   }
		   String type = child.getAttributeValue("type");
		   String value = child.getAttributeValue("value");
		   boolean enabled = child.getAttribute("enabled").getBooleanValue();
		   GLBreakpointType glType = GLBreakpointType.getTypeFromName(type);
		   
		   list.addBreak(glType, value, enabled);
	   }
	   
	   return list;
   }

}
