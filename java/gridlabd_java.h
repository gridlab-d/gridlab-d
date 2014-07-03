/** $Id: gridlabd_java.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file gridlabd_java.h
	@author Matthew L. Hauer
	@addtogroup java_module_api Java module API
	@brief The Java GridLAB-D module go-between header file

	The Java gobo module provides a set of JNI callbacks for Java classes to access the
	GridLAB-D core with native calls.  Java modules will need to import gridlabd.* and contain
	at least two classes, one for the module, which will need to extend or wrap GModule, and
	one for each constituant class, each of which will need to extend or wrap GObject.

	Most of the familiar gl_* function calls and constants in C/C++ modules can be made
	from Java using	GridlabD.*, such as GridlabD.error() and GridlabD.TS_NEVER.  Class
	registration is made through static calls to GClass, and Gridlab object construction
	through static GObject methods.

	Java modules must be careful to handle any exceptions internally, as uncaught exceptions may
	have unexpected consequences within the JVM.

 **/

#include <jni.h>
#include "gridlabd.h"

#ifndef _GRIDLABD_JAVA_H_
#define _GRIDLABD_JAVA_H_

typedef struct s_javacallbacks {
	JavaVM *(*get_jvm)();
	JNIEnv *(*get_env)();
	void *(*set_obj)(jobject, OBJECT *, void *, int);
} JAVACALLBACKS;

CDECL JavaVM *get_jvm();
CDECL JNIEnv *get_env();
CDECL void *set_obj(jobject obj, OBJECT * object_addr, void *block_addr, int id);
EXPORT void *getvar(char *varname, char *value, unsigned int size);

char *get_classpath();
char *get_libpath();
void *get_jcb();

#define jgl_get_jvm (*jcallback->get_jvm)
#define jgl_get_env (*jcallback->get_env)
#define jgl_set_obj (*jcallback->set_obj)

#ifdef JDLMAIN
#define EXTERN
#define INIT(X) =(X)
#else
#ifdef __cplusplus
#define EXTERN
#else
#define EXTERN extern
#endif /* __cplusplus */
#define INIT(X)
#endif
CDECL EXPORT EXTERN JAVACALLBACKS *jcallback INIT(NULL);
#undef INIT
#undef EXTERN

#define EXPORT_JAVA_CREATE(name) EXPORT_JAVA_CREATE_EX(name, name)
#define EXPORT_JAVA_CREATE_EX(N, C)											\
																			\
CLASS *N##_class=NULL;														\
OBJECT *last_##N=NULL;														\
																			\
