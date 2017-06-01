/* $id$
 * Copyright (C) 2008 Battelle Memorial Institute
 */

#include "gridlabd.h"
#include "gridlabd_java.h"

#define JAVA_CLASS_PATH "Win32/Debug/"
#define JAVA_LIB_PATH "Win32/Debug/"

static JavaVM *jvm = NULL;
char256 classpath, libpath;

char *get_classpath(){return classpath;}
char *get_libpath(){return libpath;}

static JAVACALLBACKS jcallbacks = {
	get_jvm,
	get_env,
	set_obj,
};

JavaVM *init_jvm();

CDECL JavaVM *get_jvm(){
	return jvm ? jvm : init_jvm();
}

void *get_jcb(){
	return &jcallbacks;
}

CDECL JNIEnv *get_env(){
	JNIEnv *env;
	if(jvm == NULL)
		init_jvm();
	jvm->AttachCurrentThread((void **)&env, NULL);
	return env;
}

CDECL void *set_obj(jobject obj, OBJECT * object_addr, void *block_addr, int id){
	JNIEnv *jnienv = get_env();
	jclass cls = jnienv->FindClass("GObject");

	if(cls == NULL){
		gl_error("%s(%s): unable to find GObject.class", __FILE__, __LINE__);
		return NULL;
	}

	jmethodID mid = jnienv->GetMethodID(cls, "SetObject", "(JJI)V");

	if(mid == NULL){
		gl_error("%s(%s): unable to find \"void GObject.SetObject(long, long, int)\"", __FILE__, __LINE__);
		return NULL;
	}
	jnienv->CallVoidMethod(obj, mid, (int64)object_addr, (int64)block_addr, id);
	return NULL;
}

EXPORT int do_kill()
{
	/* the following block drops us into an infinite loop in strange locations */
	#if 0
	// if anything needs to be deleted or freed, this is a good time to do it
	if(jvm->DestroyJavaVM() == 0){;
		jvm = NULL;
		jnienv = NULL;
	} else {
		gl_error("javamod/init.cpp:do_kill() - error with jvm->DestroyJavaVM()");
	}
	#endif
	return 0;
}

EXPORT void *getvar(char *varname, char *value, unsigned int size){
	void *rv = NULL;
	if(strcmp(varname, "jcallback") == 0)
	{
		rv = (void *)&jcallbacks;
	}
	else if(strcmp(varname, "jvm") == 0)
	{
		rv = (void *)get_jvm();
	}
	else if(strcmp(varname, "jnienv") == 0)
	{
		rv = (void *)get_env();
	}
	return rv;
}

JavaVM *init_jvm(){
	JavaVMOption options[2];
	JavaVMInitArgs vm_args;
	JNIEnv *jnienv = NULL;
	int rv = 0;
	PROPERTYTYPE p;

	if(sizeof(void *) == sizeof(int32))
		p = PT_int32;
	if(sizeof(void *) == sizeof(int64))
		p = PT_int64;
	char cpath[1025], lpath[1025];
	sprintf(cpath, "-Djava.class.path=%s", get_classpath());
	sprintf(lpath, "-Djava.library.path=%s", get_libpath());
	options[0].optionString = cpath;
	options[1].optionString = lpath;
	//options[0].optionString = "-Djava.class.path=Win32/Debug/";
	//options[1].optionString = "-Djava.library.path=Win32/Debug/";

	memset(&vm_args, 0, sizeof(vm_args));
	vm_args.version = JNI_VERSION_1_6;
	vm_args.nOptions = 2;
	vm_args.options = options;

	printf("Creating JVM...\n");
	rv = JNI_CreateJavaVM(&jvm, (void **)&jnienv, &vm_args);
	if(rv == 0)
		printf("JVM successfully created.\n");
	else
		printf("JVM creation failed.\n");

	gl_global_create("glmjava::jvm",p,get_jvm(),PT_ACCESS,PA_REFERENCE,NULL);
	gl_global_create("glmjava::jnienv",p,get_env(),PT_ACCESS,PA_REFERENCE,NULL);

	return jvm;
}

/*
 * GridlabD.Object JNI functions
 */

EXPORT jint JNICALL Java_gridlabd_GObject__1GetID(JNIEnv *env, jclass _this, jlong oaddr){
	OBJECT *obj = (OBJECT *)oaddr;
	return obj->id;
}

EXPORT jlong JNICALL Java_gridlabd_GObject__1GetOClass(JNIEnv *env, jclass _this, jlong oaddr){
	OBJECT *obj = (OBJECT *)oaddr;
	return (int64)(obj->oclass);
}

EXPORT jlong JNICALL Java_gridlabd_GObject__1GetNext(JNIEnv *env, jclass _this, jlong oaddr){
	OBJECT *obj = (OBJECT *)oaddr;
	return (int64)(obj->next);
}

EXPORT jstring JNICALL Java_gridlabd_GObject__1GetName(JNIEnv *env, jclass _this, jlong oaddr){
	OBJECT *obj = (OBJECT *)oaddr;
	return env->NewStringUTF(obj->name);
}

