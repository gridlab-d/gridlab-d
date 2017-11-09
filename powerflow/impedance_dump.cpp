// $Id: impedance_dump.cpp 1182 2008-12-22 22:08:36Z dchassin $
/**	Copyright (C) 2008 Battelle Memorial Institute

	@file impedance_dump.cpp

	@{
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "impedance_dump.h"

//////////////////////////////////////////////////////////////////////////
// impedance_dump CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

CLASS* impedance_dump::oclass = NULL;

impedance_dump::impedance_dump(MODULE *mod)
{
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"impedance_dump",sizeof(impedance_dump),PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_char32,"group",PADDR(group),PT_DESCRIPTION,"the group ID to output data for (all links if empty)",
			PT_char256,"filename",PADDR(filename),PT_DESCRIPTION,"the file to dump the current data into",
			PT_timestamp,"runtime",PADDR(runtime),PT_DESCRIPTION,"the time to check voltage data",
			PT_int32,"runcount",PADDR(runcount),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the number of times the file has been written to",
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

int impedance_dump::create(void)
{
	group.erase();
	runcount = 0;
	return 1;
}

int impedance_dump::init(OBJECT *parent)
{
	if(filename[0] == '\0'){
		gl_error("No filename was specified. Unable to open file for righting.");
		return 0;
		/* TROUBLESHOOT
		No file name was specifed. Please assign a file name to record the line/link impedance values in the impedance_dump object. 
		*/
	}

	return 1;
}

int impedance_dump::isa(char *classname)
{
	return strcmp(classname,"impedance_dump")==0;
}

complex * impedance_dump::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}

