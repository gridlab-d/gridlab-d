
package gridlabd;

import java.util.Hashtable;
import java.util.Enumeration;

/**
 * The primary representation of the GridLAB-D CLASS struct in Java.  Includes native hooks for
 * calls in the GridLAB-D core that pertain to the CLASS structures.
 * @author Matthew Hauer <matthew.hauer@pnl.gov>
 */
public class GClass{
	private String classname;
	private int synctype;
	private long classaddr;
	private long modaddr;
	private boolean inUse;
	private long size;
	private Hashtable<String, GProperty> proptable;
	private Hashtable<Long, GObject> objtable;


	private GClass(String cname, long caddr, long maddr, int sync){
		classname = new String(cname);
		classaddr = caddr;
		modaddr = maddr;
		synctype = sync;
		proptable = new Hashtable<String, GProperty>();
		objtable = new Hashtable<Long, GObject>();
	}
	/**
	 * public static GClass BuildClass(String classname, GModule module, int sync)
	 * Registers a classname with a core module and adds the new class object
	 * to the GModule.
	 * @param classname The name of the class to publish
	 * @param mod The GModule object that is registering the class
	 * @param sync The pass configuration for this class
	 * @return New GClass on success, null on failure.
	 */
	public static GClass BuildClass(String classname, GModule mod, int sync){
		long classaddr = GridlabD.register_class(mod.GetAddr(), classname, sync);
		if(classaddr == 0){
			GridlabD.error("register_class("+mod.GetAddr()+", "+classname+", "+sync+") failed");
			return null;
		}
		GClass gc = new GClass(classname, classaddr, mod.GetAddr(), sync);
		mod.AddClass(gc);
		return gc;
	}
	/**
	 * Adds an object to this GClass's HashTable of instantiated objects.
	 * @param obj The GObject to add to the object table.
	 */
	public void PutObj(GObject obj){
		if(!objtable.containsKey(obj.GetAddr())){
			objtable.put(new Long(obj.GetAddr()), obj);
		}
	}
	/* public funcs */
	public String GetClassname(){return new String(classname);}
	public long GetClassAddr(){return classaddr;}
	public long GetModuleAddr(){return modaddr;}
	public int GetSyncType(){return synctype;}
	public boolean isInUse(){return inUse;}
	public long GetSize(){return size;}
	public boolean hasProperty(String pname){
		return proptable.containsKey(pname);
	}
	/**
	 * Retrieves a property by name from the GClass's property table.
	 * @param pname The property name.
	 * @return The GProperty if it is found, null otherwise.
	 */
	public GProperty findProperty(String pname){
		return proptable.get(pname);
	}
	/**
	 * Checks to see if a particular OBJECT * has a corresponding GObject in this
	 * GClass's object table.
	 * @param addr The address of the OBJECT structure in the core.
	 * @return True if the pointer references an object of this class.
	 */
	public boolean hasObject(Long addr){
		return objtable.containsKey(addr);
	}
	public Object getObject(Long addr){
		return objtable.get(addr);
	}
	/**
	 * Adds a property to a GClass, appends it to the property table, and increases the estimated
	 * size of the class objects.  Will not add a property if the class is in use.
	 * @param pname Name of the property to add.
	 * @param ptype Name of the propety type to add (see GridlabD.proptype)
	 * @return Null if the class is in use, if the property already exists for this class, or if
	 * an error occurs.  Returns the new GProperty on success.
	 */
	public GProperty AddProperty(String pname, String ptype){
		if(inUse){
			GridlabD.error("Class "+classname+" is in use, ignoring AddProperty attempt");
			return null;
		}
		GridlabD.verbose(classname+".AddProperty("+pname+", "+ptype+")");
		long offset = 0;
		for(Enumeration<GProperty> e = proptable.elements(); e.hasMoreElements();){
			Long ov = e.nextElement().GetSize();
			offset += ov.longValue();
		}
		GridlabD.verbose("\toffset = "+offset);
		if(proptable.containsKey(pname)){
			GridlabD.error("Property \""+pname+"\" already exists");
			return null;
		}
		long psize = GridlabD.publish_variable(modaddr, classaddr, pname, ptype, offset);
		if(0 == psize){
			GridlabD.error("Unable to GridlabD.publish_variable("+modaddr+", "+classaddr+", "+pname+", "+ptype+", "+offset+")");
		}
		GProperty p = GProperty.Build(pname, ptype, offset, psize);
		if(p == null){
			GridlabD.error("Unable to GProperty.Build("+pname+", "+ptype+", "+offset+", "+psize+")");
			return null;
		}
		proptable.put(pname, p);
		size = offset + psize; /* update class size */
		return p;
	}

	/* native funcs */
	/* object props */
	private native int _GetID(long oaddr);
	private native long _GetOClass(long oaddr);
	private native long _GetNext(long oaddr);
	private native String _GetName(long oaddr);
	private native long _GetParent(long oaddr);
	private native void _SetParent(long oaddr, long addr);
	private native int _GetRank(long oaddr);
	private native long _GetClock(long oaddr);
	private native double _GetLatitude(long oaddr);
	private native double _GetLongitude(long oaddr);
	private native long _GetInSvc(long oaddr);
	private native long _GetOutSvc(long oaddr);
	private native long _GetFlags(long oaddr);
	/* per-type */
	/*static public native String _GetStringProp(long oaddr, long offset, long count);
	static public native void _SetStringProp(long oaddr, long offset, long count, String str);
	static public native int _GetInt16Prop(long oaddr, long offset);
	static public native void _SetInt16Prop(long oaddr, long offset, int val);
	static public native int _GetInt32Prop(long oaddr, long offset);
	static public native void _SetInt32Prop(long oaddr, long offset, int val);
	static public native long _GetInt64Prop(long oaddr, long offset);
	static public native long _SetInt64Prop(long oaddr, long offset, long val);
	static public native double _GetDoubleProp(long oaddr, long offset);
	static public native void _SetDoubleProp(long oaddr, long offset, double val);
	static public native double _GetComplexRealProp(long oaddr, long offset);
	static public native double _GetComplexImagProp(long oaddr, long offset);
	static public native void _SetComplexProp(long oaddr, long offset, double r, double i);*/

	/* GLD Object Gets */
	/*public int GetID(){return _GetID(object_addr);}
	public long GetObjectClass(){return _GetOClass(object_addr);}
	public long GetNext(){return _GetNext(object_addr);}
	public String GetName(){return _GetName(object_addr);}
	public long GetParent(){return _GetParent(object_addr);}
	public void SetParent(long addr){_SetParent(object_addr, addr);}
	public int GetRank(){return _GetRank(object_addr);}
	public long GetClock(){return _GetClock(object_addr);}
	public double GetLatitude(){return _GetLatitude(object_addr);}
	public double GetLongitude(){return _GetLatitude(object_addr);}
	public long GetInSvc(){return _GetInSvc(object_addr);}
	public long GetOutSvc(){return _GetOutSvc(object_addr);}
	public long GetFlags(){return _GetFlags(object_addr);}*/
}