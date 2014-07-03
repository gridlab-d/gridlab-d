
package gridlabd;

/* Object class */
/**
 * A mirror of the OBJECT struct in GridLAB-D.  Primarily used for GridLAB-D native calls.  The
 * class itself is used as the root for Java GridLAB-D objects, whether through inheritence or
 * composition.
 * @author Matthew Hauer <matthew.hauer@pnl.gov>
 */
public class GObject{
	private long object_addr;	/** pointer to the address of the corresponding OBJECT in the core */
	private GClass oclass;		/** GClass that describes this GObject. */
	private long parent;		/** pointer to the parent OBJECT.  not authoratative, use GetParent() the first time. */

	public long GetAddr(){return object_addr;}
	public void SetObject(long object_a){
		object_addr = object_a;
	}
	private GObject(long o){
		object_addr = o;
	}
	/**
	 * Attempts to build a new GObject within the core.
	 * @param oc Class to construct this object out of
	 * @return New GObject upon success.  Null on failure.
	 * @deprecated Use BuildSingle(GClass) instead.
	 */
	public static GObject BuildSingle(GridlabD.Class oc){
		long oaddr = GridlabD.create_object(oc.GetAddr(), oc.GetSize());
		if(oaddr != 0){
			return new GObject(oaddr);
		}
		else {
			GridlabD.error("Unable to create object of type ");
			return null;
		}
	}
	/**
	 * Constructs both an OBJECT* (in the core) and a GObject and adds
	 *	the GObject to the GClass's object dictionary.
	 * @param oc The class to construct this object out of
	 * @return A new GObject if GridlabD.create_object succeeded, null if failed.
	 */
	public static GObject BuildSingle(GClass oc){
		long oaddr = GridlabD.create_object(oc.GetClassAddr(), oc.GetSize());
		if(oaddr != 0){
			GObject o = new GObject(oaddr);
			oc.PutObj(o);
			return o;
		}
		else {
			GridlabD.error("Unable to create object of type ");
			return null;
		}
	}
	/* GLD Object Gets */
	public int GetID(){return _GetID(object_addr);}
	private native int _GetID(long oaddr);

	public long GetObjectClass(){return _GetOClass(object_addr);}
	private native long _GetOClass(long oaddr);

	public long GetNext(){return _GetNext(object_addr);}
	private native long _GetNext(long oaddr);

	public String GetName(){return _GetName(object_addr);}
	private native String _GetName(long oaddr);

	public long GetParent(){
		parent = _GetParent(object_addr);
		return parent;
	}
	private native long _GetParent(long oaddr);

	public int GetRank(){return _GetRank(object_addr);}
	private native int _GetRank(long oaddr);

	public long GetClock(){return _GetClock(object_addr);}
	private native long _GetClock(long oaddr);

	public double GetLatitude(){return _GetLatitude(object_addr);}
	private native double _GetLatitude(long oaddr);

	public double GetLongitude(){return _GetLatitude(object_addr);}
	private native double _GetLongitude(long oaddr);

	public long GetInSvc(){return _GetInSvc(object_addr);}
	private native long _GetInSvc(long oaddr);

	public long GetOutSvc(){return _GetOutSvc(object_addr);}
	private native long _GetOutSvc(long oaddr);

	public long GetFlags(){return _GetFlags(object_addr);}
	private native long _GetFlags(long oaddr);

	/* GLD Object Sets */
	private native void _SetParent(long oaddr, long addr);
	public void SetParent(long p){
		parent = p;
		_SetParent(object_addr, p);
	}
	public void SetFlags(int flags){_SetFlags(object_addr, flags);}
	private native void _SetFlags(long oaddr, int flags);

	/* GLD property funcs */
//	public String GetStringProp(GridlabD.Property prop){return _GetStringProp(object_addr, prop.GetOffset(), prop.GetSize()-1);}
	private native String _GetStringProp(long oaddr, long offset, long count);

//	public void SetStringProp(GridlabD.Property prop, String str){_SetStringProp(object_addr, prop.GetOffset(), prop.GetSize()-1, str);}
	private native void _SetStringProp(long oaddr, long offset, long count, String str);

//	public int GetInt16Prop(GridlabD.Property prop){return _GetInt16Prop(object_addr, prop.GetOffset());}
	private native int _GetInt16Prop(long oaddr, long offset);

//	public void SetInt16Prop(GridlabD.Property prop, int val){_SetInt16Prop(object_addr, prop.GetOffset(), val);}
	public native void _SetInt16Prop(long oaddr, long offset, int val);

//	public int GetInt32Prop(GridlabD.Property prop){return _GetInt32Prop(object_addr, prop.GetOffset());}
	private native int _GetInt32Prop(long oaddr, long offset);

//	public void SetInt32Prop(GridlabD.Property prop, int val){_SetInt32Prop(object_addr, prop.GetOffset(), val);}
	public native void _SetInt32Prop(long oaddr, long offset, int val);

//	public int GetInt64Prop(GridlabD.Property prop){return _GetInt64Prop(object_addr, prop.GetOffset());}
	private native int _GetInt64Prop(long oaddr, long offset);

//	public void SetInt64Prop(GridlabD.Property prop, long val){_SetInt64Prop(object_addr, prop.GetOffset(), val);}
	public native void _SetInt64Prop(long oaddr, long offset, long val);

//	public double GetDoubleProp(GridlabD.Property prop){return _GetDoubleProp(object_addr, prop.GetOffset());}
	private native double _GetDoubleProp(long oaddr, long offset);

//	public void SetDoubleProp(GridlabD.Property prop, double val){_SetDoubleProp(object_addr, prop.GetOffset(), val);}
	public native void _SetDoubleProp(long oaddr, long offset, double val);

//	public GridlabD.Complex GetComplexProp(GridlabD.Property prop){
//		return new GridlabD.Complex(_GetComplexRealProp(object_addr, prop.GetOffset()), _GetComplexImagProp(object_addr, prop.GetOffset()));
//	}
	private native double _GetComplexRealProp(long oaddr, long offset);
	private native double _GetComplexImagProp(long oaddr, long offset);
	private native void _SetComplexProp(long oaddr, long offset, double r, double i);

