/* @file linkage.c
 */

#include "instance.h"
#include "output.h"
#include "object.h"
#include "property.h"

/** linkage_create_writer
    Add a master->slave linkage to an instance object.
	@returns 1 on success, 0 on failure
 **/
int linkage_create_writer(instance *inst, char *fromobj, char *fromvar, char *toobj, char *tovar)
{
	/* allocate linkage */
	linkage *lnk = (linkage*)malloc(sizeof(linkage));
	if ( !lnk )
	{
		output_error("unable to allocate memory for linkage %s:%s -> %s:%s", fromobj, fromvar, toobj, tovar);
		return 0;
	}
	memset(lnk,0,sizeof(linkage));
	lnk->type = LT_MASTERTOSLAVE;

	/* copy local info */
	strcpy(lnk->local.obj,fromobj);
	strcpy(lnk->local.prop,fromvar);

	/* copy remote info */
	strcpy(lnk->remote.obj,toobj);
	strcpy(lnk->remote.prop,tovar);

	/* attach to instance cache */
	if ( !instance_add_linkage(inst, lnk) )
	{
		output_error("unable to attach linkage %s:%s -> %s:%s to instance %s", lnk->local.obj, lnk->local.prop, lnk->remote.obj, lnk->remote.prop, inst->model);
		return 0;
	}
	else
	{
		output_verbose("created write linkage from local %s:%s to remote %s:%s", fromobj, fromvar, toobj, tovar);
		return 1;
	}
}

/** linkage_create_reader
    Add a master<-slave linkage to an instance object.
	@returns 1 on success, 0 on failure
 **/
int linkage_create_reader(instance *inst, char *fromobj, char *fromvar, char *toobj, char *tovar)
{
	/* allocate linkage */
	linkage *lnk = (linkage*)malloc(sizeof(linkage));
	if ( !lnk )
	{
		output_error("unable to allocate memory for linkage %s:%s <- %s:%s", fromobj, fromvar, toobj, tovar);
		return 0;
	}
	memset(lnk,0,sizeof(linkage));
	lnk->type = LT_SLAVETOMASTER;

	/* copy local info */
	strcpy(lnk->local.obj,toobj);
	strcpy(lnk->local.prop,tovar);

	/* copy remote info */
	strcpy(lnk->remote.obj,fromobj);
	strcpy(lnk->remote.prop,fromvar);

	/* attach to instance cache */
	if ( !instance_add_linkage(inst, lnk) )
	{
		output_error("unable to attach linkage %s:%s <- %s:%s to instance %s", lnk->local.obj, lnk->local.prop, lnk->remote.obj, lnk->remote.prop, inst->model);
		return 0;
	}
	else
	{
		output_verbose("created read linkage from local %s:%s to remote %s:%s", fromobj, fromvar, toobj, tovar);
		return 1;
	}
}

/** linkage_master_to_slave
    Updates the instance cache for a master->slave linkage.
	@returns 1 on success, 0 on failure
 **/
STATUS linkage_master_to_slave(char *buffer, linkage *lnk)
{
	int rv = 0;
	int size = 0;

	//output_debug("linkage_master_to_slave");

	// null checks
	if(0 == lnk){
		output_error("linkage_master_to_slave has null lnk pointer");
		return FAILED;
	}
	if(0 == lnk->target.obj){
		output_error("linkage_master_to_slave has null lnk->target.obj pointer");
		return FAILED;
	}
	size = (int)property_minimum_buffersize(lnk->target.prop);
	switch ( global_multirun_mode ) {
		case MRM_MASTER:
//			rv = class_property_to_string(lnk->target.prop,GETADDR(lnk->target.obj,lnk->target.prop),(char *)((int64)lnk->addr),size);
			rv = object_get_value_by_addr(lnk->target.obj, GETADDR(lnk->target.obj,lnk->target.prop), (char*)((int64)lnk->addr), size, lnk->target.prop);
			output_debug("prop %s, addr %x, addr2 %x, val %s", lnk->target.prop->name, GETADDR(lnk->target.obj,lnk->target.prop), (char *)((int64)lnk->addr), lnk->addr);
			break;
		case MRM_SLAVE:
//			rv = class_string_to_property(lnk->target.prop,GETADDR(lnk->target.obj,lnk->target.prop),(char *)((int64)lnk->addr));
			rv = object_set_value_by_addr(lnk->target.obj, GETADDR(lnk->target.obj,lnk->target.prop), (char *)((int64)lnk->addr), lnk->target.prop);
			output_debug("prop %s, addr %x, addr2 %x, val %s", lnk->target.prop->name, GETADDR(lnk->target.obj,lnk->target.prop), (char *)((int64)lnk->addr), lnk->addr);
			break;
		default:
			break;
	}
	if(0 == rv){
		output_error("linkage_master_to_slave failed for link %s.%s", lnk->target.obj->name, lnk->target.prop->name);
		output_debug("str=%8s", (char *)((int64)lnk->addr));
		return FAILED;
	}
	return SUCCESS;
}

