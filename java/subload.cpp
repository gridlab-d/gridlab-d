// $Id: subload.cpp 4738 2014-07-03 00:55:39Z dchassin $
// Copyright (C) 2008 Battelle Memorial Institute
#include "gridlabd.h"
#include "gridlabd_java.h"
#include "module.h"

#if 0
mod->major = pMajor?*pMajor:0;
mod->minor = pMinor?*pMinor:0;
mod->import_file = (int(*)(char*))DLSYM(hLib,"import_file");
mod->export_file = (int(*)(char*))DLSYM(hLib,"export_file");
mod->setvar = (int(*)(char*,char*))DLSYM(hLib,"setvar");
mod->getvar = (void*(*)(char*,char*,unsigned int))DLSYM(hLib,"getvar");
mod->check = (int(*)())DLSYM(hLib,"check");
mod->module_test = (int(*)(TEST_CALLBACKS*,int,char*[]))DLSYM(hLib,"module_test");
mod->cmdargs = (int(*)(int,char**))DLSYM(hLib,"cmdargs");
mod->kmldump = (int(*)(FILE*,OBJECT*))DLSYM(hLib,"kmldump");
mod->subload = (MODULE *(*)(char *, MODULE **))DLSYM(hLib, "subload");
mod->globals = NULL;
#endif

EXPORT int create_java(OBJECT **obj, OBJECT *parent);
EXPORT int init_java(OBJECT *obj, OBJECT *parent);
EXPORT TIMESTAMP sync_java(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass);
EXPORT int notify_java(OBJECT *obj, NOTIFYMODULE msg);
EXPORT int isa_java(OBJECT *obj, char *classname);
EXPORT int64 plc_java(OBJECT *obj, TIMESTAMP t1);
EXPORT int recalc_java(OBJECT *obj);
EXPORT int commit_java(OBJECT *obj);

CLASS *java_init(CALLBACKS *, JAVACALLBACKS *, MODULE *, char *, int, char *[]);

EXPORT MODULE *subload(char *modname, MODULE **pMod, CLASS **cptr, int argc, char **argv){
	MODULE *mod = (MODULE *)malloc(sizeof(MODULE));
	CLASS *c = NULL;
	memset(mod, 0, sizeof(MODULE));
	gl_output("glmjava: trying to subload \"%s\"", modname);
	java_init(callback, (JAVACALLBACKS *)getvar("jcallback", NULL, NULL), mod, modname, argc, argv);
	c = *cptr;
	while(c != NULL){
		c->create = (FUNCTIONADDR)create_java;
		c->init = (FUNCTIONADDR)init_java;
		c->isa = (FUNCTIONADDR)isa_java;
		c->notify = (FUNCTIONADDR)notify_java;
		c->plc = (FUNCTIONADDR)plc_java;
		c->recalc = (FUNCTIONADDR)recalc_java;
		c->sync = (FUNCTIONADDR)sync_java;
		c->commit = (FUNCTIONADDR)commit_java;
		c = c->next;
	}
	return mod;
}

EXPORT int create_java(OBJECT **obj, OBJECT *parent)
{
	JNIEnv *jnienv = (JNIEnv *)getvar("jnienv", NULL, NULL);
	char *classname = (*obj) ? (*obj)->name : "ERROR_NO_CLASSNAME";
	jclass cls = jnienv->FindClass(classname);
	if(cls == NULL){
		gl_error("create_java: unable to find %s.class", classname);
		return 0;
	}
	jmethodID cfunc = jnienv->GetStaticMethodID(cls, "create", "(J)J");
	if(cfunc == NULL){
		gl_error("create_java: unable to find long %s.create(long)", classname);
		return 0;
	}
	int64 rv = jnienv->CallStaticLongMethod(cls, cfunc, (int64)parent);
	if(rv == 0){
		gl_error("create_java: %s.create() failed", classname);
		GL_THROW("%s.create() failed", classname);
	}
	if (jnienv->ExceptionOccurred()) {
		jnienv->ExceptionDescribe();
	}
	*obj = (OBJECT *)rv;
	gl_set_parent(*obj, parent);
	return 1;
}