	private native long _GetObjectProp(long oaddr, long offset);

	public String GetStringProp(GridlabD.Property prop){
		if(prop.GetType().startsWith("char")){
			return _GetStringProp(object_addr, prop.GetOffset(), prop.GetSize()-1);
		} else {
			GridlabD.error("Cannot read prop "+prop.GetName()+" as a string");
			return new String("[error]");
		}
	}
	public boolean SetStringProp(GridlabD.Property prop, String str){
		if(prop.GetType().startsWith("char")){
			_SetStringProp(object_addr, prop.GetOffset(), prop.GetSize()-1, str);
			return true;
		} else {
			GridlabD.error("Cannot write prop "+prop.GetName()+" as a string");
			return false;
		}
	}
	public int GetInt16Prop(GridlabD.Property prop){
		if(prop.GetType().equals("int16")){
			return _GetInt16Prop(object_addr, prop.GetOffset());
		} else {
			GridlabD.error("Cannot read prop "+prop.GetName()+" as an int16");
			return 0;
		}
	}
	public boolean SetInt16Prop(GridlabD.Property prop, int val){
		if(prop.GetType().equals("int16")){
			_SetInt16Prop(object_addr, prop.GetOffset(), val);
			return true;
		} else {
			GridlabD.error("Cannot write prop "+prop.GetName()+" as an int16");
			return false;
		}
	}
	public int GetInt32Prop(GridlabD.Property prop){
		if(prop.GetType().equals("int32")){
			return _GetInt32Prop(object_addr, prop.GetOffset());
		} else {
			GridlabD.error("Cannot read prop "+prop.GetName()+" as an int32");
			return 0;
		}
	}
	public boolean SetInt32Prop(GridlabD.Property prop, int val){
		if(prop.GetType().equals("int32")){
			_SetInt32Prop(object_addr, prop.GetOffset(), val);
			return true;
		} else {
			GridlabD.error("Cannot write prop "+prop.GetName()+" as an int32");
			return false;
		}
	}
	public long GetInt64Prop(GridlabD.Property prop){
		if(prop.GetType().equals("int64")){
			return _GetInt64Prop(object_addr, prop.GetOffset());
		} else {
			GridlabD.error("Cannot read prop "+prop.GetName()+" as an int64");
			return 0;
		}
	}
	public boolean SetInt64Prop(GridlabD.Property prop, long val){
		if(prop.GetType().equals("int64")){
			_SetInt64Prop(object_addr, prop.GetOffset(), val);
			return true;
		} else {
			GridlabD.error("Cannot write prop "+prop.GetName()+" as an int64");
			return false;
		}
	}
	public double GetDoubleProp(GridlabD.Property prop){
		if(prop.GetType().equals("double")){
			return _GetDoubleProp(object_addr, prop.GetOffset());
		} else {
			GridlabD.error("Cannot read prop "+prop.GetName()+" as a double");
			return 0;
		}
	}
	public boolean SetDoubleProp(GridlabD.Property prop, long val){
		if(prop.GetType().equals("double")){
			_SetDoubleProp(object_addr, prop.GetOffset(), val);
			return true;
		} else {
			GridlabD.error("Cannot write prop "+prop.GetName()+" as a double");
			return false;
		}
	}
	public long GetObjectProp(GridlabD.Property prop){
		if(prop.GetType().equals("object")){
			return _GetObjectProp(object_addr, prop.GetOffset());
		} else {
			GridlabD.error("Cannot read prop "+prop.GetName()+" as an object");
			return 0;
		}
	}
	public GridlabD.Complex GetComplexProp(GridlabD.Property prop){
		if(prop.GetType().equals("complex")){
			return new GridlabD.Complex(_GetComplexRealProp(object_addr, prop.GetOffset()), _GetComplexImagProp(object_addr, prop.GetOffset()));
		} else {
			GridlabD.error("Cannot read prop "+prop.GetName()+" as a complex");
			return null;
		}
	}
	public boolean SetComplexProp(GridlabD.Property prop, GridlabD.Complex val){
		if(prop.GetType().equals("complex")){
			_SetComplexProp(object_addr, prop.GetOffset(), val.re, val.im);
			return true;
		} else {
			GridlabD.error("Cannot write prop "+prop.GetName()+" as a complex");
			return false;
		}
	}
}