/** linkage_slave_to_master
    Updates the instance cache for a master<-slave linkage.
	@returns 1 on success, 0 on failure
 **/
STATUS linkage_slave_to_master(char *buffer, linkage *lnk)
{
	int rv = 0;
	int size = (int)property_minimum_buffersize(lnk->target.prop);

	// null checks
	if(0 == lnk){
		output_error("linkage_master_to_slave has null lnk pointer");
		return FAILED;
	}
	if(0 == lnk->target.obj){
		output_error("linkage_master_to_slave has null lnk->target.obj pointer");
		return FAILED;
	}

	switch ( global_multirun_mode ) {
	case MRM_MASTER:
//		rv = class_string_to_property(lnk->target.prop,GETADDR(lnk->target.obj,lnk->target.prop),(char *)((int64)lnk->addr));
		rv = object_set_value_by_addr(lnk->target.obj, GETADDR(lnk->target.obj,lnk->target.prop), (char *)((int64)lnk->addr), lnk->target.prop);
		output_debug("prop %s, addr %x, addr2 %x, val %s", lnk->target.prop->name, GETADDR(lnk->target.obj,lnk->target.prop), (char *)((int64)lnk->addr), lnk->addr);
		break;
	case MRM_SLAVE:
//		rv = class_property_to_string(lnk->target.prop,GETADDR(lnk->target.obj,lnk->target.prop),(char *)((int64)lnk->addr),size);
		rv = object_get_value_by_addr(lnk->target.obj, GETADDR(lnk->target.obj,lnk->target.prop), (char*)((int64)lnk->addr), size, lnk->target.prop);
		output_debug("prop %s, addr %x, addr2 %x, val %s", lnk->target.prop->name, GETADDR(lnk->target.obj,lnk->target.prop), (char *)((int64)lnk->addr), lnk->addr);
		break;
	default:
		break;
	}
	if(0 == rv){
		output_error("linkage_slave_to_master failed for link %s.%s", lnk->target.obj->name, lnk->target.prop->name);
		output_debug("str=%8s", (char *)((int64)lnk->addr));
		return FAILED;
	}
	return SUCCESS;
}

/** linkage_init
	Initialize a linkage
	@returns SUCCESS or FAILED
 **/
STATUS linkage_init(instance *inst, linkage *lnk)
{
	/* find local object */
	lnk->target.obj = object_find_name(lnk->local.obj);
	if ( !lnk->target.obj )
	{
		output_error("unable to find linkage source object '%s'", lnk->local.obj);
		return FAILED;
	}

	/* find local property */
	lnk->target.prop = object_get_property(lnk->target.obj,lnk->local.prop,NULL);
	if ( !lnk->target.prop )
	{
		output_error("unable to find linkage source property '%s' in object '%s'", lnk->local.prop, lnk->local.obj);
		return FAILED;
	}

	/* calculate buffer size */
	lnk->prop_size = property_minimum_buffersize(lnk->target.prop);
	lnk->name_size = strlen(lnk->remote.obj) + strlen(lnk->remote.prop) + 2;
	lnk->size = lnk->name_size + lnk->prop_size;

	output_verbose("initialized linkage between local %s:%s and remote %s:%s", lnk->local.obj, lnk->local.prop, lnk->remote.obj, lnk->remote.prop);
	return SUCCESS;
}
