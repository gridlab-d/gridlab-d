
package gridlabd;

import java.util.Enumeration;
import java.util.Hashtable;

/**
 * Provides an interface for the GridLAB-D core functionality and reflects what should be on the other side
 * of the Java-C divide and contains a dictionary of the recognized property types within the core and a list of
 * constants that are otherwise \#DEFINE-ed.  Internal classes are used to maintain the modules, classes,
 * properties, and objects published by Java modules.
 * <P>
 * A number of the long values that appear in the internal classes are pointers to memory allocated by the core.
 * Tinkering with these values is a Bad Idea.
 * @author Matthew Hauer <matthew.hauer@pnl.gov>
 * @version 1.0
 */
public class GridlabD {
	/* simple GridlabD statics */
	public static Hashtable<String, Long> proptype;
	public static final long TS_NEVER = Long.MAX_VALUE;
	public static final long TS_INVALID = -1;
	public static final long TS_ZERO = 0;
	public static final long TS_MAX = Long.MAX_VALUE;
	public static final long MINYEAR = 1970;
	public static final long MAXYEAR = 2969;
	public static final int PC_NOSYNC = 0;
	public static final int PC_PRETOPDOWN = 1;
	public static final int PC_BOTTOMUP = 2;
	public static final int PC_POSTTOPDOWN = 4;
	public static final double PI = 3.1415926535897932384626433832795;
	public static final double E = 2.71828182845905;
	public static final int NM_PREUPDATE = 0;
	public static final int NM_POSTUPDATE = 1;
	public static final int NM_RESET = 2;
	public static final int OF_NONE = 0;
	public static final int OF_HASPLC = 1;
	public static final int OF_LOCKED = 2;
	public static final int OF_RECALC = 8;
	public static final int OF_FOREIGN = 16;
	public static final int OF_SKIPSAFE = 32;
	public static final int OF_RERANK = 16384;
	/* Inner classes */
	/**
	 * Simple Complex number class.
	 */
	public static class Complex{
		public double re; /** Real component. */
		public double im; /** Imaginary component. */
		/** Default constructor with value 0+0j. */
		public Complex(){
			re = 0.0;
			im = 0.0;
		}
		/**
		 * Constructor.
		 * @param r Real component.
		 * @param i Imaginary component.
		 */
		public Complex(double r, double i){
			re = r;
			im = i;
		}
		public String toString(){
			if(im >= 0.0)
				return Double.toString(re)+"+"+Double.toString(im)+"i";
			return Double.toString(re)+Double.toString(im)+"i";
		}
	}
	/**
	 * The inner Property class is used to cache published properties from classes in Java modules.
	 */
	// public static class Property{
		// private String name; /** property name */
		// private String type; /** property type - must be equal to "int16", "int32", "int64", "double", "char8", "char32", "char256", "char1024", "complex", or "object". */
		// private long size; /** in bytes, needed for correctly allocating objects in the core */
		// private long offset; /** in bytes, needed for correctly locating within core memory */
		// private long addr; /** address pointer to a PROPERTY * where the core allocated this property */
		// public String GetName(){return name;}
		// public String GetType(){return type;}
		// public long GetSize(){return size;}
		// public long GetOffset(){return offset;}
		// public long GetAddr(){return addr;}
		// private Property(String n, String t, long s, long o){
			// name = new String(n);
			// type = new String(t);
			// size = s;
			// offset = o;
		// }
		// /** Builds a Property object and registers it with the core.
		  // * @param n Property name
		  // * @param t Property type
		  // * @param o Address of the object registering this property in the core
		  // * @return A new Property object.   Null return on failure.
		  // */
		// public static Property Build(String n, String t, long o){
			// Long size = proptype.get(t);
			// Property p = null;
			// if(size == null){
				// GridlabD.error("Unable to create property \""+n+"\" of type \""+t+"\"");
				// return null;
			// }
			// p = new GridlabD.Property(n, t, size.longValue(), o);
			// return p;
		// }
		// /** Builds a Property object within the Java core.  Not recommended since it does not work with the core.  Internal use only.  */
		// public static Property Build(String n, String t, long o, long s){
			// return new GridlabD.Property(n, t, s, o);
		// }
	// }
	/**
	 * Contains the properties of GridLAB-D classes registered by Java modules and keys them in a HashTable by name.
	 * @deprecated Use GClass instead of GridlabD.Class
	 */
	// public static class Class{
		// private String classname; /** The published name of this class */
		// private long addr; /** The address of the CLASS in the core. */
		// private long modaddr; /** The address of the MODULE in the core that this class belongs to */
		// private int synctype; /** Bitfield for what sync passes this class operates during */
		// private boolean inUse; /** True once an object of this class is created during load time. */
		// private long size; /** Size of this class when instatiated, in bytes.  */
		// private Hashtable<String, Property> ptable; /***/
		// public long GetSize(){inUse = true; return size;} /* once we say how big we are, keep that value valid. */
		// public long GetAddr(){return addr;}
		// public String GetName(){return classname;}
		// public void SetUsed(){inUse = true;}
		// private Class(String n, long ca, long ma, int s){
			// classname = new String(n);
			// ptable = new Hashtable<String, Property>();
			// addr = ca;
			// modaddr = ma;
			// synctype = s;
			// inUse = false;
			// size = 0;
		// }
		// /**
		 // * Registers a new class with the GridLAB-D core.
		 // * @param n Name of the new class
		 // * @param a Address of the module this class is registering with.
		 // * @param sync Bitfield of the sync phases for this class.
		 // * @return Null on error, Class object on success.
		 // */
		// public static Class Build(String n, long a, int sync){
			// long addr = GridlabD.register_class(a, n, sync); /**/
			// if(addr != 0){
				// return new Class(n, addr, a, sync);
			// }
			// return null;
		// }
		// /**
		 // * Publishes a property in the GridLAB-D core for this class.
		 // * @param pname Name of the property to publish
		 // * @param ptype Name of the property type to publish
		 // * @return New Property on success, null pointer on error.
		 // */
		// public Property AddProperty(String pname, String ptype){
			// if(inUse){
				// GridlabD.error("Class "+classname+" is in use, ignoring AddProperty attempt");
				// return null;
			// }
			// GridlabD.verbose("Class.AddProperty("+pname+", "+ptype+")");
			// long offset = 0;
			// for(Enumeration<Property> e = ptable.elements(); e.hasMoreElements();){
				// Long ov = e.nextElement().GetSize();
				// offset += ov.longValue();
			// }
			// GridlabD.verbose("\toffset = "+offset);
			// if(ptable.containsKey(pname)){
				// GridlabD.error("Property \""+pname+"\" already exists");
				// return null;
			// } else {
				// GridlabD.verbose("Property "+pname+" is unique");
			// }
			// long psize = GridlabD.publish_variable(modaddr, addr, pname, ptype, offset);
			// if(0 == psize){
				// GridlabD.error("Unable to GridlabD.publish_variable("+modaddr+", "+addr+", "+pname+", "+ptype+", "+offset+")");
			// } else {
				// GridlabD.verbose("Published "+pname);
			// }
			// Property p = GridlabD.Property.Build(pname, ptype, offset, psize);
			// if(p == null){
				// GridlabD.error("Unable to GridlabD.Property.Build("+pname+", "+ptype+", "+offset+", "+psize+")");
				// return null;
			// }
			// ptable.put(pname, p);
			// size = offset + psize; /* update class size */
			// return p;
		// }
		// public boolean hasProperty(String pname){
			// if(ptable.containsKey(pname))
				// return true;
			// return false;
		// }
		// /**
		 // * Fetches a Property object by name from the Class's property table.
		 // * @param pname Name of the property to be found.
		 // * @return A Property with the same property name, else null.
		 // */
		// public GridlabD.Property findProperty(String pname){
			// return ptable.get(pname);
		// }
	// }
	/* static methods of Property */

