import gridlabd.*;
import java.util.Hashtable;

public class SampleClass{
	///	similar to CLASS *SampleClass.  Must only be used by SampleClass type objects.
	private static GClass oclass;
	/** the per-class objtable is needed to register and access object built in Java, or they
		will be garbage collected at the end of create() **/
	private static Hashtable<Long, SampleClass> objtable = new Hashtable<Long, SampleClass>();
	/// OBJECT *this ... gives us hooks into core space.
	protected GObject obj;
	///	Constructors should be private, since the build process can fail.
	private SampleClass(GObject o, long paddr){
		obj = o;
		obj.SetParent(paddr);
	}

	/* "abstract" funcs */
	/*	public static void register(GModule mod)
		Grabs the oclass object and publishes the variables
		NOTE: the class has already been registered with the core by the Build call.
	 */
	public static void register(GClass c){
		/* catch oclass */
		oclass = c;
		/* add properties */
		oclass.AddProperty("bingo", "char8");
		oclass.AddProperty("zingo", "int16");
		oclass.AddProperty("zango", "complex");
		oclass.AddProperty("zingo", "double");
	}

	/**	public static long create(long paddr)
		Users are expected to implement thier own GClass.create() in order to construct
		objects in the core that can be accessed by Java.
		1.) long oaddr = GObject.BuildSingle(oclass);

		1.)	"long oaddr = GridlabD.create_object(classaddr, size)"
		2.)	"GClass obj = new GClass(oaddr, paddr)"
		3.) "objtable.put(oaddr, obj)"
		4.)	Perform any create-time initialization that is independent of file input and external
				values, namely non-zero defaults.  The object block is memcpy(0)'d by the time
				GridlabD.create_object() returns the address value.
		5.) "return oaddr"
		Replace "GClass" from above with your class
		Any deviation from the form "public static long create(long)" will cause the underlying
			C/JNI code to not find the method, error, and fail the load process.
		@return value of oaddr if construction was successful, 0 if failed
	 **/
	public static long create(long paddr){
		GObject o = GObject.BuildSingle(oclass);
		if(o == null){
			GridlabD.error("GObject.BuildSingle() failed");
			return 0;
		}
		SampleClass sc = new SampleClass(o, paddr);
		objtable.put(new Long(o.GetAddr()), sc);
		/* any overridable initial values here */

		/* return o.GetAddr() */
		return o.GetAddr();
	}

	/*	public static int init(long oaddr, long paddr)
	 *	The init function is an option step for classes to calculate derived values based on initial
	 *	conditions, and reality check values entered into published variables.
	 *	@return Nonzero if successful, zero if the object was initialized or constructed badly
	 */
	public static int init(long oaddr, long paddr){
		SampleClass sc = objtable.get(new Long(oaddr));
		if(sc == null){
			GridlabD.error("SampleClass.init(): not one of our objects at 0x"+Long.toHexString(oaddr));
		}
		/* your code here */
		System.out.println("SampleClass.init()");
		/* success */
		return 1;
	}

	/*	public static int commit(long oaddr)
	 *	The commit function is an optional step for classes to update or output any strictly
	 *	internal states between timesteps.  Commit() is not called between iterations, but
	 *	between clock changes.
	 *	Objects that call commit() should not refer to external objects during the commit step,
	 *	as the rank order is not maintained.
	 *	@return Nonzero if successful, zero if the object is in an inconsistant state.
	 */
	public static int commit(long oaddr){
		SampleClass sc = objtable.get(new Long(oaddr));
		if(sc == null){
			GridlabD.error("SampleClass.commit(): not one of our objects at 0x"+Long.toHexString(oaddr));
		}
		/* your code here */
		System.out.println("SampleClass.commit()");
		/* success */
		return 1;
	}
	/*	public static long sync(long oaddr, long t0, int pass)
	 *	Users are expected to implement their own GClass.sync() in order to receive
	 *	periodic attention from the core as to when events are happening to at least one
	 *	object in the core, ranging from as minor as a recorder reading to having the
	 *	feeder voltage double for three seconds.
	 *	No particular actions are required from user-defined sync() calls in Java, but the core
	 *	will only notice published variables.  There are no restrictions on using classes and
	 *	variables in Java space, just be sure to write the results when you're done with them.
	 *	@return Timestamp for next event, GridlabD.TS_NEVER if we're steady-state or finished, -1 on error, and the current clock value for ???.
	 */
	public static long sync(long oaddr, long t0, int pass){
		SampleClass sc = objtable.get(new Long(oaddr));
		if(sc == null){
			GridlabD.error("SampleClass.sync(): not one of our objects at 0x"+Long.toHexString(oaddr));
		}
		/* your code here */
		System.out.println("SampleClass.sync("+t0+")");
		/* return next event timestamp */
		return GridlabD.TS_NEVER;
	}

	/*	public static int notify(long oaddr, int msg)
	 *	"can be implemented to receive notification messages before and after a variable is changed by
	 *	the core (GridlabD.NM_PREUPDATE / GridlabD.NM_POSTUPDATE) and when the core needs the module to
	 *	reset (GridlabD.NM_RESET)"
	 *	@return Nonzero on success, but often ignored.
	 */
	public static int notify(long oaddr, int msg){
		SampleClass sc = objtable.get(new Long(oaddr));
		if(sc == null){
			GridlabD.error("SampleClass.notify(): not one of our objects at 0x"+Long.toHexString(oaddr));
		}
		/* your code here */
		return 0;
	}

	/*	public static int isa(long oaddr)
	 *	Used to determine if the object pointed to be oaddr is of the type or subtype for this
	 *	class.
	 *	@return Nonzero if true, zero if not a valid subtype.
	 */
	public static int isa(long oaddr){
		if(objtable.containsKey(new Long(oaddr)))
			return 1;
		else
			return 0;
	}

	/**	public static long plc(long oaddr, long t1)
		"can be implemented to create default PLC code that can be overridden by attaching a
		child plc object"
		PLC code is called during the GridlabD.PC_BOTTOMUP pass.
		PLC code is NOT called if OF_HASPLC is set, which indicates that a child object has PLC code
			that overrides ours.
		@return Timestamp for next event, GridlabD.TS_NEVER if we're steady-state or finished, -1 on error,
	 		and the current clock value for ???.
	 **/
	public static long plc(long oaddr, long t1){
		return GridlabD.TS_NEVER;
	}
}