EXPORT jlong JNICALL Java_gridlabd_GObject__1GetParent(JNIEnv *env, jclass _this, jlong oaddr){
	OBJECT *obj = (OBJECT *)oaddr;
	return (int64)(obj->parent);
}

EXPORT jint JNICALL Java_gridlabd_GObject__1GetRank(JNIEnv *env, jclass _this, jlong oaddr){
	OBJECT *obj = (OBJECT *)oaddr;
	return obj->rank;
}

EXPORT jlong JNICALL Java_gridlabd_GObject__1GetClock(JNIEnv *env, jclass _this, jlong oaddr){
	OBJECT *obj = (OBJECT *)oaddr;
	return obj->clock;
}

EXPORT jdouble JNICALL Java_gridlabd_GObject__1GetLongitude(JNIEnv *env, jclass _this, jlong oaddr){
	OBJECT *obj = (OBJECT *)oaddr;
	return obj->longitude;
}

EXPORT jdouble JNICALL Java_gridlabd_GObject__1GetLatitude(JNIEnv *env, jclass _this, jlong oaddr){
	OBJECT *obj = (OBJECT *)oaddr;
	return obj->latitude;
}

EXPORT jlong JNICALL Java_gridlabd_GObject__1GetInSvc(JNIEnv *env, jclass _this, jlong oaddr){
	OBJECT *obj = (OBJECT *)oaddr;
	return (int64)(obj->in_svc);
}

EXPORT jlong JNICALL Java_gridlabd_GObject__1GetOutSvc(JNIEnv *env, jclass _this, jlong oaddr){
	OBJECT *obj = (OBJECT *)oaddr;
	return (int64)(obj->out_svc);
}

EXPORT jlong JNICALL Java_gridlabd_GObject__1GetFlags(JNIEnv *env, jclass _this, jlong oaddr){
	OBJECT *obj = (OBJECT *)oaddr;
	return (int64)(obj->flags);
}

EXPORT void JNICALL Java_gridlabd_GObject__1SetParent(JNIEnv *env, jclass _this, jlong oaddr, jlong paddr){
	OBJECT *obj = (OBJECT *)oaddr;
	obj->parent = (OBJECT *)paddr;
}

EXPORT void JNICALL Java_gridlabd_GObject__1SetFlags(JNIEnv *env, jclass _this, jlong oaddr, jint flags){
	OBJECT *obj = (OBJECT *)oaddr;
	obj->flags ^= (unsigned long)flags;
}


EXPORT jstring JNICALL Java_gridlabd_GObject__1GetStringProp(JNIEnv *env, jclass _this, jlong oaddr, jlong offset, jlong size){
	char buffer[1029];
	memcpy(buffer, (void *)(oaddr+offset+sizeof(OBJECT)), (size_t)size);
	return env->NewStringUTF(buffer);
}

EXPORT void JNICALL Java_gridlabd_GObject__1SetStringProp(JNIEnv *env, jclass _this, jlong oaddr, jlong offset, jlong size, jstring str){
	const char *buffer = env->GetStringUTFChars(str, NULL);
	size_t s = env->GetStringUTFLength(str);
	memcpy((void *)(oaddr+offset+sizeof(OBJECT)), buffer, (s < size) ? s : (size_t)size);
}

EXPORT jint JNICALL Java_gridlabd_GObject__1GetInt16Prop(JNIEnv *env, jclass _this, jlong oaddr, jlong offset){
	return *(int16 *)(oaddr+offset+sizeof(OBJECT));
}

EXPORT void JNICALL Java_gridlabd_GObject__1SetInt16Prop(JNIEnv *env, jclass _this, jlong oaddr, jlong offset, jint val){
	*(int16 *)(oaddr+offset+sizeof(OBJECT)) = (int16)val;
}

EXPORT jint JNICALL Java_gridlabd_GObject__1GetInt32Prop(JNIEnv *env, jclass _this, jlong oaddr, jlong offset){
	return *(int32 *)(oaddr+offset+sizeof(OBJECT));
}

EXPORT void JNICALL Java_gridlabd_GObject__1SetInt32Prop(JNIEnv *env, jclass _this, jlong oaddr, jlong offset, jint val){
	*(int32 *)(oaddr+offset+sizeof(OBJECT)) = val;
}

EXPORT jlong JNICALL Java_gridlabd_GObject__1GetInt64Prop(JNIEnv *env, jclass _this, jlong oaddr, jlong offset){
	return *(int64 *)(oaddr+offset+sizeof(OBJECT));
}

EXPORT jlong JNICALL Java_gridlabd_GObject__1GetObjectProp(JNIEnv *env, jclass _this, jlong oaddr, jlong offset){
	return *(int64 *)(oaddr+offset+sizeof(OBJECT));
}

EXPORT void JNICALL Java_gridlabd_GObject__1SetInt64Prop(JNIEnv *env, jclass _this, jlong oaddr, jlong offset, jlong val){
	*(int64 *)(oaddr+offset+sizeof(OBJECT)) = val;
}