int impedance_dump::dump(TIMESTAMP t)
{
	FINDLIST *capacitors, *fuses, *ohlines, *reclosers, *regulators, *relays, *sectionalizers, *series_reactors, *switches, *transformers, *tplines, *uglines;
	OBJECT *obj = NULL;
	char buffer[1024];
	FILE *fn = NULL;
	int index = 0;
	int count = 0;
	int x = 0;
	int y = 0;
	CLASS *obj_class;
	char timestr[64];
	PROPERTY *xfmrconfig;

	//find the link objects
	if(group[0] == '\0'){
		fuses = gl_find_objects(FL_NEW,FT_CLASS,SAME,"fuse",FT_END);						//find all fuses
		ohlines = gl_find_objects(FL_NEW,FT_CLASS,SAME,"overhead_line",FT_END);				//find all overhead_lines
		reclosers = gl_find_objects(FL_NEW,FT_CLASS,SAME,"recloser",FT_END);				//find all reclosers
		regulators = gl_find_objects(FL_NEW,FT_CLASS,SAME,"regulator",FT_END);				//find all regulators
		relays = gl_find_objects(FL_NEW,FT_CLASS,SAME,"relay",FT_END);						//find all relays
		sectionalizers = gl_find_objects(FL_NEW,FT_CLASS,SAME,"sectionalizer",FT_END);		//find all sectionalizers
		series_reactors = gl_find_objects(FL_NEW,FT_CLASS,SAME,"series_reactor",FT_END);	//find all series_reactors
		switches = gl_find_objects(FL_NEW,FT_CLASS,SAME,"switch",FT_END);					//find all switches
		transformers = gl_find_objects(FL_NEW,FT_CLASS,SAME,"transformer",FT_END);			//find all transformers
		tplines = gl_find_objects(FL_NEW,FT_CLASS,SAME,"triplex_line",FT_END);				//find all triplex_lines
		uglines = gl_find_objects(FL_NEW,FT_CLASS,SAME,"underground_line",FT_END);			//find all underground_lines
		capacitors = gl_find_objects(FL_NEW,FT_CLASS,SAME,"capacitor",FT_END);
	} else {
		fuses = gl_find_objects(FL_NEW,FT_CLASS,SAME,"fuse",AND,FT_GROUPID,SAME,group.get_string(),FT_END);
		ohlines = gl_find_objects(FL_NEW,FT_CLASS,SAME,"overhead_line",AND,FT_GROUPID,SAME,group.get_string(),FT_END);
		reclosers = gl_find_objects(FL_NEW,FT_CLASS,SAME,"recloser",AND,FT_GROUPID,SAME,group.get_string(),FT_END);
		regulators = gl_find_objects(FL_NEW,FT_CLASS,SAME,"regulator",AND,FT_GROUPID,SAME,group.get_string(),FT_END);
		relays = gl_find_objects(FL_NEW,FT_CLASS,SAME,"relay",AND,FT_GROUPID,SAME,group.get_string(),FT_END);
		sectionalizers = gl_find_objects(FL_NEW,FT_CLASS,SAME,"sectionalizer",AND,FT_GROUPID,SAME,group.get_string(),FT_END);
		series_reactors = gl_find_objects(FL_NEW,FT_CLASS,SAME,"series_reactor",AND,FT_GROUPID,SAME,group.get_string(),FT_END);
		switches = gl_find_objects(FL_NEW,FT_CLASS,SAME,"switch",AND,FT_GROUPID,SAME,group.get_string(),FT_END);
		transformers = gl_find_objects(FL_NEW,FT_CLASS,SAME,"transformer",AND,FT_GROUPID,SAME,group.get_string(),FT_END);
		tplines = gl_find_objects(FL_NEW,FT_CLASS,SAME,"triplex_line",AND,FT_GROUPID,SAME,group.get_string(),FT_END);
		uglines = gl_find_objects(FL_NEW,FT_CLASS,SAME,"underground_line",AND,FT_GROUPID,SAME,group.get_string(),FT_END);
		capacitors = gl_find_objects(FL_NEW,FT_CLASS,SAME,"capacitor",AND,FT_GROUPID,SAME,group.get_string(),FT_END);
	}
	if(fuses == NULL && ohlines == NULL && reclosers == NULL && regulators == NULL && relays == NULL && sectionalizers == NULL && series_reactors == NULL && switches == NULL && transformers == NULL && tplines == NULL && uglines == NULL){
		gl_error("No link objects were found.");
		return 0;
		/* TROUBLESHOOT
		No link objects were found in your .glm file. if you specified a group id double check to make sure you have link objects with the specified group id. 
		*/
	}
	
	//open file for writing
	fn = fopen(filename,"w");
	if(fn == NULL){
		gl_error("Unable to open %s for writing.",(char *)(&filename));
		return 0;
	}

	//write style sheet info
	fprintf(fn,"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
	fprintf(fn,"<?xml-stylesheet href=\"C:\\Projects\\GridLAB-D_Builds\\ticket_704\\VS2005\\gridlabd-2_0.xsl\" type=\"text/xsl\"?>\n");
	fprintf(fn,"<gridlabd>\n");
	//write time
	gl_printtime(t, timestr, 64);
	fprintf(fn,"\t<Time>%s</Time>\n",timestr);
	//write fuses
	if(fuses != NULL){
		pFuse = (link_object **)gl_malloc(fuses->hit_count*sizeof(link_object*));
		if(pFuse == NULL){
			gl_error("Failed to allocate fuse array.");
			return TS_NEVER;
		}
		while(obj = gl_find_next(fuses,obj)){
			if(index >= fuses->hit_count){
				break;
			}
			pFuse[index] = OBJECTDATA(obj,link_object);
			if(pFuse[index] == NULL){
				gl_error("Unable to map object as a link object.");
				return 0;
			}

			//write the fuse
			fprintf(fn,"\t<fuse>\n");

			//write the name
			if(obj->name[0] != 0){
				fprintf(fn,"\t\t<name>%s</name>\n",obj->name);
			} else {
				fprintf(fn,"\t\t<name>NA</name>\n");
			}

			//write the id
			fprintf(fn,"\t\t<id>fuse:%d</id>\n",obj->id);

			//write the from name
			if(pFuse[index]->from->name[0] != 0){
				fprintf(fn,"\t\t<from_node>%s:%s</from_node>\n",pFuse[index]->from->oclass->name,pFuse[index]->from->name);
			} else {
				fprintf(fn,"\t\t<from_node>%s:%d</from_node>\n",pFuse[index]->from->oclass->name,pFuse[index]->from->id);
			}

			//write the to name
			if(pFuse[index]->to->name[0] != 0){
				fprintf(fn,"\t\t<to_node>%s:%s</to_node>\n",pFuse[index]->to->oclass->name,pFuse[index]->to->name);
			} else {
				fprintf(fn,"\t\t<to_node>%s:%d</to_node>\n",pFuse[index]->to->oclass->name,pFuse[index]->to->id);
			}
			
			//write the from node's voltage
			if(pFuse[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pFuse[index]->from,"voltage_A");
			} else if(pFuse[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pFuse[index]->from,"voltage_B");
			} else if(pFuse[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pFuse[index]->from,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<from_voltage>%f</from_voltage>\n",node_voltage->Mag());

			//write the to node's voltage
			if(pFuse[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pFuse[index]->to,"voltage_A");
			} else if(pFuse[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pFuse[index]->to,"voltage_B");
			} else if(pFuse[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pFuse[index]->to,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<to_voltage>%f</to_voltage>\n",node_voltage->Mag());

			//write the phases
			if(pFuse[index]->phases == 0x0001){//A
				fprintf(fn,"\t\t<phases>A</phases>\n");
			}
			if(pFuse[index]->phases == 0x0002){//B
				fprintf(fn,"\t\t<phases>B</phases>\n");
			}
			if(pFuse[index]->phases == 0x0004){//C
				fprintf(fn,"\t\t<phases>C</phases>\n");
			}
			if(pFuse[index]->phases == 0x0009){//AN
				fprintf(fn,"\t\t<phases>AN</phases>\n");
			}
			if(pFuse[index]->phases == 0x000a){//BN
				fprintf(fn,"\t\t<phases>BN</phases>\n");
			}
			if(pFuse[index]->phases == 0x000c){//CN
				fprintf(fn,"\t\t<phases>CN</phases>\n");
			}
			if(pFuse[index]->phases == 0x0071){//AS
				fprintf(fn,"\t\t<phases>AS</phases>\n");
			}
			if(pFuse[index]->phases == 0x0072){//BS
				fprintf(fn,"\t\t<phases>BS</phases>\n");
			}
			if(pFuse[index]->phases == 0x0074){//CS
				fprintf(fn,"\t\t<phases>CS</phases>\n");
			}
			if(pFuse[index]->phases == 0x0003){//AB
				fprintf(fn,"\t\t<phases>AB</phases>\n");
			}
			if(pFuse[index]->phases == 0x0006){//BC
				fprintf(fn,"\t\t<phases>BC</phases>\n");
			}
			if(pFuse[index]->phases == 0x0005){//AC
				fprintf(fn,"\t\t<phases>AC</phases>\n");
			}
			if(pFuse[index]->phases == 0x000b){//ABN
				fprintf(fn,"\t\t<phases>ABN</phases>\n");
			}
			if(pFuse[index]->phases == 0x000e){//BCN
				fprintf(fn,"\t\t<phases>BCN</phases>\n");
			}
			if(pFuse[index]->phases == 0x000d){//ACN
				fprintf(fn,"\t\t<phases>ACN</phases>\n");
			}
			if(pFuse[index]->phases == 0x0007){//ABC
				fprintf(fn,"\t\t<phases>ABC</phases>\n");
			}
			if(pFuse[index]->phases == 0x000f){//ABCN
				fprintf(fn,"\t\t<phases>ABCN</phases>\n");
			}
			if(pFuse[index]->phases == 0x0107){//ABCD
				fprintf(fn,"\t\t<phases>ABCD</phases>\n");
			}

			//write a_mat
			fprintf(fn,"\t\t<a_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<a%d%d>%+.15f%+.15fj</a%d%d>\n",x+1,y+1,pFuse[index]->a_mat[x][y].Re(),pFuse[index]->a_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</a_matrix>\n");

			//write b_mat
			fprintf(fn,"\t\t<b_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<b%d%d>%+.15f%+.15fj</b%d%d>\n",x+1,y+1,pFuse[index]->b_mat[x][y].Re(),pFuse[index]->b_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</b_matrix>\n");

			//write c_mat
			fprintf(fn,"\t\t<c_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<c%d%d>%+.15f%+.15fj</c%d%d>\n",x+1,y+1,pFuse[index]->c_mat[x][y].Re(),pFuse[index]->c_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</c_matrix>\n");

			//write d_mat
			fprintf(fn,"\t\t<d_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<d%d%d>%+.15f%+.15fj</d%d%d>\n",x+1,y+1,pFuse[index]->d_mat[x][y].Re(),pFuse[index]->d_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</d_matrix>\n");

			//write A_mat
			fprintf(fn,"\t\t<A_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<A%d%d>%+.15f%+.15fj</A%d%d>\n",x+1,y+1,pFuse[index]->A_mat[x][y].Re(),pFuse[index]->A_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</A_matrix>\n");

			//write B_mat
			fprintf(fn,"\t\t<B_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<B%d%d>%+.15f%+.15fj</B%d%d>\n",x+1,y+1,pFuse[index]->B_mat[x][y].Re(),pFuse[index]->B_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</B_matrix>\n");

			fprintf(fn,"\t</fuse>\n");
			
			++index;
		}
	}

	index = 0;
	//write the overhead_lines
	if(ohlines != NULL){
		pOhLine = (line **)gl_malloc(ohlines->hit_count*sizeof(line*));
		if(pOhLine == NULL){
			gl_error("Failed to allocate fuse array.");
			return TS_NEVER;
		}
		while(obj = gl_find_next(ohlines,obj)){
			if(index >= ohlines->hit_count){
				break;
			}
			pOhLine[index] = OBJECTDATA(obj,line);
			if(pOhLine[index] == NULL){
				gl_error("Unable to map object as overhead_line object.");
				return 0;
			}

			//write the overhead_line
			fprintf(fn,"\t<overhead_line>\n");

			//write the name
			if(obj->name[0] != 0){
				fprintf(fn,"\t\t<name>%s</name>\n",obj->name);
			} else {
				fprintf(fn,"\t\t<name>NA</name>\n");
			}

			//write the id
			fprintf(fn,"\t\t<id>overhead_line:%d</id>\n",obj->id);

			//write the from name
			if(pOhLine[index]->from->name[0] != 0){
				fprintf(fn,"\t\t<from_node>%s:%s</from_node>\n",pOhLine[index]->from->oclass->name,pOhLine[index]->from->name);
			} else {
				fprintf(fn,"\t\t<from_node>%s:%d</from_node>\n",pOhLine[index]->from->oclass->name,pOhLine[index]->from->id);
			}

			//write the to name
			if(pOhLine[index]->to->name[0] != 0){
				fprintf(fn,"\t\t<to_node>%s:%s</to_node>\n",pOhLine[index]->to->oclass->name,pOhLine[index]->to->name);
			} else {
				fprintf(fn,"\t\t<to_node>%s:%d</to_node>\n",pOhLine[index]->to->oclass->name,pOhLine[index]->to->id);
			}

			//write the from node's voltage
			if(pOhLine[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pOhLine[index]->from,"voltage_A");
			} else if(pOhLine[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pOhLine[index]->from,"voltage_B");
			} else if(pOhLine[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pOhLine[index]->from,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<from_voltage>%f</from_voltage>\n",node_voltage->Mag());

			//write the to node's voltage
			if(pOhLine[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pOhLine[index]->to,"voltage_A");
			} else if(pOhLine[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pOhLine[index]->to,"voltage_B");
			} else if(pOhLine[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pOhLine[index]->to,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<to_voltage>%f</to_voltage>\n",node_voltage->Mag());

			//write the phases
			if(pOhLine[index]->phases == 0x0001){//A
				fprintf(fn,"\t\t<phases>A</phases>\n");
			}
			if(pOhLine[index]->phases == 0x0002){//B
				fprintf(fn,"\t\t<phases>B</phases>\n");
			}
			if(pOhLine[index]->phases == 0x0004){//C
				fprintf(fn,"\t\t<phases>C</phases>\n");
			}
			if(pOhLine[index]->phases == 0x0009){//AN
				fprintf(fn,"\t\t<phases>AN</phases>\n");
			}
			if(pOhLine[index]->phases == 0x000a){//BN
				fprintf(fn,"\t\t<phases>BN</phases>\n");
			}
			if(pOhLine[index]->phases == 0x000c){//CN
				fprintf(fn,"\t\t<phases>CN</phases>\n");
			}
			if(pOhLine[index]->phases == 0x0071){//AS
				fprintf(fn,"\t\t<phases>AS</phases>\n");
			}
			if(pOhLine[index]->phases == 0x0072){//BS
				fprintf(fn,"\t\t<phases>BS</phases>\n");
			}
			if(pOhLine[index]->phases == 0x0074){//CS
				fprintf(fn,"\t\t<phases>CS</phases>\n");
			}
			if(pOhLine[index]->phases == 0x0003){//AB
				fprintf(fn,"\t\t<phases>AB</phases>\n");
			}
			if(pOhLine[index]->phases == 0x0006){//BC
				fprintf(fn,"\t\t<phases>BC</phases>\n");
			}
			if(pOhLine[index]->phases == 0x0005){//AC
				fprintf(fn,"\t\t<phases>AC</phases>\n");
			}
			if(pOhLine[index]->phases == 0x000b){//ABN
				fprintf(fn,"\t\t<phases>ABN</phases>\n");
			}
			if(pOhLine[index]->phases == 0x000e){//BCN
				fprintf(fn,"\t\t<phases>BCN</phases>\n");
			}
			if(pOhLine[index]->phases == 0x000d){//ACN
				fprintf(fn,"\t\t<phases>ACN</phases>\n");
			}
			if(pOhLine[index]->phases == 0x0007){//ABC
				fprintf(fn,"\t\t<phases>ABC</phases>\n");
			}
			if(pOhLine[index]->phases == 0x000f){//ABCN
				fprintf(fn,"\t\t<phases>ABCN</phases>\n");
			}
			if(pOhLine[index]->phases == 0x0107){//ABCD
				fprintf(fn,"\t\t<phases>ABCD</phases>\n");
			}

			//write the length
			fprintf(fn,"\t\t<length>%.15f</length>\n",pOhLine[index]->length);

			//write a_mat
			fprintf(fn,"\t\t<a_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<a%d%d>%+.15f%+.15fj</a%d%d>\n",x+1,y+1,pOhLine[index]->a_mat[x][y].Re(),pOhLine[index]->a_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</a_matrix>\n");

			//write b_mat
			fprintf(fn,"\t\t<b_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<b%d%d>%+.15f%+.15fj</b%d%d>\n",x+1,y+1,pOhLine[index]->b_mat[x][y].Re(),pOhLine[index]->b_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</b_matrix>\n");

			//write c_mat
			fprintf(fn,"\t\t<c_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<c%d%d>%+.15f%+.15fj</c%d%d>\n",x+1,y+1,pOhLine[index]->c_mat[x][y].Re(),pOhLine[index]->c_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</c_matrix>\n");

			//write d_mat
			fprintf(fn,"\t\t<d_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<d%d%d>%+.15f%+.15fj</d%d%d>\n",x+1,y+1,pOhLine[index]->d_mat[x][y].Re(),pOhLine[index]->d_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</d_matrix>\n");

			//write A_mat
			fprintf(fn,"\t\t<A_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<A%d%d>%+.15f%+.15fj</A%d%d>\n",x+1,y+1,pOhLine[index]->A_mat[x][y].Re(),pOhLine[index]->A_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</A_matrix>\n");

			//write B_mat
			fprintf(fn,"\t\t<B_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<B%d%d>%+.15f%+.15fj</B%d%d>\n",x+1,y+1,pOhLine[index]->B_mat[x][y].Re(),pOhLine[index]->B_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</B_matrix>\n");

			fprintf(fn,"\t</overhead_line>\n");
			
			++index;
		}
	}

	index = 0;
	//write reclosers
	if(reclosers != NULL){
		pRecloser = (link_object **)gl_malloc(reclosers->hit_count*sizeof(link_object*));
		if(pRecloser == NULL){
			gl_error("Failed to allocate fuse array.");
			return TS_NEVER;
		}
		while(obj = gl_find_next(reclosers,obj)){
			if(index >= reclosers->hit_count){
				break;
			}
			pRecloser[index] = OBJECTDATA(obj,link_object);
			if(pRecloser[index] == NULL){
				gl_error("Unable to map object as a link object.");
				return 0;
			}

			//write the recloser
			fprintf(fn,"\t<recloser>\n");

			//write the name
			if(obj->name[0] != 0){
				fprintf(fn,"\t\t<name>%s</name>\n",obj->name);
			} else {
				fprintf(fn,"\t\t<name>NA</name>\n");
			}

			//write the id
			fprintf(fn,"\t\t<id>recloser:%d</id>\n",obj->id);

			//write the from name
			if(pRecloser[index]->from->name[0] != 0){
				fprintf(fn,"\t\t<from_node>%s:%s</from_node>\n",pRecloser[index]->from->oclass->name,pRecloser[index]->from->name);
			} else {
				fprintf(fn,"\t\t<from_node>%s:%d</from_node>\n",pRecloser[index]->from->oclass->name,pRecloser[index]->from->id);
			}

			//write the to name
			if(pRecloser[index]->to->name[0] != 0){
				fprintf(fn,"\t\t<to_node>%s:%s</to_node>\n",pRecloser[index]->to->oclass->name,pRecloser[index]->to->name);
			} else {
				fprintf(fn,"\t\t<to_node>%s:%d</to_node>\n",pRecloser[index]->to->oclass->name,pRecloser[index]->to->id);
			}

			//write the from node's voltage
			if(pRecloser[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pRecloser[index]->from,"voltage_A");
			} else if(pRecloser[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pRecloser[index]->from,"voltage_B");
			} else if(pRecloser[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pRecloser[index]->from,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<from_voltage>%f</from_voltage>\n",node_voltage->Mag());

			//write the to node's voltage
			if(pRecloser[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pRecloser[index]->to,"voltage_A");
			} else if(pRecloser[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pRecloser[index]->to,"voltage_B");
			} else if(pRecloser[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pRecloser[index]->to,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<to_voltage>%f</to_voltage>\n",node_voltage->Mag());

			//write the phases
			if(pRecloser[index]->phases == 0x0001){//A
				fprintf(fn,"\t\t<phases>A</phases>\n");
			}
			if(pRecloser[index]->phases == 0x0002){//B
				fprintf(fn,"\t\t<phases>B</phases>\n");
			}
			if(pRecloser[index]->phases == 0x0004){//C
				fprintf(fn,"\t\t<phases>C</phases>\n");
			}
			if(pRecloser[index]->phases == 0x0009){//AN
				fprintf(fn,"\t\t<phases>AN</phases>\n");
			}
			if(pRecloser[index]->phases == 0x000a){//BN
				fprintf(fn,"\t\t<phases>BN</phases>\n");
			}
			if(pRecloser[index]->phases == 0x000c){//CN
				fprintf(fn,"\t\t<phases>CN</phases>\n");
			}
			if(pRecloser[index]->phases == 0x0071){//AS
				fprintf(fn,"\t\t<phases>AS</phases>\n");
			}
			if(pRecloser[index]->phases == 0x0072){//BS
				fprintf(fn,"\t\t<phases>BS</phases>\n");
			}
			if(pRecloser[index]->phases == 0x0074){//CS
				fprintf(fn,"\t\t<phases>CS</phases>\n");
			}
			if(pRecloser[index]->phases == 0x0003){//AB
				fprintf(fn,"\t\t<phases>AB</phases>\n");
			}
			if(pRecloser[index]->phases == 0x0006){//BC
				fprintf(fn,"\t\t<phases>BC</phases>\n");
			}
			if(pRecloser[index]->phases == 0x0005){//AC
				fprintf(fn,"\t\t<phases>AC</phases>\n");
			}
			if(pRecloser[index]->phases == 0x000b){//ABN
				fprintf(fn,"\t\t<phases>ABN</phases>\n");
			}
			if(pRecloser[index]->phases == 0x000e){//BCN
				fprintf(fn,"\t\t<phases>BCN</phases>\n");
			}
			if(pRecloser[index]->phases == 0x000d){//ACN
				fprintf(fn,"\t\t<phases>ACN</phases>\n");
			}
			if(pRecloser[index]->phases == 0x0007){//ABC
				fprintf(fn,"\t\t<phases>ABC</phases>\n");
			}
			if(pRecloser[index]->phases == 0x000f){//ABCN
				fprintf(fn,"\t\t<phases>ABCN</phases>\n");
			}
			if(pRecloser[index]->phases == 0x0107){//ABCD
				fprintf(fn,"\t\t<phases>ABCD</phases>\n");
			}

			//write a_mat
			fprintf(fn,"\t\t<a_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<a%d%d>%+.15f%+.15fj</a%d%d>\n",x+1,y+1,pRecloser[index]->a_mat[x][y].Re(),pRecloser[index]->a_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</a_matrix>\n");

			//write b_mat
			fprintf(fn,"\t\t<b_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<b%d%d>%+.15f%+.15fj</b%d%d>\n",x+1,y+1,pRecloser[index]->b_mat[x][y].Re(),pRecloser[index]->b_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</b_matrix>\n");

			//write c_mat
			fprintf(fn,"\t\t<c_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<c%d%d>%+.15f%+.15fj</c%d%d>\n",x+1,y+1,pRecloser[index]->c_mat[x][y].Re(),pRecloser[index]->c_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</c_matrix>\n");

			//write d_mat
			fprintf(fn,"\t\t<d_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<d%d%d>%+.15f%+.15fj</d%d%d>\n",x+1,y+1,pRecloser[index]->d_mat[x][y].Re(),pRecloser[index]->d_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</d_matrix>\n");

			//write A_mat
			fprintf(fn,"\t\t<A_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<A%d%d>%+.15f%+.15fj</A%d%d>\n",x+1,y+1,pRecloser[index]->A_mat[x][y].Re(),pRecloser[index]->A_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</A_matrix>\n");

			//write B_mat
			fprintf(fn,"\t\t<B_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<B%d%d>%+.15f%+.15fj</B%d%d>\n",x+1,y+1,pRecloser[index]->B_mat[x][y].Re(),pRecloser[index]->B_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</B_matrix>\n");

			fprintf(fn,"\t</recloser>\n");
			
			++index;
		}
	}
	
	index = 0;
	//write regulators
	if(regulators != NULL){
		pRegulator = (regulator **)gl_malloc(regulators->hit_count*sizeof(regulator*));
		if(pRegulator == NULL){
			gl_error("Failed to allocate fuse array.");
			return TS_NEVER;
		}
		while(obj = gl_find_next(regulators,obj)){
			if(index >= regulators->hit_count){
				break;
			}
			pRegulator[index] = OBJECTDATA(obj,regulator);
			if(pRegulator[index] == NULL){
				gl_error("Unable to map object as a link object.");
				return 0;
			}

			//write the regulator
			fprintf(fn,"\t<regulator>\n");

			//write the name
			if(obj->name != NULL){
				fprintf(fn,"\t\t<name>%s</name>\n",obj->name);
			} else {
				fprintf(fn,"\t\t<name>NA</name>\n");
			}

			//write the id
			fprintf(fn,"\t\t<id>regulator:%d</id>\n",obj->id);

			//write the from name
			if(pRegulator[index]->from->name[0] != 0){
				fprintf(fn,"\t\t<from_node>%s:%s</from_node>\n",pRegulator[index]->from->oclass->name,pRegulator[index]->from->name);
			} else {
				fprintf(fn,"\t\t<from_node>%s:%d</from_node>\n",pRegulator[index]->from->oclass->name,pRegulator[index]->from->id);
			}

			//write the to name
			if(pRegulator[index]->to->name[0] != 0){
				fprintf(fn,"\t\t<to_node>%s:%s</to_node>\n",pRegulator[index]->to->oclass->name,pRegulator[index]->to->name);
			} else {
				fprintf(fn,"\t\t<to_node>%s:%d</to_node>\n",pRegulator[index]->to->oclass->name,pRegulator[index]->to->id);
			}

			//write the from node's voltage
			if(pRegulator[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pRegulator[index]->from,"voltage_A");
			} else if(pRegulator[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pRegulator[index]->from,"voltage_B");
			} else if(pRegulator[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pRegulator[index]->from,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<from_voltage>%f</from_voltage>\n",node_voltage->Mag());

			//write the to node's voltage
			if(pRegulator[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pRegulator[index]->to,"voltage_A");
			} else if(pRegulator[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pRegulator[index]->to,"voltage_B");
			} else if(pRegulator[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pRegulator[index]->to,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<to_voltage>%f</to_voltage>\n",node_voltage->Mag());

			//write the phases
			if(pRegulator[index]->phases == 0x0001){//A
				fprintf(fn,"\t\t<phases>A</phases>\n");
			}
			if(pRegulator[index]->phases == 0x0002){//B
				fprintf(fn,"\t\t<phases>B</phases>\n");
			}
			if(pRegulator[index]->phases == 0x0004){//C
				fprintf(fn,"\t\t<phases>C</phases>\n");
			}
			if(pRegulator[index]->phases == 0x0009){//AN
				fprintf(fn,"\t\t<phases>AN</phases>\n");
			}
			if(pRegulator[index]->phases == 0x000a){//BN
				fprintf(fn,"\t\t<phases>BN</phases>\n");
			}
			if(pRegulator[index]->phases == 0x000c){//CN
				fprintf(fn,"\t\t<phases>CN</phases>\n");
			}
			if(pRegulator[index]->phases == 0x0071){//AS
				fprintf(fn,"\t\t<phases>AS</phases>\n");
			}
			if(pRegulator[index]->phases == 0x0072){//BS
				fprintf(fn,"\t\t<phases>BS</phases>\n");
			}
			if(pRegulator[index]->phases == 0x0074){//CS
				fprintf(fn,"\t\t<phases>CS</phases>\n");
			}
			if(pRegulator[index]->phases == 0x0003){//AB
				fprintf(fn,"\t\t<phases>AB</phases>\n");
			}
			if(pRegulator[index]->phases == 0x0006){//BC
				fprintf(fn,"\t\t<phases>BC</phases>\n");
			}
			if(pRegulator[index]->phases == 0x0005){//AC
				fprintf(fn,"\t\t<phases>AC</phases>\n");
			}
			if(pRegulator[index]->phases == 0x000b){//ABN
				fprintf(fn,"\t\t<phases>ABN</phases>\n");
			}
			if(pRegulator[index]->phases == 0x000e){//BCN
				fprintf(fn,"\t\t<phases>BCN</phases>\n");
			}
			if(pRegulator[index]->phases == 0x000d){//ACN
				fprintf(fn,"\t\t<phases>ACN</phases>\n");
			}
			if(pRegulator[index]->phases == 0x0007){//ABC
				fprintf(fn,"\t\t<phases>ABC</phases>\n");
			}
			if(pRegulator[index]->phases == 0x000f){//ABCN
				fprintf(fn,"\t\t<phases>ABCN</phases>\n");
			}
			if(pRegulator[index]->phases == 0x0107){//ABCD
				fprintf(fn,"\t\t<phases>ABCD</phases>\n");
			}

			//write taps
			fprintf(fn,"\t\t<tapA>%d</tapA>\n",pRegulator[index]->tap[0]);
			fprintf(fn,"\t\t<tapB>%d</tapB>\n",pRegulator[index]->tap[1]);
			fprintf(fn,"\t\t<tapC>%d</tapC>\n",pRegulator[index]->tap[2]);
			//write a_mat
			fprintf(fn,"\t\t<a_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<a%d%d>%+.15f%+.15fj</a%d%d>\n",x+1,y+1,pRegulator[index]->a_mat[x][y].Re(),pRegulator[index]->a_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</a_matrix>\n");

			//write b_mat
			fprintf(fn,"\t\t<b_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<b%d%d>%+.15f%+.15fj</b%d%d>\n",x+1,y+1,pRegulator[index]->b_mat[x][y].Re(),pRegulator[index]->b_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</b_matrix>\n");

			//write c_mat
			fprintf(fn,"\t\t<c_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<c%d%d>%+.15f%+.15fj</c%d%d>\n",x+1,y+1,pRegulator[index]->c_mat[x][y].Re(),pRegulator[index]->c_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</c_matrix>\n");

			//write d_mat
			fprintf(fn,"\t\t<d_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<d%d%d>%+.15f%+.15fj</d%d%d>\n",x+1,y+1,pRegulator[index]->d_mat[x][y].Re(),pRegulator[index]->d_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</d_matrix>\n");

			//write A_mat
			fprintf(fn,"\t\t<A_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<A%d%d>%+.15f%+.15fj</A%d%d>\n",x+1,y+1,pRegulator[index]->A_mat[x][y].Re(),pRegulator[index]->A_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</A_matrix>\n");

			//write B_mat
			fprintf(fn,"\t\t<B_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<B%d%d>%+.15f%+.15fj</B%d%d>\n",x+1,y+1,pRegulator[index]->B_mat[x][y].Re(),pRegulator[index]->B_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</B_matrix>\n");

			fprintf(fn,"\t</regulator>\n");
			
			++index;
		}
	}
	
	index = 0;
	//write relays
	if(relays != NULL){
		pRelay = (link_object **)gl_malloc(relays->hit_count*sizeof(link_object*));
		if(pRelay == NULL){
			gl_error("Failed to allocate fuse array.");
			return TS_NEVER;
		}
		while(obj = gl_find_next(relays,obj)){
			if(index >= relays->hit_count){
				break;
			}
			pRelay[index] = OBJECTDATA(obj,link_object);
			if(pRelay[index] == NULL){
				gl_error("Unable to map object as a link object.");
				return 0;
			}

			//write the relay
			fprintf(fn,"\t<relay>\n");

			//write the name
			if(obj->name[0] != 0){
				fprintf(fn,"\t\t<name>%s</name>\n",obj->name);
			} else {
				fprintf(fn,"\t\t<name>NA</name>\n");
			}

			//write the id
			fprintf(fn,"\t\t<id>relay:%d</id>\n",obj->id);

			//write the from name
			if(pRelay[index]->from->name[0] != 0){
				fprintf(fn,"\t\t<from_node>%s:%s</from_node>\n",pRelay[index]->from->oclass->name,pRelay[index]->from->name);
			} else {
				fprintf(fn,"\t\t<from_node>%s:%d</from_node>\n",pRelay[index]->from->oclass->name,pRelay[index]->from->id);
			}

			//write the to name
			if(pRelay[index]->to->name[0] != 0){
				fprintf(fn,"\t\t<to_node>%s:%s</to_node>\n",pRelay[index]->to->oclass->name,pRelay[index]->to->name);
			} else {
				fprintf(fn,"\t\t<to_node>%s:%d</to_node>\n",pRelay[index]->to->oclass->name,pRelay[index]->to->id);
			}

			//write the from node's voltage
			if(pRelay[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pRelay[index]->from,"voltage_A");
			} else if(pRelay[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pRelay[index]->from,"voltage_B");
			} else if(pRelay[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pRelay[index]->from,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<from_voltage>%f</from_voltage>\n",node_voltage->Mag());

			//write the to node's voltage
			if(pRelay[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pRelay[index]->to,"voltage_A");
			} else if(pRelay[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pRelay[index]->to,"voltage_B");
			} else if(pRelay[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pRelay[index]->to,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<to_voltage>%f</to_voltage>\n",node_voltage->Mag());

			//write the phases
			if(pRelay[index]->phases == 0x0001){//A
				fprintf(fn,"\t\t<phases>A</phases>\n");
			}
			if(pRelay[index]->phases == 0x0002){//B
				fprintf(fn,"\t\t<phases>B</phases>\n");
			}
			if(pRelay[index]->phases == 0x0004){//C
				fprintf(fn,"\t\t<phases>C</phases>\n");
			}
			if(pRelay[index]->phases == 0x0009){//AN
				fprintf(fn,"\t\t<phases>AN</phases>\n");
			}
			if(pRelay[index]->phases == 0x000a){//BN
				fprintf(fn,"\t\t<phases>BN</phases>\n");
			}
			if(pRelay[index]->phases == 0x000c){//CN
				fprintf(fn,"\t\t<phases>CN</phases>\n");
			}
			if(pRelay[index]->phases == 0x0071){//AS
				fprintf(fn,"\t\t<phases>AS</phases>\n");
			}
			if(pRelay[index]->phases == 0x0072){//BS
				fprintf(fn,"\t\t<phases>BS</phases>\n");
			}
			if(pRelay[index]->phases == 0x0074){//CS
				fprintf(fn,"\t\t<phases>CS</phases>\n");
			}
			if(pRelay[index]->phases == 0x0003){//AB
				fprintf(fn,"\t\t<phases>AB</phases>\n");
			}
			if(pRelay[index]->phases == 0x0006){//BC
				fprintf(fn,"\t\t<phases>BC</phases>\n");
			}
			if(pRelay[index]->phases == 0x0005){//AC
				fprintf(fn,"\t\t<phases>AC</phases>\n");
			}
			if(pRelay[index]->phases == 0x000b){//ABN
				fprintf(fn,"\t\t<phases>ABN</phases>\n");
			}
			if(pRelay[index]->phases == 0x000e){//BCN
				fprintf(fn,"\t\t<phases>BCN</phases>\n");
			}
			if(pRelay[index]->phases == 0x000d){//ACN
				fprintf(fn,"\t\t<phases>ACN</phases>\n");
			}
			if(pRelay[index]->phases == 0x0007){//ABC
				fprintf(fn,"\t\t<phases>ABC</phases>\n");
			}
			if(pRelay[index]->phases == 0x000f){//ABCN
				fprintf(fn,"\t\t<phases>ABCN</phases>\n");
			}
			if(pRelay[index]->phases == 0x0107){//ABCD
				fprintf(fn,"\t\t<phases>ABCD</phases>\n");
			}

			//write a_mat
			fprintf(fn,"\t\t<a_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<a%d%d>%+.15f%+.15fj</a%d%d>\n",x+1,y+1,pRelay[index]->a_mat[x][y].Re(),pRelay[index]->a_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</a_matrix>\n");

			//write b_mat
			fprintf(fn,"\t\t<b_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<b%d%d>%+.15f%+.15fj</b%d%d>\n",x+1,y+1,pRelay[index]->b_mat[x][y].Re(),pRelay[index]->b_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</b_matrix>\n");

			//write c_mat
			fprintf(fn,"\t\t<c_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<c%d%d>%+.15f%+.15fj</c%d%d>\n",x+1,y+1,pRelay[index]->c_mat[x][y].Re(),pRelay[index]->c_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</c_matrix>\n");

			//write d_mat
			fprintf(fn,"\t\t<d_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<d%d%d>%+.15f%+.15fj</d%d%d>\n",x+1,y+1,pRelay[index]->d_mat[x][y].Re(),pRelay[index]->d_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</d_matrix>\n");

			//write A_mat
			fprintf(fn,"\t\t<A_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<A%d%d>%+.15f%+.15fj</A%d%d>\n",x+1,y+1,pRelay[index]->A_mat[x][y].Re(),pRelay[index]->A_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</A_matrix>\n");

			//write B_mat
			fprintf(fn,"\t\t<B_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<B%d%d>%+.15f%+.15fj</B%d%d>\n",x+1,y+1,pRelay[index]->B_mat[x][y].Re(),pRelay[index]->B_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</B_matrix>\n");

			fprintf(fn,"\t</relay>\n");
			
			++index;
		}
	}
	
	index = 0;
	//write sectionalizers
	if(sectionalizers != NULL){
		pSectionalizer = (link_object **)gl_malloc(sectionalizers->hit_count*sizeof(link_object*));
		if(pSectionalizer == NULL){
			gl_error("Failed to allocate fuse array.");
			return TS_NEVER;
		}
		while(obj = gl_find_next(sectionalizers,obj)){
			if(index >= sectionalizers->hit_count){
				break;
			}
			pSectionalizer[index] = OBJECTDATA(obj,link_object);
			if(pSectionalizer[index] == NULL){
				gl_error("Unable to map object as a link object.");
				return 0;
			}

			//write the sectionalizer
			fprintf(fn,"\t<sectionalizer>\n");

			//write the name
			if(obj->name[0] != 0){
				fprintf(fn,"\t\t<name>%s</name>\n",obj->name);
			} else {
				fprintf(fn,"\t\t<name>NA</name>\n");
			}

			//write the id
			fprintf(fn,"\t\t<id>sectionalizer:%d</id>\n",obj->id);

			//write the from name
			if(pSectionalizer[index]->from->name[0] != 0){
				fprintf(fn,"\t\t<from_node>%s:%s</from_node>\n",pSectionalizer[index]->from->oclass->name,pSectionalizer[index]->from->name);
			} else {
				fprintf(fn,"\t\t<from_node>%s:%d</from_node>\n",pSectionalizer[index]->from->oclass->name,pSectionalizer[index]->from->id);
			}

			//write the to name
			if(pSectionalizer[index]->to->name[0] != 0){
				fprintf(fn,"\t\t<to_node>%s:%s</to_node>\n",pSectionalizer[index]->to->oclass->name,pSectionalizer[index]->to->name);
			} else {
				fprintf(fn,"\t\t<to_node>%s:%d</to_node>\n",pSectionalizer[index]->to->oclass->name,pSectionalizer[index]->to->id);
			}

			//write the from node's voltage
			if(pSectionalizer[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pSectionalizer[index]->from,"voltage_A");
			} else if(pSectionalizer[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pSectionalizer[index]->from,"voltage_B");
			} else if(pSectionalizer[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pSectionalizer[index]->from,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<from_voltage>%f</from_voltage>\n",node_voltage->Mag());

			//write the to node's voltage
			if(pSectionalizer[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pSectionalizer[index]->to,"voltage_A");
			} else if(pSectionalizer[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pSectionalizer[index]->to,"voltage_B");
			} else if(pSectionalizer[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pSectionalizer[index]->to,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<to_voltage>%f</to_voltage>\n",node_voltage->Mag());

			//write the phases
			if(pSectionalizer[index]->phases == 0x0001){//A
				fprintf(fn,"\t\t<phases>A</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x0002){//B
				fprintf(fn,"\t\t<phases>B</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x0004){//C
				fprintf(fn,"\t\t<phases>C</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x0009){//AN
				fprintf(fn,"\t\t<phases>AN</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x000a){//BN
				fprintf(fn,"\t\t<phases>BN</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x000c){//CN
				fprintf(fn,"\t\t<phases>CN</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x0071){//AS
				fprintf(fn,"\t\t<phases>AS</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x0072){//BS
				fprintf(fn,"\t\t<phases>BS</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x0074){//CS
				fprintf(fn,"\t\t<phases>CS</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x0003){//AB
				fprintf(fn,"\t\t<phases>AB</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x0006){//BC
				fprintf(fn,"\t\t<phases>BC</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x0005){//AC
				fprintf(fn,"\t\t<phases>AC</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x000b){//ABN
				fprintf(fn,"\t\t<phases>ABN</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x000e){//BCN
				fprintf(fn,"\t\t<phases>BCN</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x000d){//ACN
				fprintf(fn,"\t\t<phases>ACN</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x0007){//ABC
				fprintf(fn,"\t\t<phases>ABC</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x000f){//ABCN
				fprintf(fn,"\t\t<phases>ABCN</phases>\n");
			}
			if(pSectionalizer[index]->phases == 0x0107){//ABCD
				fprintf(fn,"\t\t<phases>ABCD</phases>\n");
			}

			//write a_mat
			fprintf(fn,"\t\t<a_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<a%d%d>%+.15f%+.15fj</a%d%d>\n",x+1,y+1,pSectionalizer[index]->a_mat[x][y].Re(),pSectionalizer[index]->a_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</a_matrix>\n");

			//write b_mat
			fprintf(fn,"\t\t<b_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<b%d%d>%+.15f%+.15fj</b%d%d>\n",x+1,y+1,pSectionalizer[index]->b_mat[x][y].Re(),pSectionalizer[index]->b_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</b_matrix>\n");

			//write c_mat
			fprintf(fn,"\t\t<c_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<c%d%d>%+.15f%+.15fj</c%d%d>\n",x+1,y+1,pSectionalizer[index]->c_mat[x][y].Re(),pSectionalizer[index]->c_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</c_matrix>\n");

			//write d_mat
			fprintf(fn,"\t\t<d_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<d%d%d>%+.15f%+.15fj</d%d%d>\n",x+1,y+1,pSectionalizer[index]->d_mat[x][y].Re(),pSectionalizer[index]->d_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</d_matrix>\n");

			//write A_mat
			fprintf(fn,"\t\t<A_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<A%d%d>%+.15f%+.15fj</A%d%d>\n",x+1,y+1,pSectionalizer[index]->A_mat[x][y].Re(),pSectionalizer[index]->A_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</A_matrix>\n");

			//write B_mat
			fprintf(fn,"\t\t<B_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<B%d%d>%+.15f%+.15fj</B%d%d>\n",x+1,y+1,pSectionalizer[index]->B_mat[x][y].Re(),pSectionalizer[index]->B_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</B_matrix>\n");

			fprintf(fn,"\t</sectionalizer>\n");
			
			++index;
		}
	}
	
	index = 0;
	//write series reactors
	if(series_reactors != NULL){
		pSeriesReactor = (link_object **)gl_malloc(series_reactors->hit_count*sizeof(link_object*));
		if(pSeriesReactor == NULL){
			gl_error("Failed to allocate fuse array.");
			return TS_NEVER;
		}
		while(obj = gl_find_next(series_reactors,obj)){
			if(index >= series_reactors->hit_count){
				break;
			}
			pSeriesReactor[index] = OBJECTDATA(obj,link_object);
			if(pSeriesReactor[index] == NULL){
				gl_error("Unable to map object as a link object.");
				return 0;
			}

			//write the series reactor
			fprintf(fn,"\t<series reactor>\n");

			//write the name
			if(obj->name[0] != 0){
				fprintf(fn,"\t\t<name>%s</name>\n",obj->name);
			} else {
				fprintf(fn,"\t\t<name>NA</name>\n");
			}

			//write the id
			fprintf(fn,"\t\t<id>series_reactor:%d</id>\n",obj->id);

			//write the from name
			if(pSeriesReactor[index]->from->name[0] != 0){
				fprintf(fn,"\t\t<from_node>%s:%s</from_node>\n",pSeriesReactor[index]->from->oclass->name,pSeriesReactor[index]->from->name);
			} else {
				fprintf(fn,"\t\t<from_node>%s:%d</from_node>\n",pSeriesReactor[index]->from->oclass->name,pSeriesReactor[index]->from->id);
			}

			//write the to name
			if(pSeriesReactor[index]->to->name[0] != 0){
				fprintf(fn,"\t\t<to_node>%s:%s</to_node>\n",pSeriesReactor[index]->to->oclass->name,pSeriesReactor[index]->to->name);
			} else {
				fprintf(fn,"\t\t<to_node>%s:%d</to_node>\n",pSeriesReactor[index]->to->oclass->name,pSeriesReactor[index]->to->id);
			}

			//write the from node's voltage
			if(pSeriesReactor[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pSeriesReactor[index]->from,"voltage_A");
			} else if(pSeriesReactor[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pSeriesReactor[index]->from,"voltage_B");
			} else if(pSeriesReactor[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pSeriesReactor[index]->from,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<from_voltage>%f</from_voltage>\n",node_voltage->Mag());

			//write the to node's voltage
			if(pSeriesReactor[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pSeriesReactor[index]->to,"voltage_A");
			} else if(pSeriesReactor[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pSeriesReactor[index]->to,"voltage_B");
			} else if(pSeriesReactor[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pSeriesReactor[index]->to,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<to_voltage>%f</to_voltage>\n",node_voltage->Mag());

			//write the phases
			if(pSeriesReactor[index]->phases == 0x0001){//A
				fprintf(fn,"\t\t<phases>A</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x0002){//B
				fprintf(fn,"\t\t<phases>B</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x0004){//C
				fprintf(fn,"\t\t<phases>C</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x0009){//AN
				fprintf(fn,"\t\t<phases>AN</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x000a){//BN
				fprintf(fn,"\t\t<phases>BN</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x000c){//CN
				fprintf(fn,"\t\t<phases>CN</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x0071){//AS
				fprintf(fn,"\t\t<phases>AS</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x0072){//BS
				fprintf(fn,"\t\t<phases>BS</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x0074){//CS
				fprintf(fn,"\t\t<phases>CS</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x0003){//AB
				fprintf(fn,"\t\t<phases>AB</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x0006){//BC
				fprintf(fn,"\t\t<phases>BC</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x0005){//AC
				fprintf(fn,"\t\t<phases>AC</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x000b){//ABN
				fprintf(fn,"\t\t<phases>ABN</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x000e){//BCN
				fprintf(fn,"\t\t<phases>BCN</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x000d){//ACN
				fprintf(fn,"\t\t<phases>ACN</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x0007){//ABC
				fprintf(fn,"\t\t<phases>ABC</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x000f){//ABCN
				fprintf(fn,"\t\t<phases>ABCN</phases>\n");
			}
			if(pSeriesReactor[index]->phases == 0x0107){//ABCD
				fprintf(fn,"\t\t<phases>ABCD</phases>\n");
			}

			//write a_mat
			fprintf(fn,"\t\t<a_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<a%d%d>%+.15f%+.15fj</a%d%d>\n",x+1,y+1,pSeriesReactor[index]->a_mat[x][y].Re(),pSeriesReactor[index]->a_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</a_matrix>\n");

			//write b_mat
			fprintf(fn,"\t\t<b_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<b%d%d>%+.15f%+.15fj</b%d%d>\n",x+1,y+1,pSeriesReactor[index]->b_mat[x][y].Re(),pSeriesReactor[index]->b_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</b_matrix>\n");

			//write c_mat
			fprintf(fn,"\t\t<c_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<c%d%d>%+.15f%+.15fj</c%d%d>\n",x+1,y+1,pSeriesReactor[index]->c_mat[x][y].Re(),pSeriesReactor[index]->c_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</c_matrix>\n");

			//write d_mat
			fprintf(fn,"\t\t<d_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<d%d%d>%+.15f%+.15fj</d%d%d>\n",x+1,y+1,pSeriesReactor[index]->d_mat[x][y].Re(),pSeriesReactor[index]->d_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</d_matrix>\n");

			//write A_mat
			fprintf(fn,"\t\t<A_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<A%d%d>%+.15f%+.15fj</A%d%d>\n",x+1,y+1,pSeriesReactor[index]->A_mat[x][y].Re(),pSeriesReactor[index]->A_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</A_matrix>\n");

			//write B_mat
			fprintf(fn,"\t\t<B_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<B%d%d>%+.15f%+.15fj</B%d%d>\n",x+1,y+1,pSeriesReactor[index]->B_mat[x][y].Re(),pSeriesReactor[index]->B_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</B_matrix>\n");

			fprintf(fn,"\t</sereis reactor>\n");
			
			++index;
		}
	}
	
	index = 0;
	//write switches
	if(switches != NULL){
		pSwitch = (switch_object **)gl_malloc(switches->hit_count*sizeof(switch_object*));
		if(pSwitch == NULL){
			gl_error("Failed to allocate fuse array.");
			return TS_NEVER;
		}
		while(obj = gl_find_next(switches,obj)){
			if(index >= switches->hit_count){
				break;
			}
			pSwitch[index] = OBJECTDATA(obj,switch_object);
			if(pSwitch[index] == NULL){
				gl_error("Unable to map object as a link object.");
				return 0;
			}

			//write the switch
			if (pSwitch[index]->phase_A_state==1) {
			fprintf(fn,"\t<switch>\n");

			//write the name
			if(obj->name[0] != 0){
				fprintf(fn,"\t\t<name>%s</name>\n",obj->name);
			} else {
				fprintf(fn,"\t\t<name>NA</name>\n");
			}

			//write the id
			fprintf(fn,"\t\t<id>switch:%d</id>\n",obj->id);

			//write the from name
			if(pSwitch[index]->from->name[0] != 0){
				fprintf(fn,"\t\t<from_node>%s:%s</from_node>\n",pSwitch[index]->from->oclass->name,pSwitch[index]->from->name);
			} else {
				fprintf(fn,"\t\t<from_node>%s:%d</from_node>\n",pSwitch[index]->from->oclass->name,pSwitch[index]->from->id);
			}

			//write the to name
			if(pSwitch[index]->to->name[0] != 0){
				fprintf(fn,"\t\t<to_node>%s:%s</to_node>\n",pSwitch[index]->to->oclass->name,pSwitch[index]->to->name);
			} else {
				fprintf(fn,"\t\t<to_node>%s:%d</to_node>\n",pSwitch[index]->to->oclass->name,pSwitch[index]->to->id);
			}

			//write the from node's voltage
			if(pSwitch[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pSwitch[index]->from,"voltage_A");
			} else if(pSwitch[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pSwitch[index]->from,"voltage_B");
			} else if(pSwitch[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pSwitch[index]->from,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<from_voltage>%f</from_voltage>\n",node_voltage->Mag());

			//write the to node's voltage
			if(pSwitch[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pSwitch[index]->to,"voltage_A");
			} else if(pSwitch[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pSwitch[index]->to,"voltage_B");
			} else if(pSwitch[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pSwitch[index]->to,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<to_voltage>%f</to_voltage>\n",node_voltage->Mag());

			//write the phases
			if(pSwitch[index]->phases == 0x0001){//A
				fprintf(fn,"\t\t<phases>A</phases>\n");
			}
			if(pSwitch[index]->phases == 0x0002){//B
				fprintf(fn,"\t\t<phases>B</phases>\n");
			}
			if(pSwitch[index]->phases == 0x0004){//C
				fprintf(fn,"\t\t<phases>C</phases>\n");
			}
			if(pSwitch[index]->phases == 0x0009){//AN
				fprintf(fn,"\t\t<phases>AN</phases>\n");
			}
			if(pSwitch[index]->phases == 0x000a){//BN
				fprintf(fn,"\t\t<phases>BN</phases>\n");
			}
			if(pSwitch[index]->phases == 0x000c){//CN
				fprintf(fn,"\t\t<phases>CN</phases>\n");
			}
			if(pSwitch[index]->phases == 0x0071){//AS
				fprintf(fn,"\t\t<phases>AS</phases>\n");
			}
			if(pSwitch[index]->phases == 0x0072){//BS
				fprintf(fn,"\t\t<phases>BS</phases>\n");
			}
			if(pSwitch[index]->phases == 0x0074){//CS
				fprintf(fn,"\t\t<phases>CS</phases>\n");
			}
			if(pSwitch[index]->phases == 0x0003){//AB
				fprintf(fn,"\t\t<phases>AB</phases>\n");
			}
			if(pSwitch[index]->phases == 0x0006){//BC
				fprintf(fn,"\t\t<phases>BC</phases>\n");
			}
			if(pSwitch[index]->phases == 0x0005){//AC
				fprintf(fn,"\t\t<phases>AC</phases>\n");
			}
			if(pSwitch[index]->phases == 0x000b){//ABN
				fprintf(fn,"\t\t<phases>ABN</phases>\n");
			}
			if(pSwitch[index]->phases == 0x000e){//BCN
				fprintf(fn,"\t\t<phases>BCN</phases>\n");
			}
			if(pSwitch[index]->phases == 0x000d){//ACN
				fprintf(fn,"\t\t<phases>ACN</phases>\n");
			}
			if(pSwitch[index]->phases == 0x0007){//ABC
				fprintf(fn,"\t\t<phases>ABC</phases>\n");
			}
			if(pSwitch[index]->phases == 0x000f){//ABCN
				fprintf(fn,"\t\t<phases>ABCN</phases>\n");
			}
			if(pSwitch[index]->phases == 0x0107){//ABCD
				fprintf(fn,"\t\t<phases>ABCD</phases>\n");
			}

			//write a_mat
			fprintf(fn,"\t\t<a_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<a%d%d>%+.15f%+.15fj</a%d%d>\n",x+1,y+1,pSwitch[index]->a_mat[x][y].Re(),pSwitch[index]->a_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</a_matrix>\n");

			//write b_mat
			fprintf(fn,"\t\t<b_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<b%d%d>%+.15f%+.15fj</b%d%d>\n",x+1,y+1,pSwitch[index]->b_mat[x][y].Re(),pSwitch[index]->b_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</b_matrix>\n");

			//write c_mat
			fprintf(fn,"\t\t<c_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<c%d%d>%+.15f%+.15fj</c%d%d>\n",x+1,y+1,pSwitch[index]->c_mat[x][y].Re(),pSwitch[index]->c_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</c_matrix>\n");

			//write d_mat
			fprintf(fn,"\t\t<d_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<d%d%d>%+.15f%+.15fj</d%d%d>\n",x+1,y+1,pSwitch[index]->d_mat[x][y].Re(),pSwitch[index]->d_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</d_matrix>\n");

			//write A_mat
			fprintf(fn,"\t\t<A_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<A%d%d>%+.15f%+.15fj</A%d%d>\n",x+1,y+1,pSwitch[index]->A_mat[x][y].Re(),pSwitch[index]->A_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</A_matrix>\n");

			//write B_mat
			fprintf(fn,"\t\t<B_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<B%d%d>%+.15f%+.15fj</B%d%d>\n",x+1,y+1,pSwitch[index]->B_mat[x][y].Re(),pSwitch[index]->B_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</B_matrix>\n");

			fprintf(fn,"\t</switch>\n");
			
			++index;
			}
		}
	}
	
	index = 0;
	//write transformers
	if(transformers != NULL){
		pTransformer = (transformer **)gl_malloc(transformers->hit_count*sizeof(transformer*));
		if(pTransformer == NULL){
			gl_error("Failed to allocate fuse array.");
			return TS_NEVER;
		}
		while(obj = gl_find_next(transformers,obj)){
			if(index >= transformers->hit_count){
				break;
			}
			pTransformer[index] = OBJECTDATA(obj,transformer);
			if(pTransformer[index] == NULL){
				gl_error("Unable to map object as a link object.");
				return 0;
			}

			//write the transformer
			fprintf(fn,"\t<transformer>\n");

			//write the name
			if(obj->name[0] != 0){
				fprintf(fn,"\t\t<name>%s</name>\n",obj->name);
			} else {
				fprintf(fn,"\t\t<name>NA</name>\n");
			}

			//write the id
			fprintf(fn,"\t\t<id>transformer:%d</id>\n",obj->id);

			//write the from name
			if(pTransformer[index]->from->name[0] != 0){
				fprintf(fn,"\t\t<from_node>%s:%s</from_node>\n",pTransformer[index]->from->oclass->name,pTransformer[index]->from->name);
			} else {
				fprintf(fn,"\t\t<from_node>%s:%d</from_node>\n",pTransformer[index]->from->oclass->name,pTransformer[index]->from->id);
			}
			

			//write the to name
			if(pTransformer[index]->to->name[0] != 0){
				fprintf(fn,"\t\t<to_node>%s:%s</to_node>\n",pTransformer[index]->to->oclass->name,pTransformer[index]->to->name);
			} else {
				fprintf(fn,"\t\t<to_node>%s:%d</to_node>\n",pTransformer[index]->to->oclass->name,pTransformer[index]->to->id);
			}

			//write the from node's voltage
			
				if(pTransformer[index]->has_phase(PHASE_A)){
					node_voltage = get_complex(pTransformer[index]->from,"voltage_A");
				} else if(pTransformer[index]->has_phase(PHASE_B)){
					node_voltage = get_complex(pTransformer[index]->from,"voltage_B");
				} else if(pTransformer[index]->has_phase(PHASE_C)){
					node_voltage = get_complex(pTransformer[index]->from,"voltage_C");
				}
			

			if(node_voltage == NULL){
				gl_error("From node %s has no voltage.",pTransformer[index]->from->name);
				return 0;
			}

			fprintf(fn,"\t\t<from_voltage>%f</from_voltage>\n",node_voltage->Mag());

			//write the to node's 
			if(pTransformer[index]->SpecialLnk == SPLITPHASE) {
				node_voltage = get_complex(pTransformer[index]->to,"voltage_12");
			} else {
				if(pTransformer[index]->has_phase(PHASE_A)){
					node_voltage = get_complex(pTransformer[index]->to,"voltage_A");
				} else if(pTransformer[index]->has_phase(PHASE_B)){
					node_voltage = get_complex(pTransformer[index]->to,"voltage_B");
				} else if(pTransformer[index]->has_phase(PHASE_C)){
					node_voltage = get_complex(pTransformer[index]->to,"voltage_C");
				}
			}

			if(node_voltage == NULL){
				gl_error("To node %s has no voltage.",pTransformer[index]->to->name);
				return 0;
			}

			fprintf(fn,"\t\t<to_voltage>%f</to_voltage>\n",node_voltage->Mag());

			//write the transformer configuration
			//xfmrconfig=gl_get_property(pTransformer[index]->config,"connect_type");
			fprintf(fn,"\t\t<xfmr_config>%u</xfmr_config>\n",pTransformer[index]->config->connect_type);
			
			//write the phases
			if(pTransformer[index]->phases == 0x0001){//A
				fprintf(fn,"\t\t<phases>A</phases>\n");
			}
			if(pTransformer[index]->phases == 0x0002){//B
				fprintf(fn,"\t\t<phases>B</phases>\n");
			}
			if(pTransformer[index]->phases == 0x0004){//C
				fprintf(fn,"\t\t<phases>C</phases>\n");
			}
			if(pTransformer[index]->phases == 0x0009){//AN
				fprintf(fn,"\t\t<phases>AN</phases>\n");
			}
			if(pTransformer[index]->phases == 0x000a){//BN
				fprintf(fn,"\t\t<phases>BN</phases>\n");
			}
			if(pTransformer[index]->phases == 0x000c){//CN
				fprintf(fn,"\t\t<phases>CN</phases>\n");
			}
			if(pTransformer[index]->phases == 0x0071){//AS
				fprintf(fn,"\t\t<phases>AS</phases>\n");
			}
			if(pTransformer[index]->phases == 0x0072){//BS
				fprintf(fn,"\t\t<phases>BS</phases>\n");
			}
			if(pTransformer[index]->phases == 0x0074){//CS
				fprintf(fn,"\t\t<phases>CS</phases>\n");
			}
			if(pTransformer[index]->phases == 0x0003){//AB
				fprintf(fn,"\t\t<phases>AB</phases>\n");
			}
			if(pTransformer[index]->phases == 0x0006){//BC
				fprintf(fn,"\t\t<phases>BC</phases>\n");
			}
			if(pTransformer[index]->phases == 0x0005){//AC
				fprintf(fn,"\t\t<phases>AC</phases>\n");
			}
			if(pTransformer[index]->phases == 0x000b){//ABN
				fprintf(fn,"\t\t<phases>ABN</phases>\n");
			}
			if(pTransformer[index]->phases == 0x000e){//BCN
				fprintf(fn,"\t\t<phases>BCN</phases>\n");
			}
			if(pTransformer[index]->phases == 0x000d){//ACN
				fprintf(fn,"\t\t<phases>ACN</phases>\n");
			}
			if(pTransformer[index]->phases == 0x0007){//ABC
				fprintf(fn,"\t\t<phases>ABC</phases>\n");
			}
			if(pTransformer[index]->phases == 0x000f){//ABCN
				fprintf(fn,"\t\t<phases>ABCN</phases>\n");
			}
			if(pTransformer[index]->phases == 0x0107){//ABCD
				fprintf(fn,"\t\t<phases>ABCD</phases>\n");
			}

			//write power rating
			if(pTransformer[index]->config->kVA_rating!=NULL){
				fprintf(fn,"\t\t<power_rating>%.6f</power_rating>\n",pTransformer[index]->config->kVA_rating);
			}


			//write impedance
			if(pTransformer[index]->config->impedance.Re()!=NULL){
				fprintf(fn,"\t\t<resistance>%.6f</resistance>\n",pTransformer[index]->config->impedance.Re());
			}
			if(pTransformer[index]->config->impedance.Im()!=NULL){
				fprintf(fn,"\t\t<reactance>%.6f</reactance>\n",pTransformer[index]->config->impedance.Im());
			}
			//write a_mat
			fprintf(fn,"\t\t<a_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<a%d%d>%+.15f%+.15fj</a%d%d>\n",x+1,y+1,pTransformer[index]->a_mat[x][y].Re(),pTransformer[index]->a_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</a_matrix>\n");

			//write b_mat
			fprintf(fn,"\t\t<b_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<b%d%d>%+.15f%+.15fj</b%d%d>\n",x+1,y+1,pTransformer[index]->b_mat[x][y].Re(),pTransformer[index]->b_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</b_matrix>\n");

			//write c_mat
			fprintf(fn,"\t\t<c_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<c%d%d>%+.15f%+.15fj</c%d%d>\n",x+1,y+1,pTransformer[index]->c_mat[x][y].Re(),pTransformer[index]->c_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</c_matrix>\n");

			//write d_mat
			fprintf(fn,"\t\t<d_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<d%d%d>%+.15f%+.15fj</d%d%d>\n",x+1,y+1,pTransformer[index]->d_mat[x][y].Re(),pTransformer[index]->d_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</d_matrix>\n");

			//write A_mat
			fprintf(fn,"\t\t<A_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<A%d%d>%+.15f%+.15fj</A%d%d>\n",x+1,y+1,pTransformer[index]->A_mat[x][y].Re(),pTransformer[index]->A_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</A_matrix>\n");

			//write B_mat
			fprintf(fn,"\t\t<B_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<B%d%d>%+.15f%+.15fj</B%d%d>\n",x+1,y+1,pTransformer[index]->B_mat[x][y].Re(),pTransformer[index]->B_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</B_matrix>\n");

			fprintf(fn,"\t</transformer>\n");
			
			++index;
		}
	}
	
	index = 0;
	//write the triplex_lines
	if(tplines != NULL){
		pTpLine = (line **)gl_malloc(tplines->hit_count*sizeof(line*));
		if(pTpLine == NULL){
			gl_error("Failed to allocate fuse array.");
			return TS_NEVER;
		}
		while(obj = gl_find_next(tplines,obj)){
			if(index >= tplines->hit_count){
				break;
			}
			pTpLine[index] = OBJECTDATA(obj,line);
			if(pTpLine[index] == NULL){
				gl_error("Unable to map object as overhead_line object.");
				return 0;
			}

			//write the triplex_line
			fprintf(fn,"\t<triplex_line>\n");

			//write the name
			if(obj->name[0] != 0){
				fprintf(fn,"\t\t<name>%s</name>\n",obj->name);
			} else {
				fprintf(fn,"\t\t<name>NA</name>\n");
			}

			//write the id
			fprintf(fn,"\t\t<id>triplex_line:%d</id>\n",obj->id);

			//write the from name
			if(pTpLine[index]->from->name[0] != 0){
				fprintf(fn,"\t\t<from_node>%s:%s</from_node>\n",pTpLine[index]->from->oclass->name,pTpLine[index]->from->name);
			} else {
				fprintf(fn,"\t\t<from_node>%s:%d</from_node>\n",pTpLine[index]->from->oclass->name,pTpLine[index]->from->id);
			}

			//write the to name
			if(pTpLine[index]->to->name[0] != 0){
				fprintf(fn,"\t\t<to_node>%s:%s</to_node>\n",pTpLine[index]->to->oclass->name,pTpLine[index]->to->name);
			} else {
				fprintf(fn,"\t\t<to_node>%s:%d</to_node>\n",pTpLine[index]->to->oclass->name,pTpLine[index]->to->id);
			}

			//write the from node's voltage
			fprintf(fn,"\t\t<from_voltage>%f</from_voltage>\n",120);

			//write the to node's voltage
			fprintf(fn,"\t\t<to_voltage>%f</to_voltage>\n",120);

			//write the phases
			if(pTpLine[index]->phases == 0x0001){//A
				fprintf(fn,"\t\t<phases>A</phases>\n");
			}
			if(pTpLine[index]->phases == 0x0002){//B
				fprintf(fn,"\t\t<phases>B</phases>\n");
			}
			if(pTpLine[index]->phases == 0x0004){//C
				fprintf(fn,"\t\t<phases>C</phases>\n");
			}
			if(pTpLine[index]->phases == 0x0009){//AN
				fprintf(fn,"\t\t<phases>AN</phases>\n");
			}
			if(pTpLine[index]->phases == 0x000a){//BN
				fprintf(fn,"\t\t<phases>BN</phases>\n");
			}
			if(pTpLine[index]->phases == 0x000c){//CN
				fprintf(fn,"\t\t<phases>CN</phases>\n");
			}
			if(pTpLine[index]->phases == 0x0071){//AS
				fprintf(fn,"\t\t<phases>AS</phases>\n");
			}
			if(pTpLine[index]->phases == 0x0072){//BS
				fprintf(fn,"\t\t<phases>BS</phases>\n");
			}
			if(pTpLine[index]->phases == 0x0074){//CS
				fprintf(fn,"\t\t<phases>CS</phases>\n");
			}
			if(pTpLine[index]->phases == 0x0003){//AB
				fprintf(fn,"\t\t<phases>AB</phases>\n");
			}
			if(pTpLine[index]->phases == 0x0006){//BC
				fprintf(fn,"\t\t<phases>BC</phases>\n");
			}
			if(pTpLine[index]->phases == 0x0005){//AC
				fprintf(fn,"\t\t<phases>AC</phases>\n");
			}
			if(pTpLine[index]->phases == 0x000b){//ABN
				fprintf(fn,"\t\t<phases>ABN</phases>\n");
			}
			if(pTpLine[index]->phases == 0x000e){//BCN
				fprintf(fn,"\t\t<phases>BCN</phases>\n");
			}
			if(pTpLine[index]->phases == 0x000d){//ACN
				fprintf(fn,"\t\t<phases>ACN</phases>\n");
			}
			if(pTpLine[index]->phases == 0x0007){//ABC
				fprintf(fn,"\t\t<phases>ABC</phases>\n");
			}
			if(pTpLine[index]->phases == 0x000f){//ABCN
				fprintf(fn,"\t\t<phases>ABCN</phases>\n");
			}
			if(pTpLine[index]->phases == 0x0107){//ABCD
				fprintf(fn,"\t\t<phases>ABCD</phases>\n");
			}

			//write the length
			fprintf(fn,"\t\t<length>%f</length>\n",pTpLine[index]->length);

			//write a_mat
			fprintf(fn,"\t\t<a_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<a%d%d>%+.15f%+.15fj</a%d%d>\n",x+1,y+1,pTpLine[index]->a_mat[x][y].Re(),pTpLine[index]->a_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</a_matrix>\n");

			//write b_mat
			fprintf(fn,"\t\t<b_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<b%d%d>%+.15f%+.15fj</b%d%d>\n",x+1,y+1,pTpLine[index]->b_mat[x][y].Re(),pTpLine[index]->b_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</b_matrix>\n");

			//write c_mat
			fprintf(fn,"\t\t<c_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<c%d%d>%+.15f%+.15fj</c%d%d>\n",x+1,y+1,pTpLine[index]->c_mat[x][y].Re(),pTpLine[index]->c_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</c_matrix>\n");

			//write d_mat
			fprintf(fn,"\t\t<d_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<d%d%d>%+.15f%+.15fj</d%d%d>\n",x+1,y+1,pTpLine[index]->d_mat[x][y].Re(),pTpLine[index]->d_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</d_matrix>\n");

			//write A_mat
			fprintf(fn,"\t\t<A_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<A%d%d>%+.15f%+.15fj</A%d%d>\n",x+1,y+1,pTpLine[index]->A_mat[x][y].Re(),pTpLine[index]->A_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</A_matrix>\n");

			//write B_mat
			fprintf(fn,"\t\t<B_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<B%d%d>%+.15f%+.15fj</B%d%d>\n",x+1,y+1,pTpLine[index]->B_mat[x][y].Re(),pTpLine[index]->B_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</B_matrix>\n");

			fprintf(fn,"\t</triplex_line>\n");
			
			++index;
		}
	}
	
	index = 0;
	//write the underground_lines
	if(uglines != NULL){
		pUgLine = (line **)gl_malloc(uglines->hit_count*sizeof(line*));
		if(pUgLine == NULL){
			gl_error("Failed to allocate fuse array.");
			return TS_NEVER;
		}
		while(obj = gl_find_next(uglines,obj)){
			if(index >= uglines->hit_count){
				break;
			}
			pUgLine[index] = OBJECTDATA(obj,line);
			if(pUgLine[index] == NULL){
				gl_error("Unable to map object as overhead_line object.");
				return 0;
			}

			//write the underground_line
			fprintf(fn,"\t<underground_line>\n");

			//write the name
			if(obj->name[0] != 0){
				fprintf(fn,"\t\t<name>%s</name>\n",obj->name);
			} else {
				fprintf(fn,"\t\t<name>NA</name>\n");
			}

			//write the id
			fprintf(fn,"\t\t<id>underground_line:%d</id>\n",obj->id);

			//write the from name
			if(pUgLine[index]->from->name[0] != 0){
				fprintf(fn,"\t\t<from_node>%s:%s</from_node>\n",pUgLine[index]->from->oclass->name,pUgLine[index]->from->name);
			} else {
				fprintf(fn,"\t\t<from_node>%s:%d</from_node>\n",pUgLine[index]->from->oclass->name,pUgLine[index]->from->id);
			}

			//write the to name
			if(pUgLine[index]->to->name[0] != 0){
				fprintf(fn,"\t\t<to_node>%s:%s</to_node>\n",pUgLine[index]->to->oclass->name,pUgLine[index]->to->name);
			} else {
				fprintf(fn,"\t\t<to_node>%s:%d</to_node>\n",pUgLine[index]->to->oclass->name,pUgLine[index]->to->id);
			}

			//write the from node's voltage
			if(pUgLine[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pUgLine[index]->from,"voltage_A");
			} else if(pUgLine[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pUgLine[index]->from,"voltage_B");
			} else if(pUgLine[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pUgLine[index]->from,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<from_voltage>%f</from_voltage>\n",node_voltage->Mag());

			//write the to node's voltage
			if(pUgLine[index]->has_phase(PHASE_A)){
				node_voltage = get_complex(pUgLine[index]->to,"voltage_A");
			} else if(pUgLine[index]->has_phase(PHASE_B)){
				node_voltage = get_complex(pUgLine[index]->to,"voltage_B");
			} else if(pUgLine[index]->has_phase(PHASE_C)){
				node_voltage = get_complex(pUgLine[index]->to,"voltage_C");
			}

			if(node_voltage == NULL){
				gl_error("From node has no voltage.");
				return 0;
			}

			fprintf(fn,"\t\t<to_voltage>%f</to_voltage>\n",node_voltage->Mag());

			//write the phases
			if(pUgLine[index]->phases == 0x0001){//A
				fprintf(fn,"\t\t<phases>A</phases>\n");
			}
			if(pUgLine[index]->phases == 0x0002){//B
				fprintf(fn,"\t\t<phases>B</phases>\n");
			}
			if(pUgLine[index]->phases == 0x0004){//C
				fprintf(fn,"\t\t<phases>C</phases>\n");
			}
			if(pUgLine[index]->phases == 0x0009){//AN
				fprintf(fn,"\t\t<phases>AN</phases>\n");
			}
			if(pUgLine[index]->phases == 0x000a){//BN
				fprintf(fn,"\t\t<phases>BN</phases>\n");
			}
			if(pUgLine[index]->phases == 0x000c){//CN
				fprintf(fn,"\t\t<phases>CN</phases>\n");
			}
			if(pUgLine[index]->phases == 0x0071){//AS
				fprintf(fn,"\t\t<phases>AS</phases>\n");
			}
			if(pUgLine[index]->phases == 0x0072){//BS
				fprintf(fn,"\t\t<phases>BS</phases>\n");
			}
			if(pUgLine[index]->phases == 0x0074){//CS
				fprintf(fn,"\t\t<phases>CS</phases>\n");
			}
			if(pUgLine[index]->phases == 0x0003){//AB
				fprintf(fn,"\t\t<phases>AB</phases>\n");
			}
			if(pUgLine[index]->phases == 0x0006){//BC
				fprintf(fn,"\t\t<phases>BC</phases>\n");
			}
			if(pUgLine[index]->phases == 0x0005){//AC
				fprintf(fn,"\t\t<phases>AC</phases>\n");
			}
			if(pUgLine[index]->phases == 0x000b){//ABN
				fprintf(fn,"\t\t<phases>ABN</phases>\n");
			}
			if(pUgLine[index]->phases == 0x000e){//BCN
				fprintf(fn,"\t\t<phases>BCN</phases>\n");
			}
			if(pUgLine[index]->phases == 0x000d){//ACN
				fprintf(fn,"\t\t<phases>ACN</phases>\n");
			}
			if(pUgLine[index]->phases == 0x0007){//ABC
				fprintf(fn,"\t\t<phases>ABC</phases>\n");
			}
			if(pUgLine[index]->phases == 0x000f){//ABCN
				fprintf(fn,"\t\t<phases>ABCN</phases>\n");
			}
			if(pUgLine[index]->phases == 0x0107){//ABCD
				fprintf(fn,"\t\t<phases>ABCD</phases>\n");
			}

			//write the length
			fprintf(fn,"\t\t<length>%f</length>\n",pUgLine[index]->length);
			
			//write a_mat
			fprintf(fn,"\t\t<a_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<a%d%d>%+.15f%+.15fj</a%d%d>\n",x+1,y+1,pUgLine[index]->a_mat[x][y].Re(),pUgLine[index]->a_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</a_matrix>\n");

			//write b_mat
			fprintf(fn,"\t\t<b_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<b%d%d>%+.15f%+.15fj</b%d%d>\n",x+1,y+1,pUgLine[index]->b_mat[x][y].Re(),pUgLine[index]->b_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</b_matrix>\n");

			//write c_mat
			fprintf(fn,"\t\t<c_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<c%d%d>%+.15f%+.15fj</c%d%d>\n",x+1,y+1,pUgLine[index]->c_mat[x][y].Re(),pUgLine[index]->c_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</c_matrix>\n");

			//write d_mat
			fprintf(fn,"\t\t<d_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<d%d%d>%+.15f%+.15fj</d%d%d>\n",x+1,y+1,pUgLine[index]->d_mat[x][y].Re(),pUgLine[index]->d_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</d_matrix>\n");

			//write A_mat
			fprintf(fn,"\t\t<A_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<A%d%d>%+.15f%+.15fj</A%d%d>\n",x+1,y+1,pUgLine[index]->A_mat[x][y].Re(),pUgLine[index]->A_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</A_matrix>\n");

			//write B_mat
			fprintf(fn,"\t\t<B_matrix>\n");
			for(x = 0; x < 3; x++){
				for(y = 0; y < 3; y++){
					fprintf(fn,"\t\t\t<B%d%d>%+.15f%+.15fj</B%d%d>\n",x+1,y+1,pUgLine[index]->B_mat[x][y].Re(),pUgLine[index]->B_mat[x][y].Im(),x+1,y+1);
				}
			}
			fprintf(fn,"\t\t</B_matrix>\n");

			fprintf(fn,"\t</underground_line>\n");
			
			++index;
		}
	}

	index=0;
	//write capacitor
	if(capacitors != NULL){
		pCapacitor = (capacitor **)gl_malloc(capacitors->hit_count*sizeof(capacitor*));
		if(pCapacitor == NULL){
			gl_error("Failed to allocate fuse array.");
			return TS_NEVER;
		}
		while(obj = gl_find_next(capacitors,obj)){
			if(index >= capacitors->hit_count){
				break;
			}
			pCapacitor[index] = OBJECTDATA(obj,capacitor);
			if(pCapacitor[index] == NULL){
				gl_error("Unable to map object as a link object.");
				return 0;
			}

			//write the capacitor
			fprintf(fn,"\t<capacitor>\n");

			//write the name
			if(obj->name[0] != 0){
				fprintf(fn,"\t\t<name>%s</name>\n",obj->name);
			} else {
				fprintf(fn,"\t\t<name>NA</name>\n");
			}

			//write the id
			fprintf(fn,"\t\t<id>cap:%d</id>\n",obj->id);


			//write the bus name
			if(obj->parent->name[0] != 0){
				fprintf(fn,"\t\t<node>%s:%s</node>\n",pCapacitor[index]->pclass->name,obj->parent->name);
			} else {
				fprintf(fn,"\t\t<node>%s:%d</node>\n",pCapacitor[index]->pclass->name,obj->parent->id);
			}
			

			//write the phases
			if(pCapacitor[index]->phases == 0x0001){//A
				fprintf(fn,"\t\t<phases>A</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x0002){//B
				fprintf(fn,"\t\t<phases>B</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x0004){//C
				fprintf(fn,"\t\t<phases>C</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x0009){//AN
				fprintf(fn,"\t\t<phases>AN</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x000a){//BN
				fprintf(fn,"\t\t<phases>BN</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x000c){//CN
				fprintf(fn,"\t\t<phases>CN</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x0071){//AS
				fprintf(fn,"\t\t<phases>AS</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x0072){//BS
				fprintf(fn,"\t\t<phases>BS</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x0074){//CS
				fprintf(fn,"\t\t<phases>CS</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x0003){//AB
				fprintf(fn,"\t\t<phases>AB</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x0006){//BC
				fprintf(fn,"\t\t<phases>BC</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x0005){//AC
				fprintf(fn,"\t\t<phases>AC</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x000b){//ABN
				fprintf(fn,"\t\t<phases>ABN</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x000e){//BCN
				fprintf(fn,"\t\t<phases>BCN</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x000d){//ACN
				fprintf(fn,"\t\t<phases>ACN</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x0007){//ABC
				fprintf(fn,"\t\t<phases>ABC</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x000f){//ABCN
				fprintf(fn,"\t\t<phases>ABCN</phases>\n");
			}
			if(pCapacitor[index]->phases == 0x0107){//ABCD
				fprintf(fn,"\t\t<phases>ABCD</phases>\n");
			}

			fprintf(fn,"\t\t<QA>%f</QA>\n",pCapacitor[index]->capacitor_A);
			fprintf(fn,"\t\t<QB>%f</QB>\n",pCapacitor[index]->capacitor_B);
			fprintf(fn,"\t\t<QC>%f</QC>\n",pCapacitor[index]->capacitor_C);

			fprintf(fn,"\t</capacitor>\n");
			
			++index;
		}
	}
	
	fprintf(fn,"</gridlabd>");
	//close file
	fclose(fn);
	return 1;
}

TIMESTAMP impedance_dump::commit(TIMESTAMP t){
	if(runtime == 0){
		runtime = t;
	}
	if((t == runtime || runtime == TS_NEVER) && (runcount < 1)){
		/* dump */
		int rv = dump(t);
		++runcount;
		if(rv == 0){
			return TS_INVALID;
		}
		return TS_NEVER;
	} else {
		if(t < runtime){
			return runtime;
		} else {
			return TS_NEVER;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: impedance_dump
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_impedance_dump(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(impedance_dump::oclass);
		if (*obj!=NULL)
		{
			impedance_dump *my = OBJECTDATA(*obj,impedance_dump);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (const char *msg)
	{
		gl_error("create_impedance_dump: %s", msg);
	}
	return 0;
}

EXPORT int init_impedance_dump(OBJECT *obj)
{
	impedance_dump *my = OBJECTDATA(obj,impedance_dump);
	try {
		return my->init(obj->parent);
	}
	catch (const char *msg)
	{
		gl_error("%s (impedance_dump:%d): %s", obj->name, obj->id, msg);
		return 0; 
	}
}

EXPORT TIMESTAMP sync_impedance_dump(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	try
	{
		impedance_dump *my = OBJECTDATA(obj,impedance_dump);
		TIMESTAMP rv;
		obj->clock = t1;
		rv = my->runtime > t1 ? my->runtime : TS_NEVER;
		return rv;
	}
	SYNC_CATCHALL(impedance_dump);
}

EXPORT TIMESTAMP commit_impedance_dump(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2){
	try {
		impedance_dump *my = OBJECTDATA(obj,impedance_dump);
		return my->commit(t1);
	} 
	I_CATCHALL(commit,impedance_dump);
}

EXPORT int isa_impedance_dump(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,impedance_dump)->isa(classname);
}

/**@}*/
