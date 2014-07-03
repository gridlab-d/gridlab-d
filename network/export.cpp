///** $Id: export.cpp 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute
//	@file export.cpp
//	@addtogroup export Model export
//	@ingroup network
//	
// @{
// **/
//
//#include <stdlib.h>
//#include <stdio.h>
//#include <errno.h>
//#include <math.h>
//#include <float.h>
//#include "network.h"
//
//static int save_cdf(char *file)
//{
//	FILE *fp=fopen(file,"w");
//	if (fp==NULL) return 0;
//	time_t now = time(NULL);
//	char timestamp[32];
//	int count=0;
//	/* TODO: these should be set as module parameters */
//	double mvabase=100.0;
//	char system_name[32]="GridLAB-D Network";
//	
//	strftime(timestamp,sizeof(timestamp),"%m/%d/%y",gmtime(&now));
//
//	count += fprintf(fp," %s %-20.20s %5.1f  %4d %c %s\n", timestamp, system_name, mvabase, model_year, model_case, model_name);
//
//	// process nodes
//	FINDLIST *found = gl_find_objects(FL_NEW,FT_CLASS,SAME,"node",FT_END);
//	count += fprintf(fp,"%-38.38s %d ITEMS\n","BUS DATA FOLLOWS",found->hit_count);
//	OBJECT *obj=NULL;
//	while ((obj=gl_find_next(found,obj))!=NULL)
//	{
//		node *my = (node*)(obj+1);
//		char name[32];
//		sprintf(name,"Bus %d", obj->id+1);
//		count += fprintf(fp,"%4d %-12.12s %2d %2d %2d %5.3f %6.2f %7.1f %8.1f %7.1f %7.1f %6.1f %6.1f %6.1f %6.1f %8.5f %8.5f %d\n", obj->id+1,name,
//			my->flow_area_num, my->loss_zone_num, my->type, my->V.Mag(), 
//			my->V.Arg()*180/PI, 
//			my->S.Re()<0?-my->S.Re()*mvabase:0, my->S.Re()<0?-my->S.Im()*mvabase:0,
//			my->S.Re()>0?my->S.Re()*mvabase:0, my->S.Re()>0?my->S.Im()*mvabase:0,
//			my->base_kV, my->desired_kV, my->Qmax_MVAR, my->Qmin_MVAR, my->G, my->B, my->remote_bus_id?my->remote_bus_id->id+1:0);
//	}
//	count += fprintf(fp,"-999\n");
//	gl_free(found);
//
//	// process links
//	found = gl_find_objects(FL_NEW,FT_CLASS,SAME,"link",FT_END);
//	count += fprintf(fp,"%-38.38s %d ITEMS\n","BRANCH DATA FOLLOWS",found->hit_count);
//	obj=NULL;
//	while ((obj=gl_find_next(found,obj))!=NULL)
//	{
//		link *my = (link*)(obj+1);
//		node *from = my->from?(node*)(((OBJECT*)my->from)+1):NULL;
//		node *to= my->to?(node*)(((OBJECT*)my->to)+1):NULL;
//		if (from==NULL||to==NULL) /* ignore badly connected nodes */
//			continue;
//		double R = (complex(1)/my->Y).Re();
//		double X = (complex(1)/my->Y).Im();
//		count += fprintf(fp,"%4d %4d %2d %2d 1 0 %9.5f %11.5f %9.4f %4d %4d %4d %4d %1d  %6.3f %7.2f %5f %5f %5f %5f %5f\n", 
//			my->from->id+1, my->to->id+1, from->flow_area_num, from->loss_zone_num, R,X,0.0,
//			0,0,0,0, 0.0,0.0,0.0,0.0, 0.0,0.0,0.0,0.0);
//	}
//	count += fprintf(fp,"-999\n");
//	gl_free(found);
//
//	count += fprintf(fp,"LOSS ZONES FOLLOWS                    1 ITEMS\n  1 %s\n-99\n",model_name);
//	count += fprintf(fp,"INTERCHANGE DATA FOLLOWS              0 ITEMS\n-9\n");
//	count += fprintf(fp,"TIE LINES FOLLOWS                     0 ITEMS\n-999\n");
//
//	count += fprintf(fp,"END OF DATA\n");
//
//	return count;
//}
//
//EXPORT int export_file(char *file)
//{
//	if (file==NULL) file="network.cdf";
//
//	char *ext = strrchr(file,'.');
//	if (ext!=NULL && stricmp(ext,".cdf")==0)
//		return save_cdf(file);
//	errno = ENOENT;
//	return 0;
//}
//
//EXPORT int kmldump(FILE *fp, OBJECT *obj)
//{
//	if (obj==NULL) /* dump document styles */
//	{
//		/* line styles */
//		fprintf(fp,"    <Style id=\"TransmissionLine\">\n");
//		fprintf(fp,"	  <LineStyle>\n"
//				"        <color>7f00ffff</color>\n"
//				"        <width>4</width>\n"
//				"      </LineStyle>\n"
//				"      <PolyStyle>\n"
//				"        <color>7f00ff00</color>\n"
//				"      </PolyStyle>\n"
//				"    </Style>\n");
//	}
//	else if (strcmp(obj->oclass->name,"node")==0)
//	{
//		node *pNode = (node*)OBJECTDATA(obj,node);
//		if (isnan(obj->latitude) || isnan(obj->longitude))
//			return 0;
//		fprintf(fp,"    <Placemark>\n");
//		if (obj->name)
//			fprintf(fp,"      <name>%s</name>\n", obj->name);
//		else
//			fprintf(fp,"      <name>%s %d</name>\n", obj->oclass->name, obj->id);
//		fprintf(fp,"      <description>\n");
//		fprintf(fp,"        <![CDATA[\n");
//		fprintf(fp,"          <TABLE><TR>\n");
//		switch (pNode->type) {
//		case SWING:
//			fprintf(fp,"            <TR><TH ALIGN=LEFT>Type</TH><TD ALIGN=RIGHT STYLE=\"font-family:courier;\">SWING&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</TD></TR>\n");
//			break;			
//		case PV:
//			fprintf(fp,"            <TR><TH ALIGN=LEFT>Type</TH><TD ALIGN=RIGHT STYLE=\"font-family:courier;\">PV&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</TD></TR>\n");
//			break;			
//		case PQ:
//			fprintf(fp,"            <TR><TH ALIGN=LEFT>Type</TH><TD ALIGN=RIGHT STYLE=\"font-family:courier;\">PQ&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</TD></TR>\n");
//			break;			
//		}
//		fprintf(fp,"            <TR><TH ALIGN=LEFT>Voltage</TH><TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;kV&nbsp;&nbsp;<BR>%.3f&nbsp;&ang;&nbsp;&nbsp;&nbsp;</TD></TR>\n",
//			pNode->V.Mag()*pNode->base_kV,pNode->V.Arg()*180/3.1416);
//		if (pNode->S.Mag()>0)
//			fprintf(fp,"            <TR><TH ALIGN=LEFT>%s</TH><TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;MW&nbsp;&nbsp;<BR>%.3f&nbsp;MVAR</TD></TR>\n",
//				pNode->S.Re()<0?"Demand":"Supply",fabs(pNode->S.Re())*mvabase,pNode->S.Im()*mvabase);
//		fprintf(fp,"          </TR></TABLE>\n");
//		fprintf(fp,"        ]]>\n");
//		fprintf(fp,"      </description>\n");
//		fprintf(fp,"      <Point>\n");
//		fprintf(fp,"        <coordinates>%f,%f</coordinates>\n",obj->longitude,obj->latitude);
//		fprintf(fp,"      </Point>\n");
//		fprintf(fp,"    </Placemark>\n");
//	}
//	else if (strcmp(obj->oclass->name,"link")==0)
//	{
//		link *pLink = (link*)OBJECTDATA(obj,link);
//		OBJECT *from = pLink->from;
//		OBJECT *to = pLink->to;
//		if (isnan(from->latitude) || isnan(to->latitude) || isnan(from->longitude) || isnan(to->longitude))
//			return 0;
//		fprintf(fp,"    <Placemark>\n");
//		if (obj->name)
//			fprintf(fp,"      <name>%s</name>\n", obj->name);
//		else
//			fprintf(fp,"      <name>%s ==> %s</name>\n", from->name?from->name:"unnamed", to->name?to->name:"unnamed");
//			fprintf(fp,"      <description>\n");
//			fprintf(fp,"        <![CDATA[\n");
//			fprintf(fp,"          <TABLE><TR>\n");
//			complex flow;
//			node *pFrom = OBJECTDATA(from,node);
//			node *pTo = OBJECTDATA(to,node);
//			if (pLink->I.Re()<0)
//				flow = pFrom->V * pLink->I;
//			else
//				flow = pTo->V * pLink->I;
//			complex loss = pLink->I*pLink->I / pLink->Y;
//			complex imp = complex(1,0)/pLink->Y;
//			double current_base = mvabase/pFrom->base_kV*1000;
//			fprintf(fp,"            <TR><TH ALIGN=LEFT>Flow</TH><TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;&nbsp;MW&nbsp;&nbsp;<BR>%.3f&nbsp;&nbsp;MVAR</TD></TR>\n",
//				flow.Re()*mvabase, flow.Im()*mvabase);
//			fprintf(fp,"            <TR><TH ALIGN=LEFT>Current</TH><TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.3f&nbsp;&nbsp;Amps</TD></TR>\n",
//				pLink->I.Mag()*current_base);
//			fprintf(fp,"            <TR><TH ALIGN=LEFT>Impedance</TH><TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.4f&nbsp;R&nbsp;&nbsp;&nbsp;<BR>%.4f&nbsp;X&nbsp;&nbsp;&nbsp;</TD></TR>\n",
//				imp.Re(),imp.Im());
//			fprintf(fp,"            <TR><TH ALIGN=LEFT>Losses</TH><TD ALIGN=RIGHT STYLE=\"font-family:courier;\">%.2f&nbsp;&nbsp;&nbsp;%%P&nbsp;&nbsp;<BR>%.2f&nbsp;&nbsp;&nbsp;%%Q&nbsp;&nbsp;</TD></TR>\n",
//				loss.Re()/flow.Re()*100,loss.Im()/flow.Im()*100);
//			fprintf(fp,"          </TR></TABLE>\n");
//			fprintf(fp,"        ]]>\n");
//			fprintf(fp,"      </description>\n");
//		fprintf(fp,"      <styleUrl>#TransmissionLine</styleUrl>>\n");
//		fprintf(fp,"      <coordinates>%f,%f</coordinates>\n",
//			(from->longitude+to->longitude)/2,(from->latitude+to->latitude)/2);
//		fprintf(fp,"      <LineString>\n");			
//		fprintf(fp,"        <extrude>0</extrude>\n");
//		fprintf(fp,"        <tessellate>0</tessellate>\n");
//		fprintf(fp,"        <altitudeMode>relative</altitudeMode>\n");
//		fprintf(fp,"        <coordinates>%f,%f,50 %f,%f,50</coordinates>\n",
//			from->longitude,from->latitude,to->longitude,to->latitude);
//		fprintf(fp,"      </LineString>\n");			
//		fprintf(fp,"    </Placemark>\n");
//	}
//	return 0;
//}
//
///**@}*/
