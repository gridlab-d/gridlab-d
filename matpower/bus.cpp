/** $Id: bus.cpp 4738 2014-07-03 00:55:39Z dchassin $
	@file bus.cpp
	@defgroup bus Template for a new object class
	@ingroup MODULENAME

	You can add an object class to a module using the \e add_class
	command:
	<verbatim>
	linux% add_class CLASS
	</verbatim>

	You must be in the module directory to do this.

	author: Kyle Anderson (GridSpice Project),  kyle.anderson@stanford.edu
	Released in Open Source to Gridlab-D Project

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
//#include "solver_matpower.h"
#include "bus.h"
#include "gen.h"
#include "branch.h"
#include "baseMVA.h"
#include "areas.h"
//#include "gen_cost.h"
#include "matpower.h"
//#include "lock.h"
#include <map>
#include <iostream>
#include <string>

using namespace std;

#define TIME_INTERVAL 900

CLASS *bus::oclass = NULL;
bus *bus::defaults = NULL;

// Global Variable
multimap<string,unsigned int>bus_map;
vector<unsigned int> bus_BUS_I;
vector<unsigned int> branch_F_BUS;
vector<unsigned int> branch_T_BUS;
vector<unsigned int> gen_GEN_BUS;
vector<unsigned int> gen_NCOST;

//Sanity Check Flag
bool IF_REFERENCE = false;
unsigned int BUS_NUM = 0;
unsigned int BUS_NUM_GEN = 0;
unsigned int BUS_NUM_BRANCH = 0;
unsigned int GEN_NUM = 0;
unsigned int BRANCH_NUM = 0;



#ifdef OPTIONAL
/* TODO: define this to allow the use of derived classes */
CLASS *PARENTbus::pclass = NULL;
#endif

/* TODO: remove passes that aren't needed */
static PASSCONFIG passconfig = PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN;