	/* Module inner class */
	/* NOTE: modules are already registered with the core by the time we get to Java.
	 * 	All we're doing on this side is storing what we'll want for later, the
	 *	class information, and the property information. */
	/**
	 * The Module class tracks what classes have been added, and the address for the module in the core.  By the time Java
	 * code is executed, the module has already been instantiated in the core, but will not have any classes or states registered.
	 * @deprecated Use GModule instead of GridlabD.Module
	 */
	// public static class Module{
		// private String modulename; /** Name of this module, as we will look for it in the core. */
		// private long addr; /** Address of the MODULE allocated in the core. */
		// private Hashtable<String, Class> ctable; /** Dictionary of the classes registered by this module. */
		// /**
		 // * Returns the core memory address of the first class that we registered.
		 // * @return A pointer to the first CLASS registered for this module in the core.
		 // */
		// public long GetFirstClassAddr(){
			Enumeration<Class> ec = ctable;
			// if(ctable.isEmpty())
				// return 0;
			// return ctable.elements().nextElement().GetAddr();
		// }
		// public Module(String name, long a){
			// modulename = new String(name);
			// ctable = new Hashtable<String, Class>();
			// addr = a;
		// }
		// /**
		 // * Tells the module to publish a class in the core.
		 // * @param cname The name of the class to be published.
		 // * @param sync The pass flags for the new class
		 // * @return The new GridlabD.Class object on success, null on failure.
		 // */
		// public GridlabD.Class AddClass(String cname, int sync){
			// if(!ctable.containsKey(cname)){
				// GridlabD.Class c = GridlabD.Class.Build(cname, addr, sync);
				// ctable.put(new String(cname), c);
				// GridlabD.verbose("Module "+modulename+" registered "+c.GetName()+" and got a pointer to 0x"+Long.toHexString(c.GetAddr()));
				// return c;
			// }
			// GridlabD.error("We were unable to register class \""+cname+"\"");
			// return null;
		// }
	// }
	// /* complex GridlabD statics ~ singleton behavior */
	// private static Hashtable<String, GridlabD.Module> moduletable;
	private static Hashtable<String, GModule> gmoduletable;
	/* Initialization block */
	static
	{
		moduletable = new Hashtable<String, GridlabD.Module>();
		gmoduletable = new Hashtable<String, GModule>();
		proptype = new Hashtable<String, Long>();
		proptype.put("int16", new Long(2));
		proptype.put("int32", new Long(4));
		proptype.put("int64", new Long(8));
		proptype.put("double", new Long(8));
		proptype.put("char8", new Long(9));
		proptype.put("char32", new Long(33));
		proptype.put("char256", new Long(257));
		proptype.put("char1024",new Long(1025));
		proptype.put("complex", new Long(24)); /* under win32 msvc2k5 */
		proptype.put("object", new Long(8));
		proptype.put("bool", new Long(4));
		proptype.put("timestamp", new Long(8)); // int64
		proptype.put("loadshape", new Long(256)); /* slight overestimation, but close enough -mhauer */
		proptype.put("enduse", new Long(256));
		//System.out.println("Attempting to load glmjava.dll");
		try{
			System.loadLibrary("glmjava");
		}
		catch(Throwable e){
			System.out.println(e);
		}
		//System.out.println("Concluded attempt");
	}
	/* Static methods */
	public static double clip(double x, double a, double b){
		if(x < a) return a;
		if(x > b) return b;
		return x;
	}
	/* Utility native stubs */
	public static native int GetInt16(long block, long offset);
	public static native int GetInt32(long block, long offset);
	public static native long GetInt64(long block, long offset);
	public static native double GetDouble(long block, long offset);
	public static native double[] GetComplex(long block, long offset);
	public static native int SetInt16(long block, long offset, int value);
	public static native int SetInt32(long block, long offset, int value);
	public static native int SetInt64(long block, long offset, long value);
	public static native int SetDouble(long block, long offset, double value);
	public static native int SetComplex(long block, long offset, double r, double i);

