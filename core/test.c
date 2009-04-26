/* test.c
 * Copyright (C) 2008 Battelle Memorial Institute
 * Integrated system testing routines.  Start is called before executive runs, end after executive exits.
 */

#include <stdlib.h>

#include "globals.h"
#include "cmdarg.h"
#include "module.h"
#include "output.h"
#include "save.h"
#include "find.h"
#include "test.h"
#include "aggregate.h"

#ifndef _NO_CPPUNIT
#include "test_callbacks.h"

#ifndef WIN32
#include <dlfcn.h>
#endif


TIMESTAMP get_global_clock(void)
{
	return global_clock;
}

STATUS t_setup_ranks(void);
STATUS t_sync_all(PASSCONFIG pass);

STATUS test_init(void)
{
	OBJECT *obj;
	output_verbose("initializing objects...");
	for (obj=object_get_first(); obj!=NULL; obj=object_get_next(obj))
	{
		if (object_init(obj)==FAILED)
		{
			output_error("object %s initialization failed", object_name(obj));
			return FAILED;
		}
	}
	return SUCCESS;
}

STATUS do_sync_all(PASSCONFIG pass)
{
	return t_sync_all(pass);
}

static TEST_CALLBACKS callbacks = {
	//printStuff,
	class_get_class_from_classname,
	get_global_clock,
	object_sync,
	do_sync_all,
	test_init,
	t_setup_ranks,
	remove_objects
};