EXPORT int init_java(OBJECT *obj, OBJECT *parent){
	JNIEnv *jnienv = (JNIEnv *)getvar("jnienv", NULL, NULL);
	char *name = obj->oclass->name;
	jclass cls = jnienv->FindClass(name);
	if(cls == NULL){
		gl_error("init_java: unable to find %s.class", name);
		return 0;
	}
	jmethodID cfunc = jnienv->GetStaticMethodID(cls, "init", "(JJ)I");
	if(cfunc == NULL){
		gl_error("init_java: unable to find int %s.init(long, long)", name);
		return 0;
	}
	int rv = jnienv->CallStaticIntMethod(cls, cfunc, (int64)obj, (int64)parent);
	return rv;
}

EXPORT int commit_java(OBJECT *obj){
	JNIEnv *jnienv = (JNIEnv *)getvar("jnienv", NULL, NULL);
	char *name = obj->oclass->name;
	jclass cls = jnienv->FindClass(name);
	if(cls == NULL){
		gl_error("commit_java: unable to find %s.class", name);
		return 0;
	}
	jmethodID cfunc = jnienv->GetStaticMethodID(cls, "commit", "(J)I");
	if(cfunc == NULL){
		gl_error("commit_java: unable to find int %s.commit(long)", name);
		return 0;
	}
	int rv = jnienv->CallStaticIntMethod(cls, cfunc, (int64)obj);
	return rv;
}

EXPORT TIMESTAMP sync_java(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass){
	JNIEnv *jnienv = (JNIEnv *)getvar("jnienv", NULL, NULL);
		jclass cls = jnienv->FindClass(obj->oclass->name);
	if(cls == NULL){
		gl_error("sync_java: unable to find %s.class", obj->oclass->name);
		return 0;
	}
	jmethodID cfunc = jnienv->GetStaticMethodID(cls, "sync", "(JJI)J");
	if(cfunc == NULL){
		gl_error("sync_java: unable to find long %s.sync(long, long, int)", obj->oclass->name);
		return 0;
	}
	int64 t1 = jnienv->CallStaticLongMethod(cls, cfunc, (int64)obj, t0, pass);
	if (pass==PC_POSTTOPDOWN) obj->clock = t0;
	return t1;
}

int notify_java(OBJECT *obj, NOTIFYMODULE msg){
	JNIEnv *jnienv = (JNIEnv *)getvar("jnienv", NULL, NULL);
	jclass cls = jnienv->FindClass(obj->oclass->name);
	if(cls == NULL)
	{
		gl_error("notify_java: unable to find %s.class", obj->oclass->name);
		return 0;
	}
	jmethodID cfunc = jnienv->GetStaticMethodID(cls, "notify", "(JLjava.lang.String;)I");
	if(cfunc == NULL)
	{
		/* acceptable omission */
		//gl_error("notify_java: unable to find int %s.notify(long, int)", obj->oclass->name);
		obj->oclass->notify = NULL;
		return 0;
	}
	return jnienv->CallStaticIntMethod(cls, cfunc, (int64)obj, (int)msg);
}

EXPORT int isa_java(OBJECT *obj, char *classname){
	JNIEnv *jnienv = (JNIEnv *)getvar("jnienv", NULL, NULL);
	jclass cls = jnienv->FindClass(obj->oclass->name);
	if(cls == NULL)
	{
		gl_error("isa_java: unable to find %s.class", obj->oclass->name);
		return 0;
	}
	jmethodID cfunc = jnienv->GetStaticMethodID(cls, "isa", "(JLjava.lang.String;)I");
	if(cfunc == NULL)
	{
		gl_error("isa_java: unable to find int %s.isa(long, String)", obj->oclass->name);
		return 0;
	}
	jstring isaname = jnienv->NewStringUTF(classname);
	int rv = jnienv->CallStaticIntMethod(cls, cfunc, (int64)obj, isaname);
	/* do we need to delete the string? */
	return rv;
}