	/* Native stubs */
	public static native int verbose(String str);
	public static native int output(String str);
	public static native int warning(String str);
	public static native int error(String str);
	public static native int debug(String str);
	public static native int testmsg(String str);
	public static native long malloc(int size);
	/* double * module_getvar_addr(MODULE *module, char *varname) */
	public static native long get_module_var(String modulename,  String varname);
	public static native String findfile(String filename);
	public static native long find_module(String modulename);
	public static native long register_class(long moduleaddr, String classname, int passconfig);
	public static native long create_object(long oclass_addr, long size);
	public static native long create_array(long oclass_addr, long size, int n_objects);
	public static native int object_isa(long obj_addr, String typename);
	public static native long publish_variable(long moduleaddr, long classaddr, String varname, String vartype, long offset); /* define_map */
	public static native long publish_function(String modulename, String classname, String funcname); /* define_function */
	public static native int set_dependent(long from_addr, long to_addr);
	public static native int set_parent(long child_addr, long parent_addr);
	public static native long register_type(long oclass_addr, String typename, String from_string_fname, String to_string_fname);
	public static native int publish_delegate(); /* define_type*/
	public static native long get_property(long objaddr, String propertyname);
	public static native long get_property_by_name(String objectname, String propertyname);
	public static native String get_value(long object_addr, String propertyname);
	public static native int set_value(long object_addr, String propertyname, String value);
	public static native double unit_convert(String from, String to, double invalue);
	//public static native *** unit_covert_ex(***); /* no good conversion! */
	public static GridlabD.Complex get_complex(long object_addr, long property_addr){
		double r = get_complex_real(object_addr, property_addr);
		double i = get_complex_imag(object_addr, property_addr);
		return new GridlabD.Complex(r, i);
	}
	public static GridlabD.Complex get_complex_by_name(long object_addr, String propertyname){
		double r = get_complex_real_by_name(object_addr, propertyname);
		double i = get_complex_imag_by_name(object_addr, propertyname);
		return new GridlabD.Complex(r, i);
	}
	private static native double get_complex_real(long object_addr, long property_addr);
	private static native double get_complex_imag(long object_addr, long property_addr);
	private static native double get_complex_real_by_name(long object_addr, String propertyname);
	private static native double get_complex_imag_by_name(long object_addr, String propertyname);
	public static native long get_object(String oname);
	public static native long get_object_prop(long object_addr, long property_addr);
	public static native int get_int16(long object_addr, long property_addr);
	public static native int get_int16_by_name(long object_addr, String propertyname);
	public static native int get_int32(long object_addr, long property_addr);
	public static native int get_int32_by_name(long object_addr, String propertyname);
	public static native long get_int64(long object_addr, long property_addr);
	public static native long get_int64_by_name(long object_addr, String propertyname);
	public static native double get_double(long object_addr, long property_addr);
	public static native double get_double_by_name(long object_addr, String propertyname);
	public static native String get_string(long object_addr, long property_addr);
	public static native String get_string_by_name(long object_addr, String propertyname);
	public static native long[] find_objects(long findlist_addr, String[] args); /* returns findlist *[] */
	public static native long find_next(long findlist_addr); /* returns an object_addr */
	//public static native *** free(***); /* links straight to stdlib */
	public static native long create_aggregate(String aggregator, String group_expression); /* returns struct s_aggregate * */
	public static native double run_aggregate(long aggregate_addr);
	public static native double random_uniform(double a, double b);
	public static native double random_normal(double m, double s);
	public static native double random_lognormal(double m, double s);
	public static native double random_bernoulli(double p);
	public static native double random_pareto(double m, double a);
	public static native double random_sampled(int n, double[] x);
	public static native double random_exponential(double l);
	public static native long parsetime(String value); /* convert_to_timestamp */
	//public static native long mktime(Object timestamp); /* mkdatetime */
	public static native int strtime(Object timestamp, String buffer, int size); /* strdatetime */
	public static native double todays(long t); /* timestamp_to_days */
	public static native double tohours(long t);
	public static native double tominutes(long t);
	public static native int localtime(long t, Object datetime); /* local_datetime */
	public static native long global_create(String name, String args); /* returns GLOBALVAR * */
	public static native int global_setvar(String def, String args);
	public static native String global_getvar(String name); /* WARNING ~ parametric */
	public static native long global_find(String name); /* returns GLOBALVAR * */
	public static native double clip(double x, double a, double b);
	public static native int bitof(long x);
	public static native String name(long myaddr);
	public static native long find_schedule(String name);
	public static native long schedule_create(String name, String def);
	public static native long schedule_index(long sch, long ts);
	public static native double schedule_value(long sch, long index);
	public static native long schedule_dtnext(long sch, long index);
	public static native long enduse_create();
	public static native int enduse_sync(long e, long t1);
	public static native long loadshape_create(long sched_addr);
	public static native double get_loadshape_value(long ls_addr)
	// gl_strftime(DATETIME *, char *, int) // not sure about passing DATETIME over
	public static native String strftime(long ts);
	public static native double lerp(double t, double x0, double y0, double x1, double y1);
	public static native double 1erp(double t, double x0, double y0, double x1, double y1, double x2, double y2);
}