/* TODO: specify which pass the clock advances */
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
bus::bus(MODULE *module)
#ifdef OPTIONAL
/* TODO: include this if you are deriving this from a superclass */
: SUPERCLASS(module)
#endif
{
#ifdef OPTIONAL
	/* TODO: include this if you are deriving this from a superclass */
	pclass = SUPERCLASS::oclass;
#endif



	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"bus",sizeof(bus),passconfig);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);
                
                // attributes of bus class. the names follow the MATPOWER Bus Data structure
		if (gl_publish_variable(oclass,
		   	//PT_int16, "BUS_I", PADDR(BUS_I), PT_DESCRIPTION, "bus number",
			PT_enumeration, "BUS_TYPE", PADDR(BUS_TYPE), PT_DESCRIPTION, "bus type",
				PT_KEYWORD, "PQ", 1,
				PT_KEYWORD, "PV", 2,
				PT_KEYWORD, "REF", 3,
				PT_KEYWORD, "ISOLATE", 4,
                        PT_double, "PD[MW]", PADDR(PD), PT_DESCRIPTION, "real power demand",
                        PT_double, "QD[MVAr]", PADDR(QD), PT_DESCRIPTION, "reactive power demand",
                        PT_double, "GS[MW]", PADDR(GS), PT_DESCRIPTION, "shunt conductance",
                        PT_double, "BS[MVAr]", PADDR(BS), PT_DESCRIPTION, "shunt susceptance",
                        //PT_int16, "BUS_AREA", PADDR(BUS_AREA), PT_DESCRIPTION, "area number",
                        PT_double, "VM", PADDR(VM), PT_DESCRIPTION, "voltage magnitude",
                        PT_double, "VA", PADDR(VA), PT_DESCRIPTION, "voltage angle",
                        PT_double, "BASE_KV[kV]", PADDR(BASE_KV), PT_DESCRIPTION, "base voltage",
                        PT_int16, "ZONE", PADDR(ZONE), PT_DESCRIPTION, "lose zone",
                        PT_double, "VMAX", PADDR(VMAX), PT_DESCRIPTION, "maximum voltage magnitude",
                        PT_double, "VMIN", PADDR(VMIN), PT_DESCRIPTION, "minimum voltage magnitude",
			PT_int16, "BASE_MVA", PADDR(BASE_MVA), PT_DESCRIPTION, "base MVA",
			PT_double, "LAM_P", PADDR(LAM_P), PT_DESCRIPTION, "Lagrange multiplier on real power mismatch (mu/MW)",
			PT_double, "LAM_Q", PADDR(LAM_Q), PT_DESCRIPTION, "Lagrange multiplier on reactive power mistmatch (mu/MW)",
			PT_double, "MU_VMAX", PADDR(MU_VMAX), PT_DESCRIPTION, "Kuhn-Tucker multiplier on upper voltage limit (mu/p.u.)",
			PT_double, "MU_VMIN", PADDR(MU_VMIN), PT_DESCRIPTION, "Kuhn-Tucker multiplier on lower voltage limit (mu/p.u.)",
			PT_enumeration, "IFHEADER", PADDR(ifheader), PT_DESCRIPTION, "if connected with distribution network",
				PT_KEYWORD, "Yes", 1,
				PT_KEYWORD, "No", 0,
	
			PT_complex,"CVoltageA",PADDR(CVoltageA),PT_DESCRIPTION, "complex voltage phase A of distribution network: Cvoltage.Mag() = VM, Cvoltage.Arg() = VA;",
			PT_complex,"CVoltageB",PADDR(CVoltageB),PT_DESCRIPTION, "complex voltage phase B of distribution network: Cvoltage.Mag() = VM, Cvoltage.Arg() = VA;",
			PT_complex,"CVoltageC",PADDR(CVoltageC),PT_DESCRIPTION, "complex voltage phase C of distribution network: Cvoltage.Mag() = VM, Cvoltage.Arg() = VA;",
			PT_double,"V_nom",PADDR(V_nom), PT_DESCRIPTION, "Normalized voltage of distribution network",
			PT_complex,"feeder0", PADDR(feeder0), PT_DESCRIPTION, "Feeder 0 Complex power",
			PT_complex,"feeder1", PADDR(feeder1), PT_DESCRIPTION, "Feeder 1 Complex power",
			PT_complex,"feeder2", PADDR(feeder2), PT_DESCRIPTION, "Feeder 2 Complex power",
			PT_complex,"feeder3", PADDR(feeder3), PT_DESCRIPTION, "Feeder 3 Complex power",
			PT_complex,"feeder4", PADDR(feeder4), PT_DESCRIPTION, "Feeder 4 Complex power",
			PT_complex,"feeder5", PADDR(feeder5), PT_DESCRIPTION, "Feeder 5 Complex power",
			PT_complex,"feeder6", PADDR(feeder6), PT_DESCRIPTION, "Feeder 6 Complex power",
			PT_complex,"feeder7", PADDR(feeder7), PT_DESCRIPTION, "Feeder 7 Complex power",
			PT_complex,"feeder8", PADDR(feeder8), PT_DESCRIPTION, "Feeder 8 Complex power",
			PT_complex,"feeder9", PADDR(feeder9), PT_DESCRIPTION, "Feeder 9 Complex power",
		     /* TODO: add your published properties here */
		    NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		memset(this,0,sizeof(bus));
		/* TODO: set the default values of all properties here */
		ifheader = 0;
		CVoltageA = 0;
		CVoltageB = 0;
		CVoltageC = 0;
		V_nom = 0;
		ZONE = 1;
		
	}
}