EXPORT int64 plc_java(OBJECT *obj, TIMESTAMP t0){
	JNIEnv *jnienv = (JNIEnv *)getvar("jnienv", NULL, NULL);
	jclass cls = jnienv->FindClass(obj->oclass->name);
	if(cls == NULL){
		gl_error("plc_java: unable to find %s.class", obj->oclass->name);
		return 0;
	}
	jmethodID cfunc = jnienv->GetStaticMethodID(cls, "plc", "(JJ)J");
	if(cfunc == NULL){
		// reasonable omission
		//gl_error("plc_java: unable to find long %s.plc(long, long)", obj->oclass->name);
		obj->oclass->plc = NULL;
		return 0;
	}
	int64 rv = jnienv->CallStaticLongMethod(cls, cfunc, (int64)obj, t0);
	return rv;
}

EXPORT int recalc_java(OBJECT *obj){
	JNIEnv *jnienv = (JNIEnv *)getvar("jnienv", NULL, NULL);
	jclass cls = jnienv->FindClass(obj->oclass->name);
	if(cls == NULL){
		gl_error("recalc_java: unable to find %s.class", obj->oclass->name);
		return 0;
	}
	jmethodID cfunc = jnienv->GetStaticMethodID(cls, "recalc", "(J)V");
	if(cfunc == NULL){
		// reasonable omission
		//gl_error("recalc_java: unable to find void %s.recalc(long)", obj->oclass->name);
		obj->oclass->recalc = NULL;
		return 0;
	}
	int64 rv = jnienv->CallStaticLongMethod(cls, cfunc, (int64)obj);
	return 0;
}

CLASS *java_init(CALLBACKS *fntable, JAVACALLBACKS *jfntable, MODULE *module, char *modulename, int argc, char *argv[])
{
	JavaVM *jvm = NULL;
	JNIEnv *jnienv = NULL;
	if (!set_callback(fntable)) {
		errno = EINVAL;
		return NULL;
	}
	JAVACALLBACKS *jcallback = jfntable;
	if(jcallback == NULL){
		gl_error("%s:java_init() - unable to find jcallback", modulename);
		return NULL;
	}
	if(jvm == NULL)
		jvm = get_jvm();
	if(jnienv == NULL)
		jnienv = get_env();
	jstring *jargv = new jstring[argc];
	int i = 0;
	gl_output("javamod init entered\n");

	jclass cls = jnienv->FindClass(modulename);
	if(cls == NULL){
		gl_error("javamod:init.cpp: unable to find %s.class", modulename);
		return NULL;
	}

	jmethodID init_mid = jnienv->GetStaticMethodID(cls, "init", "(JLjava/lang/String;I[Ljava/lang/String;)J");

	if(init_mid == NULL){
		gl_error("javamod:init.cpp: unable to find \"int %s.init(long, string, int, string[])\"", modulename);
		return NULL;
	}

	jobjectArray args = jnienv->NewObjectArray(argc, jnienv->FindClass("[Ljava/lang/String;"), NULL);
	if(args == NULL){
		gl_error("javamod:init.cpp: unable to allocate args[] for %s.init()", modulename);
		return NULL;
	}
	
	for(i = 0; i < argc; ++i){
		jargv[i] = jnienv->NewStringUTF(argv[i]);
		jnienv->SetObjectArrayElement(args, i, jargv[i]);
	}

	jstring jmodname = jnienv->NewStringUTF(modulename);
	if(jmodname == NULL){
		gl_error("javamod:init.cpp: unable to allocate jmodname for %s.init()", modulename);
	}

	gl_output("javamod:init.cpp(): moduleaddr = %x", module);

	int64 rv = jnienv->CallStaticLongMethod(cls, init_mid, (int64)module, jmodname, argc, jargv);
	if (jnienv->ExceptionOccurred()) {
		jnienv->ExceptionDescribe();
	}
	
	// JNI cleanup
	jnienv->DeleteLocalRef(args);
	for(i = 0; i < argc; ++i)
		; /* delete the strings */

	gl_output("finished javamod init\n");

	return (CLASS *)rv;
}

/* EOF */
