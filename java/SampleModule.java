/*
 *
 *
 */

class SampleModule {
	private static GModule module;
	public static long init(long mod_addr, String mod_name, int argc, String[] argv){
		module = new GModule(mod_addr, mod_name);
		System.out.println("Hello, world, from Java!");
		System.out.println("\tmod_name = "+ mod_name);
		System.out.println("\tmod_addr = "+ Long.toHexString(mod_addr));
		GridlabD.output("This is our first attempt to use a C callback from Java!");
		GridlabD.warning("This is a warning from Java");
		GridlabD.error("We really don't like having Java go sideways!");
		GridlabD.debug("Squish!");
		GridlabD.testmsg("This is only a test...");
		GridlabD.output("we can find powerflow.dll at " + GridlabD.findfile("powerflow.dll"));

		/* overt class registration */
		/* not a good idea, take it to register() */
		GClass c1 = GClass.BuildClass("my_java_class", module, 4);
		c1.AddProperty("bingo", "char8");
		c1.AddProperty("zingo", "int16");
		c1.AddProperty("zango", "complex");
		c1.AddProperty("zingo", "double");

		/* class registration with register(GModule) */
		GClass c2 = GClass.BuildClass("SampleClass", module, 4);
		SampleClass.register(c2);
		return module.GetFirstClassAddr();
	}
	public static int do_kill(){
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
	public static String kmldump(long object_addr){
		return "";
	}

}