EXPORT int create_##N(OBJECT **obj, OBJECT *parent){						\
	JNIEnv *jnienv = jgl_get_env();											\
	jclass cls = jnienv->FindClass(#N);										\
	if(cls == NULL){														\
		gl_error("create_%s: unable to find %s.class", #N, #N);				\
		return 0;															\
	}																		\
	jmethodID cfunc = jnienv->GetStaticMethodID(cls, "create", "(J)J");		\
	if(cfunc == NULL){														\
		gl_error("create_%s: unable to find long %s.create(long)", #N, #N);	\
		return 0;															\
	}																		\
	int64 rv = jnienv->CallStaticLongMethod(cls, cfunc, (int64)parent);		\
	if(rv == 0){															\
		gl_error("create_%s: %s.create() failed", #N, #N);					\
		GL_THROW("%s.create() failed", #N);									\
	}																		\
	if (jnienv->ExceptionOccurred()) {										\
		jnienv->ExceptionDescribe();										\
	}																		\
	*obj = (OBJECT *)rv;													\
	last_##N = *obj;														\
	gl_set_parent(*obj, parent);											\
	return 1;																\
}																			\

#define EXPORT_JAVA_SYNC(name) EXPORT_JAVA_SYNC_EX(name,name)
#define EXPORT_JAVA_SYNC_EX(name, classname)											\
EXPORT TIMESTAMP sync_##name(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass){				\
	JNIEnv *jnienv = jgl_get_env();														\
	jclass cls = jnienv->FindClass(#name);												\
	if(cls == NULL){																	\
		gl_error("sync_%s: unable to find %s.class", #name, #name);						\
		return 0;																		\
	}																					\
	jmethodID cfunc = jnienv->GetStaticMethodID(cls, "sync", "(JJI)J");					\
	if(cfunc == NULL){																	\
		gl_error("sync_%s: unable to find long %s.sync(long, long, int)", #name, #name);\
		return 0;																		\
	}																					\
	int64 t1 = jnienv->CallStaticLongMethod(cls, cfunc, (int64)obj, t0, pass);			\
	if (pass==PC_POSTTOPDOWN) obj->clock = t0;											\
	return t1;																			\
}

#define EXPORT_JAVA_INIT(name)														\
EXPORT int init_##name(OBJECT *obj, OBJECT *parent){								\
	JNIEnv *jnienv = jgl_get_env();													\
	jclass cls = jnienv->FindClass(#name);											\
	if(cls == NULL){																\
		gl_error("init_%s: unable to find %s.class", #name, #name);					\
		return 0;																	\
	}																				\
	jmethodID cfunc = jnienv->GetStaticMethodID(cls, "init", "(JJ)I");				\
	if(cfunc == NULL){																\
		gl_error("init_%s: unable to find int %s.init(long, long)", #name, #name);	\
		return 0;																	\
	}																				\
	int rv = jnienv->CallStaticIntMethod(cls, cfunc, (int64)obj, (int64)parent);	\
	return rv;																		\
}

#define EXPORT_JAVA_PLC(name)														\
EXPORT int64 plc_##name(OBJECT *obj, TIMESTAMP t0){									\
	JNIEnv *jnienv = jgl_get_env();													\
	jclass cls = jnienv->FindClass(#name);											\
	if(cls == NULL){																\
		gl_error("plc_%s: unable to find %s.class", #name, #name);					\
		return 0;																	\
	}																				\
	jmethodID cfunc = jnienv->GetStaticMethodID(cls, "plc", "(JJ)J");				\
	if(cfunc == NULL){																\
		gl_error("init_%s: unable to find long %s.plc(long, long)", #name, #name);	\
		return 0;																	\
	}																				\
	int64 rv = jnienv->CallStaticLongMethod(cls, cfunc, (int64)obj, t0);			\
	return rv;																		\
}

EXPORT jint JNICALL Java_GridlabD_verbose(JNIEnv *env, jclass _this, jstring str);
EXPORT jint JNICALL Java_GridlabD_output(JNIEnv *, jclass, jstring);
EXPORT jint JNICALL Java_GridlabD_warning(JNIEnv *env, jobject _this, jstring str);
EXPORT jint JNICALL Java_GridlabD_error(JNIEnv *env, jobject _this, jstring str);
EXPORT jint JNICALL Java_GridlabD_debug(JNIEnv *env, jobject _this, jstring str);
EXPORT jint JNICALL Java_GridlabD_testmsg(JNIEnv *env, jobject _this, jstring str);
EXPORT jlong JNICALL Java_GridlabD_get_1module_1var(JNIEnv *env, jobject _this, jstring modulename, jstring varname);
EXPORT jstring JNICALL Java_GridlabD_findfile(JNIEnv *env, jobject _this, jstring filename);
EXPORT jlong JNICALL Java_GridlabD_find_1module(JNIEnv *env, jobject _this, jstring modulename);
EXPORT jlong JNICALL Java_GridlabD_register_1class(JNIEnv *env, jobject _this, jlong moduleaddr, jstring classname, jint passconfig);
EXPORT jlong JNICALL Java_GridlabD_create_1object(JNIEnv *env, jobject _this, jlong oclass_addr, jlong size);
EXPORT jlong JNICALL Java_GridlabD_create_1array(JNIEnv *env, jobject _this, jlong oclass_addr, jlong size, jint n_objects);
EXPORT jint JNICALL Java_GridlabD_object_1isa(JNIEnv *env, jobject _this, jlong obj_addr, jstring _typename);
EXPORT jlong JNICALL Java_GridlabD_publish_1variable(JNIEnv *env, jobject _this, jlong moduleaddr, jlong classaddr, jstring varname, jstring vartype, jlong offset);
EXPORT jlong JNICALL Java_GridlabD_publish_1function(JNIEnv *env, jobject _this, jlong moduleaddr, jlong classaddr, jstring funcname);
EXPORT jint JNICALL Java_GridlabD_set_1dependent(JNIEnv *env, jobject _this, jlong from_addr, jlong to_addr);
EXPORT jint JNICALL Java_GridlabD_set_1parent(JNIEnv *env, jobject _this, jlong child_addr, jlong parent_addr);
EXPORT jint JNICALL Java_GridlabD_register_1type(JNIEnv *env, jobject _this, jlong oclass_addr, jstring _typename, jstring from_string_fname, jstring to_string_fname);
EXPORT jint JNICALL Java_GridlabD_publish_1delegate(JNIEnv *env, jobject _this);
EXPORT jlong JNICALL Java_GridlabD_get_1property(JNIEnv *env, jobject _this, jlong oaddr, jstring propertyname);
EXPORT jstring JNICALL Java_GridlabD_get_1value(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname);
EXPORT jint JNICALL Java_GridlabD_set_1value(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname, jstring value);
EXPORT jdouble JNICALL Java_GridlabD_unit_1convert(JNIEnv *env, jobject _this, jstring from, jstring to, jdouble invalue);
EXPORT jlong JNICALL Java_GridlabD_get_1object(JNIEnv *env, jobject _this, jstring oname);
EXPORT jint JNICALL Java_GridlabD_get_1int16(JNIEnv *env, jobject _this, jlong object_addr, jlong property_addr);
EXPORT jint JNICALL Java_GridlabD_get_1int16_1by_1name(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname);
EXPORT jint JNICALL Java_GridlabD_get_1int32(JNIEnv *env, jobject _this, jlong object_addr, jlong property_addr);
EXPORT jint JNICALL Java_GridlabD_get_1int32_1by_1name(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname);
EXPORT jlong JNICALL Java_GridlabD_get_1int64(JNIEnv *env, jobject _this, jlong object_addr, jlong property_addr);
EXPORT jlong JNICALL Java_GridlabD_get_1int64_1by_1name(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname);
EXPORT jdouble JNICALL Java_GridlabD_get_1double(JNIEnv *env, jobject _this, jlong object_addr, jlong property_addr);
EXPORT jdouble JNICALL Java_GridlabD_get_1double_1by_1name(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname);
EXPORT jstring JNICALL Java_GridlabD_get_1string(JNIEnv *env, jobject _this, jlong object_addr, jlong property_addr);
EXPORT jstring JNICALL Java_GridlabD_get_1string_1by_1name(JNIEnv *env, jobject _this, jlong object_addr, jstring propertyname);
EXPORT jlong JNICALL Java_GridlabD_create_1aggregate(JNIEnv *env, jobject _this, jstring aggregator, jstring group_expression);
EXPORT jdouble JNICALL Java_GridlabD_run_1aggregate(JNIEnv *env, jobject _this, jlong aggregate_addr);
EXPORT jdouble JNICALL Java_GridlabD_random_1unifrom(JNIEnv *env, jobject _this, jdouble a, jdouble b);
EXPORT jdouble JNICALL Java_GridlabD_random_1normal(JNIEnv *env, jobject _this, jdouble m, jdouble s);
EXPORT jdouble JNICALL Java_GridlabD_random_1lognormal(JNIEnv *env, jobject _this, jdouble m, jdouble s);
EXPORT jdouble JNICALL Java_GridlabD_random_1bernoulli(JNIEnv *env, jobject _this, jdouble p);
EXPORT jdouble JNICALL Java_GridlabD_random_1pareto(JNIEnv *env, jobject _this, jdouble m, jdouble s);
EXPORT jdouble JNICALL Java_GridlabD_random_1sampled(JNIEnv *env, jobject _this, jint n, jdoubleArray x);
EXPORT jdouble JNICALL Java_GridlabD_random_1exponential(JNIEnv *env, jobject _this, jdouble l);
EXPORT jlong JNICALL Java_GridlabD_parsetime(JNIEnv *env, jobject _this, jstring value);
EXPORT jdouble JNICALL Java_GridlabD_todays(JNIEnv *env, jobject _this, jlong t);
EXPORT jdouble JNICALL Java_GridlabD_tohours(JNIEnv *env, jobject _this, jlong t);
EXPORT jdouble JNICALL Java_GridlabD_tominutes(JNIEnv *env, jobject _this, jlong t);
EXPORT jlong JNICALL Java_GridlabD_global_1create(JNIEnv *env, jobject _this, jstring name, jstring args);
EXPORT jint JNICALL Java_GridlabD_global_1setvar(JNIEnv *env, jobject _this, jstring def, jstring args);
EXPORT jstring JNICALL Java_GridlabD_global_1getvar(JNIEnv *env, jobject _this, jstring name);
EXPORT jlong JNICALL Java_GridlabD_global_1find(JNIEnv *env, jobject _this, jstring name);

#endif

/* EOF */