EXPORT jdouble JNICALL Java_gridlabd_GObject__1GetDoubleProp(JNIEnv *env, jclass _this, jlong oaddr, jlong offset){
	return *(double *)(oaddr+offset+sizeof(OBJECT));
}

EXPORT void JNICALL Java_gridlabd_GObject__1SetDoubleProp(JNIEnv *env, jclass _this, jlong oaddr, jlong offset, jdouble val){
	*(double *)(oaddr+offset+sizeof(OBJECT)) = val;
}

EXPORT jdouble JNICALL Java_gridlabd_GObject__1GetComplexRealProp(JNIEnv *env, jclass _this, jlong oaddr, jlong offset){
	complex *c = (complex *)(oaddr+offset+sizeof(OBJECT));
	return c->Re();
}

EXPORT jdouble JNICALL Java_gridlabd_GObject__1GetComplexImagProp(JNIEnv *env, jclass _this, jlong oaddr, jlong offset){
	complex *c = (complex *)(oaddr+offset+sizeof(OBJECT));
	return c->Im();
}

EXPORT void JNICALL Java_gridlabd_GObject__1SetComplexProp(JNIEnv *env, jclass _this, jlong oaddr, jlong offset, jdouble re, jdouble im){
	complex *c=(complex *)(oaddr+offset+sizeof(OBJECT));
	c->SetReal(re);
	c->SetImag(im);
}

/*
 *	GridlabD.Property functions
 */

/*
 *	GridlabD callback JNI functions
 */