/* Object creation is called once for each object that is created by the core */
int bus::create(void)
{
	memcpy(this,defaults,sizeof(bus));
	/* TODO: set the context-free initial value of properties, such as random distributions */
	
	OBJECT *obj = OBJECTHDR(this);
	bus *tempbus;

	tempbus = OBJECTDATA(obj,bus);

	setObjectValue_Double2Complex(obj,"feeder0",0,0);
	setObjectValue_Double2Complex(obj,"feeder1",0,0);
	setObjectValue_Double2Complex(obj,"feeder2",0,0);
	setObjectValue_Double2Complex(obj,"feeder3",0,0);
	setObjectValue_Double2Complex(obj,"feeder4",0,0);
	setObjectValue_Double2Complex(obj,"feeder5",0,0);
	setObjectValue_Double2Complex(obj,"feeder6",0,0);
	setObjectValue_Double2Complex(obj,"feeder7",0,0);
	setObjectValue_Double2Complex(obj,"feeder8",0,0);
	setObjectValue_Double2Complex(obj,"feeder9",0,0);

	
	

	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int bus::init(OBJECT *parent)
{
	/* TODO: set the context-dependent initial value of properties */
	OBJECT *obj_this = OBJECTHDR(this);
	bus *bus_this;
	bus_this = OBJECTDATA(obj_this,bus);

	// Sanity Check -- number of branch bus/gen bus
	BUS_NUM += 1;
	if (bus_this->BUS_TYPE == 2 || bus_this->BUS_TYPE == 3)
		BUS_NUM_GEN += 1;
	if (bus_this->BUS_TYPE == 1)
		BUS_NUM_BRANCH += 1;


	if (bus_this->BUS_TYPE == 3)
	{
		// Create baseMVA object
		//printf("Arrive tihs point\n");
		/*
		OBJECT *obj_baseMVA;
		obj_baseMVA = gl_create_object(baseMVA::oclass);
		//printf("Arrive tihs point2\n");
		if (obj_baseMVA == NULL)
			gl_error("Create baseMVA object failed\n");
		else
		{
			OBJECTDATA(obj_baseMVA,baseMVA)->create();
			//printf("Arrive tihs point3\n");
			baseMVA *baseMVA_obj;
			baseMVA_obj = OBJECTDATA(obj_baseMVA,baseMVA);
			setObjectValue_Double(obj_baseMVA,"BASEMVA",bus_this->BASE_MVA);
			//printf("this is bus %d with base MVA %d\n",bus_this->BUS_I, bus_this->BASE_MVA);
		}

		// Create Areas object
		OBJECT *obj_areas;
		obj_areas = gl_create_object(areas::oclass);
		if (obj_areas == NULL)
			gl_error("Create Areas object failed\n");
		else
		{
			OBJECTDATA(obj_areas,areas)->create();
			setObjectValue_Double(obj_areas,"AREA",1);
			setObjectValue_Double(obj_areas,"REFBUS",1);
		}
		*/

		
		// Create map between bus name and bus id		

		multimap<string,unsigned int>::iterator it;
		unsigned int bus_index = 0;
		OBJECT *obj_map = NULL;
		bus *bus_obj_map;
		FINDLIST *bus_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"bus",FT_END);
		while (gl_find_next(bus_list,obj_map) != NULL)
		{
			bus_index++;
			obj_map = gl_find_next(bus_list,obj_map);
			bus_obj_map = OBJECTDATA(obj_map,bus);
			bus_map.insert(pair<string,unsigned int>(obj_map->name,bus_index));
			//setObjectValue_Double(obj_map,"BUS_I",bus_index);
			bus_BUS_I.push_back(bus_index);
		}

		//Update bus index in branch
		OBJECT *obj_branch = NULL;
		branch *branch_obj;
		FINDLIST *branch_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"branch",FT_END);
		multimap<string,unsigned int>::iterator find_res;
		
		while (gl_find_next(branch_list,obj_branch) != NULL)
		{
			obj_branch = gl_find_next(branch_list,obj_branch);
			branch_obj = OBJECTDATA(obj_branch,branch);
			
			string F_bus_name(branch_obj->from);
			find_res = bus_map.find(F_bus_name);
			if (find_res == bus_map.end())
			{
			  //gl_error("Cannot find from bus %s in branch", F_bus_name);
				exit(1);
			}
			//setObjectValue_Double(obj_branch,"F_BUS",(*find_res).second);
			branch_F_BUS.push_back((*find_res).second);

			string T_bus_name(branch_obj->to);
			find_res = bus_map.find(T_bus_name);
			if (find_res == bus_map.end())
			{
			  //gl_error("Cannot find to bus %s in branch", T_bus_name);
				exit(1);
			}
			//setObjectValue_Double(obj_branch,"T_BUS",(*find_res).second);
			branch_T_BUS.push_back((*find_res).second);

			//Sanity Check -- count branch number
			//BRANCH_NUM += 1;
		}

		//Update bus index in gen
		OBJECT *obj_gen = NULL;
		gen *gen_obj;
		FINDLIST *gen_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"gen",FT_END);
		bus *gen_parent_bus;
		multimap<string,unsigned int>::iterator find_gen;
		while (gl_find_next(gen_list,obj_gen) != NULL)
		{
					
	
			obj_gen = gl_find_next(gen_list,obj_gen);
			gen_obj = OBJECTDATA(obj_gen,gen);
				
			OBJECT *gen_parent = object_parent(obj_gen);
			gen_parent_bus = OBJECTDATA(gen_parent,bus);
			
			//setObjectValue_Double(obj_gen,"GEN_BUS",gen_parent_bus->BUS_I);

			string Gen_parent_name(gen_parent->name);
			find_gen = bus_map.find(Gen_parent_name);
			if (find_gen == bus_map.end())
			{
			  //gl_error("Cannot find bus %s at generator %s",Gen_parent_name,obj_gen->name);
				exit(1);
			}
			gen_GEN_BUS.push_back((*find_gen).second);

			// add NCOST
			string double_string(gen_obj->COST);
			vector<string> v;
			v = split(double_string,',');
			//setObjectValue_Double(obj_gen,"NCOST",v.size());
			gen_NCOST.push_back(v.size());

			//Sanity Check -- count gen number
			GEN_NUM += 1;
		}
		
		// Sanity Check -- if reference bus is defined
		IF_REFERENCE ^= true;
	}


	

	return 1; /* return 1 on success, 0 on failure */
}

