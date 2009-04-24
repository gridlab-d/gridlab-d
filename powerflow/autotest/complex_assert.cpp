/* automatically generated from GridLAB-D */

int major=0, minor=0;

#include "rt/gridlabd.h"

int __declspec(dllexport) dllinit() { return 0;};
int __declspec(dllexport) dllkill() { return 0;};
CALLBACKS *callback = NULL;
static CLASS *myclass = NULL;
static void setup_class(CLASS *);

extern "C" CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	callback=fntable;
	myclass=(CLASS*)((*(callback->class_getname))("complex_assert"));
	setup_class(myclass);
	return myclass;}
#line 3 "assert.glm"
class complex_assert {
public:
	complex_assert(MODULE*mod) {};
#line 3 "assert.glm"
#line 4 "assert.glm"
	char32 target;
#line 27 "complex_assert.cpp"
#line 5 "assert.glm"
	complex value;
#line 30 "complex_assert.cpp"
#line 6 "assert.glm"
	double within;
#line 33 "complex_assert.cpp"
#line 7 "assert.glm"
	TIMESTAMP postsync (TIMESTAMP t0, TIMESTAMP t1) { OBJECT*my=((OBJECT*)this)-1;
	try {
		if (t0>0)
		{
			complex x;
			gl_get_value(my->parent,target, x);
			complex m = (x-value).Mag();
			if (!m.IsFinite() || m>within){
				
				gl_error("assert failed on %s: %s (%g%+gi) not within %f of %g%+gi", 
					gl_name(my->parent), target, x.Re(), x.Im(), within, value.Re(), value.Im());
				return t1;
				}
			return TS_NEVER;
		} else {
			return t1+1;
		}
	} catch (char *msg) {callback->output_error("%s[%s:%d] exception - %s",my->name?my->name:"(unnamed)",my->oclass->name,my->id,msg); return 0;} catch (...) {callback->output_error("%s[%s:%d] unhandled exception",my->name?my->name:"(unnamed)",my->oclass->name,my->id); return 0;} callback->output_error("complex_assert::postsync(TIMESTAMP t0, TIMESTAMP t1) not all paths return a value"); return 0;}
#line 53 "complex_assert.cpp"
};
#line 55 "complex_assert.cpp"
#line 56 "complex_assert.cpp"
extern "C" int64 create_complex_assert(OBJECT **obj, OBJECT *parent)
{
	if ((*obj=gl_create_object(myclass))==NULL)
		return 0;
	memset((*obj)+1,0,sizeof(complex_assert));
	gl_set_parent(*obj,parent);
	return 1;
}
#line 65 "complex_assert.cpp"
extern "C" int64 sync_complex_assert(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	int64 t2 = TS_NEVER;
	switch (pass) {
	case PC_POSTTOPDOWN:
		t2=((complex_assert*)(obj+1))->postsync(obj->clock,t1);
		obj->clock = t2;
		break;
	default:
		break;
	}
	return t2;
}
static void setup_class(CLASS *oclass)
{	
	OBJECT obj; obj.oclass = oclass; complex_assert *t = (complex_assert*)(&obj)+1;
	oclass->size = sizeof(complex_assert);
	(*(callback->properties.get_property))(&obj,"target")->addr = (PROPERTYADDR)((char*)&(t->target) - (char*)t);
	(*(callback->properties.get_property))(&obj,"value")->addr = (PROPERTYADDR)((char*)&(t->value) - (char*)t);
	(*(callback->properties.get_property))(&obj,"within")->addr = (PROPERTYADDR)((char*)&(t->within) - (char*)t);

}
