
package gridlabd;

import java.util.Hashtable;

/**
 * The base representation of a GridLAB-D MODULE struct.  Should be incorporated into Java
 * GridLAB-D modules, whether with composition or (preferably) inheritence.
 * @author Matthew Hauer <matthew.hauer@pnl.gov>
 */
public class GModule{
	protected String name;
	protected long addr;
	protected Hashtable<String, GClass> ctable = new Hashtable<String, GClass>();
	public long GetFirstClassAddr(){
		if(ctable.isEmpty())
			return 0;
		return ctable.elements().nextElement().GetClassAddr();
	}
	public GModule(long a, String n){
		addr = a;
		name = new String(n);
	}
	public int AddClass(GClass gc){
		if(!ctable.containsKey(gc.GetClassname())){
			ctable.put(new String(gc.GetClassname()), gc);
			return 1;
		}
		GridlabD.error("Module "+name+" already contains class "+gc.GetClassname());
		return 0;
	}
	public long GetAddr(){return addr;}
	public String GetName(){return new String(name);}
	/* the module has been allocated in C already, we just need to fill it in on this side */
	public final static GModule init(long mod_addr, String mod_name, int argc, String[] argv){
		GModule mod = new GModule(mod_addr, mod_name);
		return mod;
	}
	public final static int do_kill(){
		// if anything needs to be deleted or freed, this is a good time to do it
		return 0;
	}
	public static int check(){
		return 0;
	}
	public static int import_file(String filename){
		return 0;
	}
	public static int export_file(String filename){
		return 0;
	}
	public static int setvar(String varname, String value){
		return 0;
	}
	public static int module_test(int argc, String[] argv){
		return 0;
	}
	public static int cmdargs(int argc, String[] argv){
		return 0;
	}
	/* returns a string to fprintf into the C-side stub */
	public String kmldump(long object_addr){
		return "";
	}
}
