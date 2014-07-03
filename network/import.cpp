/** $Id: import.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file import.cpp
	@addtogroup import Model import
	@ingroup network
	
	The only import format currently supported is the IEEE \p .cdf format 
	\htmlonly (<a href="http://www.google.com/search?hl=en&q=IEEE power flow file format CDF">find it</a>) \endhtmlonly 
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network.h"

int import_cdf(char *file)
{
	int n_bus=0;
	int n_branch=0;
	int linenum=0;
	int errors = 0; 
	OBJECT *bus[10000]; memset(bus,0,sizeof(bus));// CDF does not allow more than 9999 as a bus no
	OBJECT *branch[10000]; memset(branch,0,sizeof(branch));// CDF does not allow more than 9999 as a bus no
	// allow up to 100 zones
	OBJECT *swing[100]; memset(swing,0,sizeof(swing)); 
	int count[100]; memset(count,0,sizeof(count)); 
	char line[1024];
	char *ff;// = gl_findfile(file,NULL,FF_READ);
	FILE *fp;// = ff?fopen(ff, "r"):NULL;
	OBJECT *obj;
	enum {LD_INIT, LD_READY, LD_BUS, LD_BRANCH} state = LD_INIT;
	// model variables
	// branch variables
	char buffer[1024];
	ff = gl_findfile(file,NULL,FF_READ,buffer,sizeof(buffer)-1);
	fp = ff?fopen(ff, "r"):NULL;
	if (fp==NULL)
		return 0;
	while (fgets(line,sizeof(line),fp)!=NULL)
	{
		linenum++;
		switch (state) {
		case LD_INIT:
			if (sscanf(line+31,"%lg",&mvabase)==1)
				state=LD_READY;
			break;
		case LD_READY:
			if (mvabase>0 && strncmp(line,"BUS",3)==0)
				state = LD_BUS;
			else if (mvabase>0 && strncmp(line,"BRANCH",6)==0)
				state = LD_BRANCH;
			/* ignore line otherwise */
			break;
		case LD_BUS:
			if (atoi(line)>0)
			{	int n, area, zone, type;
				int64 remote;
				char32 name;
				double Vm, Va, Lr, Li, Gr, Gi, bV, dV, Qh, Ql, G, B;
				if (sscanf(line,"%d %12[^\n] %d %d%d %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %lg %" FMT_INT64 "d", 
					&n, name, &area, &zone, &type, &Vm, &Va, &Lr, &Li, &Gr, &Gi, &bV, &dV, &Qh, &Ql, &G, &B, &remote)==18 && n_bus<10000)
				{
					obj = gl_create_object(node_class);
					obj->parent = NULL;
					bus[n] = obj;
					node *p = OBJECTDATA(obj,node);
					p->create();
					obj->name = new char[strlen(name)+1];
					strcpy(obj->name,name);
					p->flow_area_num = area;
					p->loss_zone_num = zone;
					p->type = (BUSTYPE)type;
					p->V.SetPolar(Vm,Va*PI/180);
					p->S.SetRect((Lr-Gr)/mvabase,(Li-Gi)/mvabase); /* loads are positive, gen is negative */
					p->base_kV = bV;
					p->desired_kV = dV;
					p->Qmax_MVAR = Qh;
					p->Qmin_MVAR = Ql;
					p->G = G;
					p->B = B;
					/** @todo using remote_bus_id is just plain wrong, try using a lookup based on getting object node:bus_id **/
					p->remote_bus_id = (OBJECT*)remote;
					if (p->type==SWING)	swing[area]=obj;
					count[area]++;
					n_bus++;
#ifdef DEBUG_OBS
					if (p->type!=PQ) 
					{
						p->Vstdev = 0.01;
						p->Vobs.Re() = gl_random_normal(p->V.Re(),p->Vstdev);
						p->Vobs.Im() = gl_random_normal(p->V.Im(),p->Vstdev);
					}
					if (p->type!=SWING) 
						p->V = complex(1,0);
					else if (p->Vstdev>0)
						p->V.SetPolar(p->Vobs.Mag(),0);
#endif
				}
				else
				{
					gl_output("%s(%d) : missing bus data", file, linenum);
					errors++;
				}
			}
			else
				state = LD_READY;
			break;
		case LD_BRANCH:
			if (atoi(line)>0)
			{ 	int from, to, area, zone, type, circuit;
				double R, X, B, n=0;
				if (sscanf(line,"%d %d %d %d %d %d %lg %lg %lg %*lg %*lg %*lg %*d %*d %lg",
					&from,&to,&area,&zone,&type,&circuit,&R,&X,&B, &n)>=9 && n_branch<10000)
				{
					obj = gl_create_object(link_class);
					obj->parent = NULL;
					branch[n_branch] = obj;
					link *p = OBJECTDATA(obj,link);
					p->create();
					p->from = bus[from];
					p->to = bus[to];
					if (n>0.0)
						p->turns_ratio = n;
					else
						p->turns_ratio = 1.0;
					p->Y = complex(1)/complex(R,X); 
					p->B = B;
					n_branch++;
				}
				else
				{
					gl_output("%s(%d) : missing branch data", file, linenum);
					errors++;
				}
			}
			else
				state = LD_READY;
			break;
		default:
			break;
		}
	}
	fclose(fp);

	int n;
	// check area for swing buses
	for (n=0; n<sizeof(count)/sizeof(count[0]); n++)
	{
		if (count[n]>0 && swing[n]==NULL)
		{
			gl_output("%s : flow area %d has no swing bus", file, n);
			errors++;
		}
	}

	// fixup parent swing bus and remote bus, if any
	for (n=0; n<sizeof(bus)/sizeof(bus[0]); n++)
	{
		OBJECT *obj = bus[n];
		if (obj==NULL) continue;
		node *p = OBJECTDATA(obj,node);
		if (p->type!=SWING)
		{
			OBJECT *swingbus = swing[p->flow_area_num];
			if (swingbus!=NULL)
				gl_set_parent(obj,swingbus);
		}
		p->remote_bus_id= bus[(int64)p->remote_bus_id];
	}

	// branch parent is bus of lowest rank
	for (n=0; n<sizeof(branch)/sizeof(branch[0]); n++)
	{
		OBJECT *obj = branch[n];
		if (obj==NULL) continue;
		link *p = OBJECTDATA(obj,link);
		if (p->from!=NULL && p->to!=NULL)
			gl_set_parent(obj, p->from->rank<p->to->rank ? p->from : p->to);

		// update injections to node
		p->sync(TS_NEVER);
	}

	return errors>0 ? -errors : n_bus+n_branch;
}
EXPORT int import_file(char *file)
{
	char *ext = strrchr(file,'.');
	if (ext!=NULL && stricmp(ext,".cdf")==0)
		return import_cdf(file);
	errno = ENOENT;
	return 0;
}

/**@}*/