/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP bus::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement pre-topdown behavior */
	OBJECT *obj = OBJECTHDR(this);
	bus *tempbus;

	tempbus = OBJECTDATA(obj,bus);


	//Sanity Check
	if (IF_REFERENCE != true)
	{
		gl_error("No defined reference bus or more than one reference bus\n");
		exit(1);
	}
	if (BUS_NUM_GEN != GEN_NUM)
	{
		gl_error("There are %d generator bus and %d generator. Not equal!\n",BUS_NUM_GEN,GEN_NUM);
		exit(1);
	}
	//if (BUS_NUM_BRANCH != BRANCH_NUM)
	//{
	//	gl_error("There are %d branch bus and %d branch. Not equal!\n",BUS_NUM_BRANCH,BRANCH_NUM);
	//	exit(1);
	//}
	


	if (tempbus->ifheader == 1)
	{
		LOCK_OBJECT(obj);		
		double sum_PD, sum_QD;


		sum_PD = tempbus->feeder0.Re() + tempbus->feeder1.Re() + tempbus->feeder2.Re() + tempbus->feeder3.Re() + tempbus->feeder4.Re() + tempbus->feeder5.Re() + tempbus->feeder6.Re() + tempbus->feeder7.Re() + tempbus->feeder8.Re() + tempbus->feeder9.Re();

		sum_QD = tempbus->feeder0.Im() + tempbus->feeder1.Im() + tempbus->feeder2.Im() + tempbus->feeder3.Im() + tempbus->feeder4.Im() + tempbus->feeder5.Im() + tempbus->feeder6.Im() + tempbus->feeder7.Im() + tempbus->feeder8.Im() + tempbus->feeder9.Im();

		UNLOCK_OBJECT(obj);

		setObjectValue_Double2Complex(obj,"feeder0",0,0);
		setObjectValue_Double2Complex(obj,"feeder1",0,0);
		setObjectValue_Double2Complex(obj,"feeder2",0,0);
		setObjectValue_Double2Complex(obj,"feeder3",0,0);
		setObjectValue_Double2Complex(obj,"feeder4",0,0);
		setObjectValue_Double2Complex(obj,"feeder5",0,0);
		setObjectValue_Double2Complex(obj,"feeder6",0,0);
		setObjectValue_Double2Complex(obj,"feeder7",0,0);
		setObjectValue_Double2Complex(obj,"feeder8",0,0);
		setObjectValue_Double2Complex(obj,"feeder9",0,0);
		

		//tempbus->PD = sum_PD;
		//tempbus->QD = sum_QD;

		// Unit of Power in MATPOWER is MW and MVAr
		setObjectValue_Double(obj,"PD",sum_PD/1000000);
		setObjectValue_Double(obj,"QD",sum_QD/1000000);

		
		
		//cout<<"sum_PD"<<sum_PD<<"   "<<tempbus->PD<<endl;
		//cout<<"sum_QD"<<sum_QD<<"   "<<tempbus->QD<<endl;

	}

	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP bus::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	//TIMESTAMP t2 = TS_NEVER;
	TIMESTAMP t2 = t1+TIME_INTERVAL;
	OBJECT *obj = OBJECTHDR(this);
	bus *tempbus;
	
	tempbus = OBJECTDATA(obj,bus);

	if (tempbus->BUS_TYPE == 3) //ref bus
	{	
		//Run OPF solver
		if (solver_matpower(bus_BUS_I,branch_F_BUS,branch_T_BUS,gen_GEN_BUS,gen_NCOST,tempbus->BASE_MVA) == 1)
		{
			gl_error("OPF Failed");
			exit(1);
		}
	
	}
	

	/* TODO: implement bottom-up behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}	

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP bus::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement post-topdown behavior */
	//OBJECT *obj = OBJECTHDR(this);
	//bus *tempbus;
	
	//tempbus = OBJECTDATA(obj,bus);
	//unsigned int bus_num = tempbus->BUS_I;
	//printf("Bus %d, Real power %f, reactive power %f\n",tempbus->BUS_I,tempbus->LAM_P,tempbus->LAM_Q);
