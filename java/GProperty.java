
package gridlabd;

/**
 * An extensible class that mirrors the struct PROPERTY in the GridLAB-D core.  Used in GObject,
 * but little is done with this class beyond struct-like behavior.  Primarily for internal use and
 * passing a property address to GridLAB-D core calls.
 * @author Matthew Hauer <matthew.hauer@pnl.gov>
 */
public class GProperty{
	private String name;	/** Name of this property*/
	private String type;	/** Name of the type of this property (see GridlabD.proptype) */
	private long size;		/** Estimated size of this property, in bytes. */
	private long offset;	/** Estimated offset within the OBJECT data block in C for this property, in bytes */
	private long addr;		/** Pointer to the */
	public String GetName(){return name;}
	public String GetType(){return type;}
	public long GetSize(){return size;}
	public long GetOffset(){return offset;}
	public long GetAddr(){return addr;}
	private GProperty(String n, String t, long s, long o){
		name = new String(n);
		type = new String(t);
		size = s;
		offset = o;
	}
	/**
	 * Public constructor wrapper.
	 * @param n Name of the property to create.
	 * @param t Name of the type of property to create.
	 * @param o Offset for this property, in bytes.
	 * @return The new GProperty object, null if the property type was invalid.
	 */
	 public static GProperty Build(String n, String t, long o){
		Long size = GridlabD.proptype.get(t);
		GProperty p = null;
		if(size == null){
			GridlabD.error("Unable to create property \""+n+"\" of type \""+t+"\"");
			return null;
		}
		p = new GProperty(n, t, size.longValue(), o);
		//p.addr = GridlabD.find_property();
		return p;
	}
	/**
	 * Public constructor wrapper.
	 * @param n Name of the property to create.
	 * @param t Name of the type of property to create.
	 * @param o Offset for this property, in bytes.
	 * @param s Size of this property, in bytes.
	 * @return The new GProperty object
	 */
	public static GProperty Build(String n, String t, long o, long s){
		return new GProperty(n, t, s, o);
	}
}