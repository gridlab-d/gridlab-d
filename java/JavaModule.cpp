#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <jni.h>

#include "gridlabd.h"
//#include "myclass.h"

#define MODULENAME "JavaModule"
#define MAJOR 1
#define MINOR 0

static JNIEnv *jnienv = NULL;

EXPORT CLASS *javainit(CALLBACKS *fntable, MODULE *module, int argc, char *argv[], JNIEnv *env)
{
	// set the GridLAB core API callback table
	callback = fntable;

	// set the JNI Environment pointer
	jnienv = env;

	jclass cls = jnienv->FindClass(MODULENAME);

	// init is int (*)(long, int, String[])
	jmethodID init_mid = env->GetStaticMethodID(cls, "init", "(JI[Ljava/lang/String;)I");

	if(cls == NULL)
		return NULL;

	if(init_mid == NULL)
		return NULL;

	jobjectArray args = env->NewObjectArray(argc, jnienv->FindClass("[Ljava/lang/String;"), NULL);
	if(args == NULL)
		return NULL;

	jstring jargv[argc];
	for(int i = 0; i < argc; ++i){
		jargv[i] = env->NewStringUTF(argv[i]);
		jnienv->SetObjectArrayElement(jargv[i], i, args);
	}

	jnienv->CallStaticIntMethod(cls, init_mid, (long long)module, argc, jargv);

	// TODO: register each object class by creating its first instance
	//new myclass(module);

	// always return the first class registered
	//return myclass::oclass;

	// JNI cleanup
	jnienv->DeleteLocalRef(jobjectArray);
	for(int i = 0; i < argc; ++i)
		; /* delete the strings */

	return NULL;
}

CDECL int do_kill()
{
	// if anything needs to be deleted or freed, this is a good time to do it
	return 0;
}

#if 0

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

#endif

/* EOF */