/*	if (tempbus->BUS_TYPE == 3)
	{
		OBJECT *temp_obj = NULL;
		bus *iter_bus;
		FINDLIST *bus_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"bus",FT_END);
		printf("After simulation\n");
		while (gl_find_next(bus_list,temp_obj)!=NULL)
		{
			temp_obj = gl_find_next(bus_list,temp_obj);
			iter_bus = OBJECTDATA(temp_obj,bus);
			printf("Bus %d; PD %f; QD %f; VM %f; VA %f;\n",iter_bus->BUS_I,iter_bus->PD,iter_bus->QD,iter_bus->VM,iter_bus->VA);
		};
	}
*/
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_bus(OBJECT **obj)
{
	try
	{
		*obj = gl_create_object(bus::oclass);
		if (*obj!=NULL)
			return OBJECTDATA(*obj,bus)->create();
	}
	catch (char *msg)
	{
		gl_error("create_bus: %s", msg);
	}
	return 0;
}

EXPORT int init_bus(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,bus)->init(parent);
	}
	catch (char *msg)
	{
		gl_error("init_bus(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return 0;
}

EXPORT TIMESTAMP sync_bus(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	bus *my = OBJECTDATA(obj,bus);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock,t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;
		return t2;
	}
	catch (char *msg)
	{
		gl_error("sync_bus(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return TS_INVALID;
}