EXPORT jint JNICALL Java_gridlabd_GridlabD_verbose(JNIEnv *env, jclass _this, jstring str){
	char *instr = (char *)env->GetStringUTFChars(str, NULL);
	int len = (int)strlen(instr);
	gl_verbose(instr);
//	env->ReleaseStringUTFChars(str, instr);
	return len;
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_output(JNIEnv *env, jclass _this, jstring str){
	char *instr = (char *)env->GetStringUTFChars(str, NULL);
	int len = (int)strlen(instr);
	gl_output(instr);
	env->ReleaseStringUTFChars(str, instr);
	return len;
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_warning(JNIEnv *env, jobject _this, jstring str){
	char *instr = (char *)env->GetStringUTFChars(str, NULL);
	int len = (int)strlen(instr);
	gl_warning(instr);
	env->ReleaseStringUTFChars(str, instr);
	return len;
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_error(JNIEnv *env, jobject _this, jstring str){
	char *instr = (char *)env->GetStringUTFChars(str, NULL);
	int len = (int)strlen(instr);
	gl_error(instr);
	env->ReleaseStringUTFChars(str, instr);
	return len;
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_debug(JNIEnv *env, jobject _this, jstring str){
	char *instr = (char *)env->GetStringUTFChars(str, NULL);
	int len = (int)strlen(instr);
	gl_debug(instr);
	env->ReleaseStringUTFChars(str, instr);
	return len;
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_testmsg(JNIEnv *env, jobject _this, jstring str){
	char *instr = (char *)env->GetStringUTFChars(str, NULL);
	int len = (int)strlen(instr);
	gl_testmsg(instr);
	env->ReleaseStringUTFChars(str, instr);
	return len;
}

//
// gl_malloc is not exposed ~ dynamic memory in Java for use within Java should be kept in the JVM. -MH
//

EXPORT jlong JNICALL Java_gridlabd_GridlabD_module_find(JNIEnv *env, jobject _this, jstring modulename){
	int64 rv = 0;
	char *instr = (char *)env->GetStringUTFChars(modulename, NULL);
	rv = (int64)gl_find_module(instr);
	env->ReleaseStringUTFChars(modulename, instr);
	return rv;
}

EXPORT jlong JNICALL Java_gridlabd_GridlabD_get_module_var(JNIEnv *env, jobject _this, jstring modulename, jstring varname){
	int64 rv = 0;
	char *instr = (char *)env->GetStringUTFChars(modulename, NULL);
	MODULE *module = gl_find_module(instr);
	if(module != NULL){
		char *varstr = (char *)env->GetStringUTFChars(varname, NULL);
		void *addr = gl_get_module_var(module, varstr);
		env->ReleaseStringUTFChars(varname, varstr);
		rv = (int64)addr;
	}
	env->ReleaseStringUTFChars(modulename, instr);
	return rv;
}

EXPORT jstring JNICALL Java_gridlabd_GridlabD_findfile(JNIEnv *env, jobject _this, jstring filename){
	char *instr = (char *)env->GetStringUTFChars(filename, NULL);
	char buffer[256];
	char *filestr = gl_findfile(instr, NULL, 0, buffer, 255);
	jstring rv = NULL;
	if(filestr)
		rv = env->NewStringUTF(filestr);
	env->ReleaseStringUTFChars(filename, instr);
	return rv;
}

EXPORT jlong JNICALL Java_gridlabd_GridlabD_find_1module(JNIEnv *env, jobject _this, jstring modulename){
	char *instr = (char *)env->GetStringUTFChars(modulename, NULL);
	int64 rv = (int64)gl_find_module(instr);
	env->ReleaseStringUTFChars(modulename, instr);
	return rv;
}

// new
EXPORT jlong JNICALL Java_gridlabd_GridlabD_find_1property(JNIEnv *env, jobject _this, jlong oclass_addr, jstring propname){
	char *instr = (char *)env->GetStringUTFChars(propname, NULL);
	int64 rv = (int64)(gl_find_property((CLASS *)oclass_addr, instr));
	env->ReleaseStringUTFChars(propname, instr);
	return rv;
}

// new
EXPORT jint JNICALL Java_gridlabd_GridlabD_module_1depends(JNIEnv *env, jobject _this, jstring modname, jint major, jint minor, jint build){
	char *instr = (char *)env->GetStringUTFChars(modname, NULL);
	int rv = gl_module_depends(instr, (unsigned char)major, (unsigned char)minor, (unsigned char)build);
	return rv;
}

EXPORT jlong JNICALL Java_gridlabd_GridlabD_register_1class(JNIEnv *env, jobject _this, jlong moduleaddr, jstring classname, jint passconfig){
	char *instr = (char *)env->GetStringUTFChars(classname, NULL);
	int64 rv = (int64)gl_register_class((MODULE *)moduleaddr, instr, 0, (PASSCONFIG)(0xFF & passconfig));
	env->ReleaseStringUTFChars(classname, instr);
	return rv;
}

EXPORT jlong JNICALL Java_gridlabd_GridlabD_create_1object(JNIEnv *env, jobject _this, jlong oclass_addr, jlong size){
	return (int64)(gl_create_object((CLASS *)oclass_addr));
}

EXPORT jlong JNICALL Java_gridlabd_GridlabD_create_1array(JNIEnv *env, jobject _this, jlong oclass_addr, jlong size, jint n_objects){
	return (int64)(gl_create_array((CLASS *)oclass_addr, n_objects));
}

// new
EXPORT jlong JNICALL Java_gridlabd_GridlabD_create_foreign(JNIEnv *env, jobject _this, jlong inobj){
	OBJECT *ptr = gl_create_foreign((OBJECT *)inobj);
	return (jlong)ptr;
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_object_1isa(JNIEnv *env, jobject _this, jlong obj_addr, jstring _typename){
	char *name = (char *)env->GetStringUTFChars(_typename, NULL);
	int rv = gl_object_isa((OBJECT *)obj_addr, name);
	env->ReleaseStringUTFChars(_typename, name);
	return rv;
}

EXPORT jlong JNICALL Java_gridlabd_GridlabD_publish_1variable(JNIEnv *env, jobject _this, jlong moduleaddr, jlong classaddr, jstring varname, jstring vartype, jlong offset){
	static struct s_map {
		char *name;
		PROPERTYTYPE pt;
		int size;
	} map[] = {
		{"int16", PT_int16, sizeof(int16)},
		{"int32", PT_int32, sizeof(int32)},
		{"int64", PT_int64, sizeof(int64)},
		{"double", PT_double, sizeof(double)},
		{"char8", PT_char8, sizeof(char8)},
		{"char32", PT_char32, sizeof(char8)},
		{"char256", PT_char256, sizeof(char256)},
		{"char1024", PT_char1024, sizeof(char1024)},
		{"complex", PT_complex, sizeof(complex)},
		{"object", PT_object, sizeof(OBJECT *)},
	};
	char *varstr = (char *)env->GetStringUTFChars(varname, NULL);
	char *typestr = (char *)env->GetStringUTFChars(vartype, NULL);
	PROPERTY *prop = NULL;
	int i = 0;
	for(i = 0; i < (sizeof(map)/sizeof(map[0])); ++i){
		if(0 == strcmp(map[i].name, typestr))
			break;
	}
	if(i < sizeof(map)/sizeof(map[0])){
		if(gl_publish_variable((CLASS *)(classaddr),map[i].pt, varstr, (void*)offset, PT_EXTEND, NULL) != 0){
			env->ReleaseStringUTFChars(varname, varstr);
			env->ReleaseStringUTFChars(vartype, typestr);
			return map[i].size;
		} else {
			gl_error("unable to publish property %s in class %s", varstr, ((CLASS *)(classaddr))->name);
			env->ReleaseStringUTFChars(varname, varstr);
			env->ReleaseStringUTFChars(vartype, typestr);
			return 0;
		}
	} else {
		gl_error("unrecognized proptype %s", typestr);
		env->ReleaseStringUTFChars(varname, varstr);
		env->ReleaseStringUTFChars(vartype, typestr);
		return 0;
	}
}

EXPORT jlong JNICALL Java_gridlabd_GridlabD_publish_1function(JNIEnv *env, jobject _this, jlong moduleaddr, jlong classaddr, jstring funcname){
	gl_error("publish_function from Java not supported (still working on mapping fuctions to methods)");
	return 0; /*****/
}

//FUNCTIONADDR class_get_function(char *classname, char *functionname)
EXPORT jlong JNICALL Java_gridlabd_GridlabD_get_1function(JNIEnv *env, jobject _this, jstring classstr, jstring ftnstr){
	gl_error("get_function from Java not supported (still working on mapping functions to methods)");
	return 0;
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_set_1dependent(JNIEnv *env, jobject _this, jlong from_addr, jlong to_addr){
	return gl_set_dependent((OBJECT *)from_addr, (OBJECT *)to_addr);
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_set_1parent(JNIEnv *env, jobject _this, jlong child_addr, jlong parent_addr){
	return gl_set_parent((OBJECT *)child_addr, (OBJECT *)parent_addr);
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_register_1type(JNIEnv *env, jobject _this, jlong oclass_addr, jstring _typename, jstring from_string_fname, jstring to_string_fname){
	gl_error("register_type from Java not supported (still working on mapping fuctions to methods)");
	return 0; /*****/
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_publish_1delegate(JNIEnv *env, jobject _this){
	gl_error("delegated types not supported using class_define_type (use class_define_map instead)");
	return 0;
}

EXPORT jlong JNICALL Java_gridlabd_GridlabD_get_1property(JNIEnv *env, jobject _this, jlong oaddr, jstring propertyname){
	char *pstr = (char *)env->GetStringUTFChars(propertyname, NULL);
	OBJECT *obj = (OBJECT *)oaddr;
	int64 rv = (int64)gl_get_property(obj, pstr);
	env->ReleaseStringUTFChars(propertyname, pstr);
	return rv;
}

// does not have gl_ parallel?
EXPORT jlong JNICALL Java_gridlabd_GridlabD_get_1property_1by_1name(JNIEnv *env, jobject _this, jstring objectname, jstring propertyname){
	char *ostr = (char *)env->GetStringUTFChars(objectname, NULL);
	char *pstr = (char *)env->GetStringUTFChars(propertyname, NULL);
	OBJECT *obj = gl_get_object(ostr);
	int64 rv = (int64)gl_get_property(obj, pstr);
	env->ReleaseStringUTFChars(propertyname, pstr);
	env->ReleaseStringUTFChars(objectname, ostr);
	return rv;
}

EXPORT jstring JNICALL Java_gridlabd_GridlabD_get_1value(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname){
	char buffer[1025];
	char *pstr = (char *)env->GetStringUTFChars(propertyname, NULL);
	PROPERTY *prop = gl_get_property((OBJECT *)object_addr, pstr);
	// GETADDR(O,P) (O?((void*)((char*)(O+1)+(unsigned int64)((P)->addr))):NULL)
	int rv = gl_get_value((OBJECT *)object_addr, (void *)(object_addr+sizeof(OBJECT)+(int64)prop->addr), buffer, 1024, prop);
	env->ReleaseStringUTFChars(propertyname, pstr);
	return rv ? env->NewStringUTF(buffer) : NULL;
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_set_1value(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname, jstring value){
	char *vstr = (char *)env->GetStringUTFChars(value, NULL);
	char *pstr = (char *)env->GetStringUTFChars(propertyname, NULL);
	OBJECT *obj = (OBJECT *)object_addr;
	PROPERTY *prop = gl_get_property(obj, pstr);
	int rv = gl_set_value(obj, (void *)(object_addr + sizeof(OBJECT) + (int64)prop->addr), vstr, prop);
	if(rv == 0){
		gl_error("unable to set_value for %s.%s to %s", obj->oclass->name, pstr, vstr);
	}
	env->ReleaseStringUTFChars(propertyname, pstr);
	env->ReleaseStringUTFChars(value, vstr);
	return rv;
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_unit_1convert(JNIEnv *env, jobject _this, jstring from, jstring to, jdouble invalue){
	char *fromstr = (char *)env->GetStringUTFChars(from, NULL);
	char *tostr = (char *)env->GetStringUTFChars(to, NULL);
	double rv = gl_convert(fromstr, tostr, &invalue);
	env->ReleaseStringUTFChars(from, fromstr);
	env->ReleaseStringUTFChars(to, tostr);
	return rv;
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_get_1complex_1real(JNIEnv *env, jobject _this, jlong object_addr, jlong property_addr){
	return (gl_get_complex((OBJECT *)object_addr, (PROPERTY *)property_addr))->Re();
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_get_1complex_1_real_1by_1name(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname){
	char *pname =(char *)env->GetStringUTFChars(propertyname, NULL);
	double rv = (gl_get_complex_by_name((OBJECT *)object_addr, pname))->Re();
	env->ReleaseStringUTFChars(propertyname, pname);
	return rv;
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_get_1complex_1imag(JNIEnv *env, jobject _this, jlong object_addr, jlong property_addr){
	return (gl_get_complex((OBJECT *)object_addr, (PROPERTY *)property_addr))->Im();
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_get_1complex_1imag_1by_1name(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname){
	char *pname =(char *)env->GetStringUTFChars(propertyname, NULL);
	double rv = (gl_get_complex_by_name((OBJECT *)object_addr, pname))->Im();
	env->ReleaseStringUTFChars(propertyname, pname);
	return rv;
}

EXPORT jlong JNICALL Java_gridlabd_GridlabD_get_1object(JNIEnv *env, jobject _this, jstring oname){
	char *name =(char *)env->GetStringUTFChars(oname, NULL);
	int64 rv = (int64)gl_get_object(name);
	env->ReleaseStringUTFChars(oname, name);
	return rv;
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_get_1int16(JNIEnv *env, jobject _this, jlong object_addr, jlong property_addr){
	return *gl_get_int16((OBJECT *)object_addr, (PROPERTY *)property_addr);
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_get_1int16_1by_1name(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname){
	char *pname =(char *)env->GetStringUTFChars(propertyname, NULL);
	int16 rv = *gl_get_int16_by_name((OBJECT *)object_addr, pname);
	env->ReleaseStringUTFChars(propertyname, pname);
	return rv;
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_get_1int32(JNIEnv *env, jobject _this, jlong object_addr, jlong property_addr){
	return *gl_get_int32((OBJECT *)object_addr, (PROPERTY *)property_addr);
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_get_1int32_1by_1name(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname){
	char *pname =(char *)env->GetStringUTFChars(propertyname, NULL);
	int32 rv = *gl_get_int32_by_name((OBJECT *)object_addr, pname);
	env->ReleaseStringUTFChars(propertyname, pname);
	return rv;
}

EXPORT jlong JNICALL Java_gridlabd_GridlabD_get_1int64(JNIEnv *env, jobject _this, jlong object_addr, jlong property_addr){
	return *gl_get_int64((OBJECT *)object_addr, (PROPERTY *)property_addr);
}

EXPORT jlong JNICALL Java_gridlabd_GridlabD_get_1int64_1by_1name(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname){
	char *pname =(char *)env->GetStringUTFChars(propertyname, NULL);
	int64 rv = *gl_get_int64_by_name((OBJECT *)object_addr, pname);
	env->ReleaseStringUTFChars(propertyname, pname);
	return rv;
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_get_1double(JNIEnv *env, jobject _this, jlong object_addr, jlong property_addr){
	return *gl_get_double((OBJECT *)object_addr, (PROPERTY *)property_addr);
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_get_1double_1by_1name(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname){
	char *pname =(char *)env->GetStringUTFChars(propertyname, NULL);
	int l = env->GetStringUTFLength(propertyname);
	double rv = *gl_get_double_by_name((OBJECT *)object_addr, pname);
	env->ReleaseStringUTFChars(propertyname, pname);
	return rv;
}

EXPORT jstring JNICALL Java_gridlabd_GridlabD_get_1string(JNIEnv *env, jobject _this, jlong object_addr, jlong property_addr){
	char *str = gl_get_string((OBJECT *)object_addr, (PROPERTY *)property_addr);
	return env->NewStringUTF(str);
}

EXPORT jstring JNICALL Java_gridlabd_GridlabD_get_1string_1by_1name(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname){
	char *pname =(char *)env->GetStringUTFChars(propertyname, NULL);
	char *str = gl_get_string_by_name((OBJECT *)object_addr, pname);
	env->ReleaseStringUTFChars(propertyname, pname);
	return env->NewStringUTF(str);
}

EXPORT jlong JNICALL Java_gridlabd_GridlabD_get_1object_1prop(JNIEnv *env, jobject _this, jlong object_addr, jlong property_addr){
	PROPERTY *p = (PROPERTY *)(property_addr);
	int64 i = (int64)(gl_get_object_prop((OBJECT *)object_addr, p));
	return i;
}

// new for glmjava
EXPORT jlong JNICALL Java_gridlabd_GridlabD_get_1object_1by_1name(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname){
	char *pname =(char *)env->GetStringUTFChars(propertyname, NULL);
	int64 rv = (int64)gl_get_object_prop((OBJECT *)object_addr, gl_get_property((OBJECT *)object_addr, pname));
	env->ReleaseStringUTFChars(propertyname, pname);
	return rv;
}

//	 The following gl functions are not mapped with the JNI due to the bitwise nature of the FINDLIST struct.  -mhauer
// gl_find_objects
// gl_find_next
// gl_findlist_copy
// gl_findlist_add
// gl_findlist_del
// gl_findlist_clear

// gl_free is not relevant to glmjava modules. -mhauer

EXPORT jlong JNICALL Java_gridlabd_GridlabD_create_1aggregate(JNIEnv *env, jobject _this, jstring aggregator, jstring group_expression){
	char *agstr = (char *)env->GetStringUTFChars(aggregator, NULL);
	char *expstr = (char *)env->GetStringUTFChars(group_expression, NULL);
	int64 rv = (int64)gl_create_aggregate(agstr, expstr);
	env->ReleaseStringUTFChars(group_expression, expstr);
	env->ReleaseStringUTFChars(aggregator, agstr);
	return rv;
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_run_1aggregate(JNIEnv *env, jobject _this, jlong aggregate_addr){
	return gl_run_aggregate((s_aggregate *)aggregate_addr);
}

// gl_randomtype
// gl_randomvalue
// gl_psuedorandom

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_random_1unifrom(JNIEnv *env, jobject _this, jdouble a, jdouble b){
	return gl_random_uniform(0, a, b);
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_random_1normal(JNIEnv *env, jobject _this, jdouble m, jdouble s){
	return gl_random_normal(0, m, s);
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_random_1lognormal(JNIEnv *env, jobject _this, jdouble m, jdouble s){
	return gl_random_lognormal(0, m, s);
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_random_1bernoulli(JNIEnv *env, jobject _this, jdouble p){
	return gl_random_bernoulli(0, p);
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_random_1pareto(JNIEnv *env, jobject _this, jdouble m, jdouble s){
	return gl_random_pareto(0, m, s);
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_random_1sampled(JNIEnv *env, jobject _this, jint n, jdoubleArray x){
	double *_x = env->GetDoubleArrayElements(x, NULL);
	double rv = gl_random_sampled(0, n, _x);
	env->ReleaseDoubleArrayElements(x, _x, 0);
	return rv;
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_random_1exponential(JNIEnv *env, jobject _this, jdouble l){
	return gl_random_exponential(0, l);
}

// gl_random_triangle
// gl_random_gamma
// gl_random_beta
// gl_random_weibull
// gl_random_rayleight

// gl_globalclock

EXPORT jlong JNICALL Java_gridlabd_GridlabD_parsetime(JNIEnv *env, jobject _this, jstring value){
	char *timestr = (char *)env->GetStringUTFChars(value, NULL);
	int64 rv = gl_parsetime(timestr);
	env->ReleaseStringUTFChars(value, timestr);
	return rv;
}

// gl_printtime
// gl_mktime
// gl_strtime

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_todays(JNIEnv *env, jobject _this, jlong t){
	return gl_todays(t);
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_tohours(JNIEnv *env, jobject _this, jlong t){
	return gl_tohours(t);
}

EXPORT jdouble JNICALL Java_gridlabd_GridlabD_tominutes(JNIEnv *env, jobject _this, jlong t){
	return gl_tominutes(t);
}

// gl_toseconds
// gl_localtime
// gl_getweekday
// gl_gethour

EXPORT jlong JNICALL Java_gridlabd_GridlabD_global_1create(JNIEnv *env, jobject _this, jstring name, jstring args){
	char *namestr = (char *)env->GetStringUTFChars(name, NULL);
	char *argstr = (char *)env->GetStringUTFChars(args, NULL);
	int rv = gl_global_setvar(namestr, argstr);
	env->ReleaseStringUTFChars(args, argstr);
	env->ReleaseStringUTFChars(name, namestr);
	return rv;
}

EXPORT jint JNICALL Java_gridlabd_GridlabD_global_1setvar(JNIEnv *env, jobject _this, jstring def, jstring args){
	char *namestr = (char *)env->GetStringUTFChars(def, NULL);
	char *argstr = (char *)env->GetStringUTFChars(args, NULL);
	int rv = gl_global_setvar(namestr, argstr);
	env->ReleaseStringUTFChars(args, argstr);
	env->ReleaseStringUTFChars(def, namestr);
	return rv;
}

EXPORT jstring JNICALL Java_gridlabd_GridlabD_global_1getvar(JNIEnv *env, jobject _this, jstring name){
	char buffer[1025];
	char *namestr = (char *)env->GetStringUTFChars(name, NULL);
	int64 rv = (int64)gl_global_getvar(namestr, buffer, 1024);
	env->ReleaseStringUTFChars(name, namestr);
	return rv ? env->NewStringUTF(buffer) : NULL;
}

EXPORT jlong JNICALL Java_gridlabd_GridlabD_global_1find(JNIEnv *env, jobject _this, jstring name){
	char *namestr = (char *)env->GetStringUTFChars(name, NULL);
	int64 rv = (int64)gl_global_find(namestr);
	env->ReleaseStringUTFChars(name, namestr);
	return rv;
}

// clip
EXPORT jdouble Java_gridlabd_GridlabD_clip(JNIEnv *env, jobject _this, jdouble x, jdouble a, jdouble b){
	return clip(x, a, b);
}

// bitof
EXPORT jint Java_gridlabd_GridlabD_bitof(JNIEnv *env, jobject _this, jlong x){
	return bitof(x);
}

// gl_name
EXPORT jstring Java_gridlabd_GridlabD_name(JNIEnv *env, jobject _this, jlong myaddr){
	char buffer[256];
	char *ptr;
	OBJECT *my = (OBJECT *)myaddr;
	if(my == 0){
		return NULL;
	}
	ptr = gl_name(my, buffer, 255);
	if(ptr){
		return env->NewStringUTF(buffer);
	} else {
		return NULL;
	}
}

// gl_schedule_find
JNIEXPORT jlong JNICALL Java_gridlabd_GridlabD_find_1schedule(JNIEnv *env, jobject _this, jstring name){
	char *namestr = (char *)env->GetStringUTFChars(name, NULL);
	return (jlong)(callback->schedule.find(namestr));
}

// gl_schedule_create
JNIEXPORT jlong JNICALL Java_gridlabd_GridlabD_schedule_1create(JNIEnv *env, jobject _this, jstring name, jstring def){
	char *namestr = (char *)(env->GetStringUTFChars(name, NULL));
	char *definition = (char *)(env->GetStringUTFChars(def, NULL));
	return (jlong)(callback->schedule.create(namestr, definition));
}

// gl_schedule_index
JNIEXPORT jlong JNICALL Java_gridlabd_GridlabD_schedule_1index(JNIEnv *env, jobject _this, jlong sch, jlong ts){
	return (jlong)(callback->schedule.index((SCHEDULE *)(sch), (TIMESTAMP)(ts)));
}

// gl_schedule_value
JNIEXPORT jdouble JNICALL Java_gridlabd_GridlabD_schedule_1value(JNIEnv *env, jobject _this, jlong sch, jlong index){
	return (jdouble)(callback->schedule.value((SCHEDULE *)sch, (SCHEDULEINDEX)index));
}

// gl_schedule_dtnext
JNIEXPORT jlong JNICALL Java_gridlabd_GridlabD_schedule_1dtnext(JNIEnv *env, jobject _this, jlong sch, jlong index){
	return (jlong)(callback->schedule.dtnext((SCHEDULE *)(sch), (SCHEDULEINDEX)index));
}

// gl_enduse_create
JNIEXPORT jlong JNICALL Java_gridlabd_GridlabD_enduse_1create(JNIEnv *env, jobject _this){
	enduse *eu = (enduse *)malloc(sizeof(enduse));
	memset(eu, 0, sizeof(enduse));
	if(1 == (callback->enduse.create(eu))){
		return (jlong)(eu);
	} else {
		return 0;
	}
}

// gl_enduse_sync
JNIEXPORT jlong JNICALL Java_gridlabd_GridlabD_enduse_1sync(JNIEnv *env, jobject _this, jlong e, jlong t1){
	return (callback->enduse.sync((enduse *)e, PC_BOTTOMUP,(TIMESTAMP)t1));
}

// gl_loadshape_create
JNIEXPORT jlong JNICALL Java_gridlabd_GridlabD_loadshape_1create(JNIEnv *env, jobject _this, jlong sched_addr){
	loadshape *ls;
	if(sched_addr == 0){
		return 0;
	}
	ls = (loadshape *)malloc(sizeof(loadshape));
	memset(ls, 0, sizeof(loadshape));
	if(0 == callback->loadshape.create(ls)){
		return 0;
	} 
	ls->schedule = (SCHEDULE *)sched_addr;
	return (jlong)ls;
}

// gl_get_loadshape_value
JNIEXPORT jdouble JNICALL Java_gridlabd_GridlabD_get_1loadshape_1value(JNIEnv *env, jobject _this, jlong ls_addr){
	if(ls_addr != 0){
		loadshape *ls = (loadshape *)ls_addr;
		return (jdouble)(ls->load);
	} else {
		return 0.0;
	}
}

// not sure about passing DATETIME structures through the JNI. -mhauer
// gl_strftime(DATETIME *, char *, int)
//JNIEXPORT jstring JNICALL Java_gridlabd_GridlabD_strftime(JNIEnv *env, jobject _this, );

// gl_strftime(TIMESTAMP)
JNIEXPORT jstring JNICALL Java_gridlabd_GridlabD_(JNIEnv *env, jobject _this, jlong ts){
	char buffer[64];
	DATETIME dt;
	strcpy(buffer,"(invalid time)");
	if(gl_localtime(ts, &dt)){
		gl_strftime(&dt, buffer, sizeof(buffer));
	}
	return env->NewStringUTF(buffer);
}

// gl_lerp
JNIEXPORT jdouble JNICALL Java_gridlabd_GridlabD_lerp(JNIEnv *env, jobject _this, jdouble t, jdouble x0, jdouble y0, jdouble x1, jdouble y1){
	return gl_lerp(t, x0, y0, x1, y1);
}

// gl_qerp
JNIEXPORT jdouble JNICALL Java_gridlabd_GridlabD_qerp(JNIEnv *env, jobject _this, jdouble t, jdouble x0, jdouble y0, jdouble x1, jdouble y1, jdouble x2, jdouble y2){
	return gl_qerp(t, x0, y0, x1, y1, x2, y2);
}



/*** LINE ***/
/*
JNIEXPORT jint JNICALL Java_gridlabd_GridlabD_(JNIEnv *env, jobject _this, ...);
*/

/* the find object code will need some figuring to get the find list to cooperate, likely will be skipped
 * in favor of straight addrs */
//public static native long[] find_objects(long findlist_addr, String[] args); /* returns findlist *[] */
//public static native long find_next(long findlist_addr); /* returns an object_addr */

/* timestamp objects will be figured out later*/
//public static native long mktime(Object timestamp); /* mkdatetime */
//public static native int strtime(Object timestamp, String buffer, int size); /* strdatetime */
//public static native int localtime(long t, Object datetime); /* local_datetime */

/* EOF */
