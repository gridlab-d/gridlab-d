#include "serialize.h"
#include "class.h"
#include "object.h"
#include <cstring>

nlohmann::json serialize_objeventdetails(const OBJEVENTDETAILS_LOCAL *details)
{
	if (details == nullptr || details->obj_of_int == nullptr)
		return nullptr;
	nlohmann::json j;
	if (details->obj_of_int)
	{
		if (details->obj_of_int->name && strlen(details->obj_of_int->name) > 0)
			j["obj_of_int"] = details->obj_of_int->name;
		else
			j["obj_of_int"] = (int64_t)details->obj_of_int->id;
	}
	else
		j["obj_of_int"] = nullptr;

	if (details->obj_made_int)
	{
		if (details->obj_made_int->name && strlen(details->obj_made_int->name) > 0)
			j["obj_made_int"] = details->obj_made_int->name;
		else
			j["obj_made_int"] = (int64_t)details->obj_made_int->id;
	}
	else
		j["obj_made_int"] = nullptr;

	j["fail_time"] = (int64_t)details->fail_time;
	j["fail_time_ns"] = details->fail_time_ns;
	j["fail_time_dbl"] = details->fail_time_dbl;
	j["rest_time"] = (int64_t)details->rest_time;
	j["rest_time_ns"] = details->rest_time_ns;
	j["rest_time_dbl"] = details->rest_time_dbl;
	j["fail_length"] = (int64_t)details->fail_length;
	j["fail_length_ns"] = details->fail_length_ns;
	j["fail_length_dbl"] = details->fail_length_dbl;
	j["rest_length"] = (int64_t)details->rest_length;
	j["rest_length_ns"] = details->rest_length_ns;
	j["rest_length_dbl"] = details->rest_length_dbl;
	j["in_fault"] = details->in_fault;
	j["implemented_fault"] = details->implemented_fault;
	j["customers_affected"] = details->customers_affected;
	j["customers_affected_sec"] = details->customers_affected_sec;
	return j;
}

nlohmann::json serialize_indexarray(INDEXARRAY_LOCAL *index)
{
	if (index == nullptr)
		return nullptr;
	nlohmann::json j;
	j["MetricName"] = std::string((char *)index->MetricName);

	if (index->MetricLoc && index->MetricLoc->get_property())
	{
		j["MetricLoc"] = index->MetricLoc->get_property()->name;
		char buf[1024];
		if (class_property_to_string(index->MetricLoc->get_property(), index->MetricLoc->get_addr(), buf, sizeof(buf)) > 0)
			j["MetricLocValue"] = buf;
	}
	if (index->MetricLocInterval && index->MetricLocInterval->get_property())
	{
		j["MetricLocInterval"] = index->MetricLocInterval->get_property()->name;
		char buf[1024];
		if (class_property_to_string(index->MetricLocInterval->get_property(), index->MetricLocInterval->get_addr(), buf, sizeof(buf)) > 0)
			j["MetricLocIntervalValue"] = buf;
	}
	return j;
}

nlohmann::json serialize_custarray(CUSTARRAY_LOCAL *cust)
{
	if (cust == nullptr || cust->CustomerObj == nullptr)
		return nullptr;
	nlohmann::json j;
	if (cust->CustomerObj->name && strlen(cust->CustomerObj->name) > 0)
		j["CustomerObj"] = cust->CustomerObj->name;
	else
		j["CustomerObj"] = (int64_t)cust->CustomerObj->id;

	if (cust->CustInterrupted && cust->CustInterrupted->get_property())
	{
		j["CustInterrupted"] = cust->CustInterrupted->get_property()->name;
		char buf[1024];
		if (class_property_to_string(cust->CustInterrupted->get_property(), cust->CustInterrupted->get_addr(), buf, sizeof(buf)) > 0)
			j["CustInterruptedValue"] = buf;
	}
	if (cust->CustInterrupted_Secondary && cust->CustInterrupted_Secondary->get_property())
	{
		j["CustInterrupted_Secondary"] = cust->CustInterrupted_Secondary->get_property()->name;
		char buf[1024];
		if (class_property_to_string(cust->CustInterrupted_Secondary->get_property(), cust->CustInterrupted_Secondary->get_addr(), buf, sizeof(buf)) > 0)
			j["CustInterrupted_SecondaryValue"] = buf;
	}
	return j;
}

int get_int_property(OBJECT *obj, const char *name)
{
	PROPERTY *p = class_find_property(obj->oclass, name);
	if (p == nullptr)
		return 0;
	void *addr = (char *)obj + (ptrdiff_t)p->addr;
	if (p->ptype == PT_int32)
		return *static_cast<int32 *>(addr);
	if (p->ptype == PT_int16)
		return *static_cast<int16 *>(addr);
	if (p->ptype == PT_int64)
		return (int)*static_cast<int64 *>(addr);
	return 0;
}