STATUS test_start(int argc, char *argv[])
{
	int mod_test_num = 1;
	char mod_test[100];
	char *mod_name;
	int test_result = 0;
	sprintf(mod_test,"mod_test%d",mod_test_num++);
	mod_name = global_getvar(mod_test, NULL, 0);
	module_load("tape",argc,argv);
	while(mod_name != NULL)
	{
		MODULE *mod = module_load(mod_name,argc,argv);
		
		if(mod == NULL)
		{
			output_fatal("Invalid module name");
			/*	TROUBLESHOOT
				The test_start procedure was given an invalid module name.
				Check the command line argument and/or the unit test sequence
				to be sure the test is requested properly.
			 */
			return FAILED;
		}

		if (mod->module_test==NULL)
		{
			output_fatal("Module %s does not implement cppunit test", mod->name);
			/*	TROUBLESHOOT
				The test_start procedure was given the name of a module that doesn't
				implement unit testing.
				Check the command line argument and/or the test configuration file
				to be sure the test is requested properly.				
			 */
			return FAILED;
		}

		test_result = mod->module_test(&callbacks,argc,argv);
		
		if(test_result == 0)
			return FAILED;
		sprintf(mod_test,"mod_test%d",mod_test_num++);
		mod_name = global_getvar(mod_test, NULL, 0);
	}
	
	return SUCCESS;

}
STATUS original_test_start(int argc, char *argv[])
{
	OBJECT *obj[6];
	MODULE *network;
	CLASS *node, *link;
	MODULE *tape;
	CLASS *player, *recorder, *collector;

	network = module_load("network",argc,argv);
	if (network==NULL)
	{
#ifndef WIN32
		fprintf(stderr,"%s\n",dlerror());
#else
		perror("network module load failed");
#endif
		return FAILED;
	}
	output_verbose("network module loaded ok");

	node = class_get_class_from_classname("node");
	if (node==NULL)
	{
		output_fatal("network module does not implement class node");
		/*	TROUBLESHOOT
			The <b>network</b> module test can't find the <b>node</b>
			class definition.  This is probably caused by either
			an internal system error or a version of the network module
			that doesn't implement node object as expected (or at all).
		 */
		return FAILED;
	}
	output_verbose("class node implementation loaded ok");

	link = class_get_class_from_classname("link");
	if (node==NULL || link==NULL)
	{
		output_fatal("network module does not implement class link");
		/*	TROUBLESHOOT
			The <b>network</b> module test can't find the <b>link</b>
			class definition.  This is probably caused by either
			an internal system error or a version of the network module
			that doesn't implement link object as expected (or at all).
		 */
		return FAILED;
	}
	output_verbose("class link implementation loaded ok");

	tape = module_load("tape",argc,argv);
	if (tape==NULL)
	{
#ifndef WIN32
		fprintf(stderr,"%s\n",dlerror());
#else
		perror("tape module load failed");
#endif
		return FAILED;
	}

	player = class_get_class_from_classname("player");
	if (player==NULL)
	{
		output_fatal("tape module does not implement class player");
		/*	TROUBLESHOOT
			The <b>tape</b> module test can't find the <b>player</b>
			class definition.  This is probably caused by either
			an internal system error or a version of the tape module
			that doesn't implement player object as expected (or at all).
		 */
		return FAILED;
	}
	recorder = class_get_class_from_classname("recorder");
	if (recorder==NULL)
	{
		output_fatal("tape module does not implement class recorder");
		/*	TROUBLESHOOT
			The <b>tape</b> module test can't find the <b>recorder</b>
			class definition.  This is probably caused by either
			an internal system error or a version of the tape module
			that doesn't implement recorder object as expected (or at all).
		 */
		return FAILED;
	}
	collector = class_get_class_from_classname("collector");
	if (collector==NULL)
	{
		output_fatal("tape module does not implement class collector");
		/*	TROUBLESHOOT
			The <b>tape</b> module test can't find the <b>collector</b>
			class definition.  This is probably caused by either
			an internal system error or a version of the tape module
			that doesn't implement collector object as expected (or at all).
		 */
		return FAILED;
	}

	if (module_import(network,"../test/pnnl2bus.cdf")<=0)
		return FAILED;

	/* tape player */
	if ((*player->create)(&obj[3],object_get_first())==FAILED)
	{
		output_fatal("player creation failed");
		/*	TROUBLESHOOT
			The <b>tape</b> module test can't create a <b>player</b>
			object.  This is probably caused by either
			an internal system error or a version of the tape module
			that doesn't implement player object as expected (or at all).
		 */
		return FAILED;
	}
	object_set_value_by_name(obj[3],"loop","3600"); /* 18000 is about 12y at 1h steps */
	object_set_value_by_name(obj[3],"property","S");

	/* tape recorder */
	if ((*recorder->create)(&obj[4],object_get_first())==FAILED)
	{
		output_fatal("recorder creation failed");
		/*	TROUBLESHOOT
			The <b>tape</b> module test can't create a <b>recorder</b>
			object.  This is probably caused by either
			an internal system error or a version of the tape module
			that doesn't implement recorder object as expected (or at all).
		 */
		return FAILED;
	}
	object_set_value_by_name(obj[4],"property","V,S");
	object_set_value_by_name(obj[4],"interval","0");
	object_set_value_by_name(obj[4],"limit","1000");

	/* tape collector */
	if ((*collector->create)(&obj[5],NULL)==FAILED)
	{
		output_fatal("collector creation failed");
		/*	TROUBLESHOOT
			The <b>tape</b> module test can't create a <b>collector</b>
			object.  This is probably caused by either
			an internal system error or a version of the tape module
			that doesn't implement collector object as expected (or at all).
		 */
		return FAILED;
	}
	object_set_value_by_name(obj[5],"property","count(V.mag),min(V.mag),avg(V.mag),std(V.mag),max(V.mag),min(V.ang),avg(V.ang),std(V.ang),max(V.ang)");
	object_set_value_by_name(obj[5],"interval","0");
	object_set_value_by_name(obj[5],"limit","1000");
	object_set_value_by_name(obj[5],"group","class=node;");

	module_check(network);

	return SUCCESS;
}



STATUS test_end(int argc, char *argv[])
{
	return SUCCESS;
}

STATUS original_test_end(int argc, char *argv[])
{
	FINDLIST *find = find_objects(FL_GROUP,"class=node;");
	OBJECT *obj;
	AGGREGATION *aggr;
	char *exp = "class=node";
	char *agg = "max(V.ang)";

	for (obj=find_first(find); obj!=NULL; obj=find_next(find,obj))
		output_message("object %s found", object_name(obj));
	free(find);

	output_message("Aggregation of %s over %s...", agg,exp);
	aggr = aggregate_mkgroup(agg,exp);
	if (aggr)
		output_message("Result is %lf", aggregate_value(aggr));
	else
		output_message("Aggregation failed!");

	if (!saveall("-"))
		perror("save failed");

	return SUCCESS;
}

#endif
