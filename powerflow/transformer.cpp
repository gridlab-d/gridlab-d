/** $Id: transformer.cpp 1211 2009-01-17 00:45:29Z d3x593 $
	Copyright (C) 2008 Battelle Memorial Institute
	@file transformer.cpp
	@addtogroup powerflow_transformer Transformer
	@ingroup powerflow

	The transformer is one of the more complex objects in the powerflow network.
	Implemented as a link, the transformer configuration exports a type property
	that allows the transformer to operate as and single phase transformer, a
	wye-wye connected transformer, a delta-grounded wye tranformer, a delta-delta
	transformer, and as a center-tapped transformer.
	
	The transformer exports a phase property that is a set of phases and may be 
	set using the bitwise or operator (A|B|C for a 3 phase line).
 @{
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>
using namespace std;

#include "transformer.h"

CLASS* transformer::oclass = NULL;
CLASS* transformer::pclass = NULL;

//Default temperature for thermal aging calculations
double default_outdoor_temperature = 74;

transformer::transformer(MODULE *mod) : link_object(mod)
{
	if(oclass == NULL)
	{
		pclass = link_object::oclass;
		
		oclass = gl_register_class(mod,"transformer",sizeof(transformer),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class transformer";
		else
			oclass->trl = TRL_PROVEN;

        if(gl_publish_variable(oclass,
			PT_INHERIT, "link",
           	PT_object, "configuration", PADDR(configuration),PT_DESCRIPTION,"Configuration library used for transformer setup",
			PT_object, "climate", PADDR(climate),PT_DESCRIPTION,"climate object used to describe thermal model ambient temperature",
			PT_double, "ambient_temperature[degC]", PADDR(amb_temp),PT_DESCRIPTION,"ambient temperature in degrees C",
			PT_double, "top_oil_hot_spot_temperature[degC]", PADDR(theta_TO),PT_DESCRIPTION,"top-oil hottest-spot temperature, degrees C",
			PT_double, "winding_hot_spot_temperature[degC]", PADDR(theta_H),PT_DESCRIPTION,"winding hottest-spot temperature, degrees C",
			PT_double, "percent_loss_of_life", PADDR(life_loss),PT_DESCRIPTION,"the percent loss of life",
			PT_double, "aging_constant", PADDR(B_age),PT_DESCRIPTION,"the aging rate slope for the transformer insulation",
			PT_bool, "use_thermal_model", PADDR(use_thermal_model),PT_DESCRIPTION,"boolean to enable use of thermal model",
			PT_double, "transformer_replacement_count", PADDR(transformer_replacements), PT_DESCRIPTION,"counter of the number times the transformer has been replaced due to lifetime failure",
			PT_double, "aging_granularity[s]", PADDR(aging_step),PT_DESCRIPTION,"maximum timestep before updating thermal and aging model in seconds",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

			if (gl_publish_function(oclass,"power_calculation",(FUNCTIONADDR)power_calculation)==NULL)
					GL_THROW("Unable to publish fuse state change function");

			//Publish deltamode functions
			if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_link)==NULL)
				GL_THROW("Unable to publish transformer deltamode function");
    }
}

int transformer::isa(char *classname)
{
	return strcmp(classname,"transformer")==0 || link_object::isa(classname);
}

int transformer::create()
{
	int result = link_object::create();
	configuration = NULL;
	ptheta_A = NULL;
	transformer_replacements = 0;
	return result;
}

void transformer::fetch_double(double **prop, char *name, OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	*prop = gl_get_double_by_name(parent, name);
	if(*prop == NULL){
		char tname[32];
		char *namestr = (hdr->name ? hdr->name : tname);
		char msg[256];
		sprintf(tname, "transformer:%i", hdr->id);
		if(*name == NULL)
			sprintf(msg, "%s: transformer unable to find property: name is NULL", namestr);
		else
			sprintf(msg, "%s: transformer unable to find %s", namestr, name);
		throw(msg);
	}
}

int transformer::init(OBJECT *parent)
{
	if (!configuration)
		GL_THROW("no transformer configuration specified.");
		/*  TROUBLESHOOT
		A transformer configuration was not provided.  Please use object transformer_configuration
		and define the necessary parameters of your transformer to continue.
		*/
	if (!gl_object_isa(configuration, "transformer_configuration"))
		GL_THROW("invalid transformer configuration");
		/*  TROUBLESHOOT
		An invalid transformer configuration was provided.  Ensure you have proper values in each field
		of the transformer_configuration object and that you haven't inadvertantly used a line configuration
		as the transformer configuration.
		*/
	if((configuration->flags & OF_INIT) != OF_INIT){
		char objname[256];
		gl_verbose("transformer::init(): deferring initialization on %s", gl_name(configuration, objname, 255));
		return 2; // defer
	}
	double V_base,za_basehi,za_baselo,V_basehi;
	double sa_base;
	double nt, nt_a, nt_b, nt_c, inv_nt_a, inv_nt_b, inv_nt_c;
	complex zt, zt_a, zt_b, zt_c, z0, z1, z2, zc;
	FINDLIST *climate_list = NULL;

	config = OBJECTDATA(configuration,transformer_configuration);

	if (config->connect_type==2)		//Flag delta-delta for power calculations
		SpecialLnk = DELTADELTA;
	else if (config->connect_type==3)	//Flag Delta-Gwye and Split-phase for phase checks
		SpecialLnk = DELTAGWYE;
	else if (config->connect_type==5)	//Flag Delta-Gwye and Split-phase for phase checks
		SpecialLnk = SPLITPHASE;
	else								//Wye-wye or single phase, but flag anyways
		SpecialLnk = WYEWYE;

	//Populate limits - emergency and continuous are the same - moved above the init so the internal check works
	link_rating[0] = config->kVA_rating;
	link_rating[1] = config->kVA_rating;

	link_object::init(parent);
	OBJECT *obj = OBJECTHDR(this);

	V_base = config->V_secondary;
	voltage_ratio = nt = config->V_primary / config->V_secondary;
	zt = (config->impedance * V_base * V_base) / (config->kVA_rating * 1000.0);
	zc =  complex(V_base * V_base,0) / (config->kVA_rating * 1000.0) * complex(config->shunt_impedance.Re(),0) * complex(0,config->shunt_impedance.Im()) / complex(config->shunt_impedance.Re(),config->shunt_impedance.Im());

	for (int i = 0; i < 3; i++) 
	{
		for (int j = 0; j < 3; j++) 
			a_mat[i][j] = b_mat[i][j] = c_mat[i][j] = d_mat[i][j] = A_mat[i][j] = B_mat[i][j] = 0.0;
	}

	switch (config->connect_type) {
		case transformer_configuration::WYE_WYE: 
		case transformer_configuration::SINGLE_PHASE:
			if (has_phase(PHASE_A)) 
			{
				nt_a = nt;
				zt_a = zt * nt_a;
				inv_nt_a = 1 / nt_a;
				c_mat[0][0] = complex(1,0) / ( complex(nt_a,0) * zc);
			} 
			else 
			{
				nt_a = inv_nt_a = 0.0;
				zt_a = complex(0,0);
			}

			if (has_phase(PHASE_B)) 
			{
				nt_b = nt;
				zt_b = zt * nt_b;
				inv_nt_b = 1 / nt_b;
				c_mat[1][1] = complex(1,0) / ( complex(nt_b,0) * zc);
			} 
			else 
			{
				nt_b = inv_nt_b = 0.0;
				zt_b = complex(0,0);
			}

			if (has_phase(PHASE_C)) 
			{
				nt_c = nt;
				zt_c = zt * nt_c;
				inv_nt_c = 1 / nt_c;
				c_mat[2][2] = complex(1,0) / ( complex(nt_c,0) * zc);
			} 
			else 
			{
				nt_c = inv_nt_c = 0.0;
				zt_c = complex(0,0);
			}
			
			if (solver_method==SM_FBS)
			{
				if (has_phase(PHASE_A)) 
				{
					A_mat[0][0] = zc / ((zt + zc) * complex(nt,0));//1/nt_a;
					a_mat[0][0] = complex(nt,0) * (zt + zc)/zc;//nt_a;
					b_mat[0][0] = complex(nt_a,0) * zt_a;
					d_mat[0][0] = 1/nt;//(zc + zt_a) / (complex(nt_a,0) * zc);
					//A_mat[0][0] = (zc - zt_a) / ( complex(nt_a,0) * (zc + zt_a));
					//a_mat[0][0] = complex(1,0) / A_mat[0][0];
					//b_mat[0][0] = zt_a / A_mat[0][0];
					//d_mat[0][0] = (zt_a + zc) / ( complex(nt_a,0) * zc);

				}

				if (has_phase(PHASE_B))
				{
					A_mat[1][1] = zc / ((zt + zc) * complex(nt,0));//1/nt_b;
					a_mat[1][1] = complex(nt,0) * (zt + zc)/zc;//nt_b;
					b_mat[1][1] = complex(nt_b,0) * zt_b;
					d_mat[1][1] = 1/nt;//(zc + zt_b) / (complex(nt_b,0) * zc);
					//A_mat[1][1] = (zc - zt_b) / ( complex(nt_b,0) * (zc + zt_b));
					//a_mat[1][1] = complex(1,0) / A_mat[1][1];
					//b_mat[1][1] = zt_b / A_mat[1][1];
					//d_mat[1][1] = (zt_b + zc) / ( complex(nt_b,0) * zc);
				}

				if (has_phase(PHASE_C))
				{
					A_mat[2][2] = zc / ((zt + zc) * complex(nt,0));//1/nt_c;
					a_mat[2][2] = complex(nt,0) * (zt + zc)/zc;//nt_c;
					b_mat[2][2] = complex(nt_c,0) * zt_c;
					d_mat[2][2] = 1/nt;//(zc + zt_c) / (complex(nt_c,0) * zc);
					//A_mat[2][2] = (zc - zt_c) / ( complex(nt_c,0) * (zc + zt_c));
					//a_mat[2][2] = complex(1,0) / A_mat[2][2];
					//b_mat[2][2] = zt_c / A_mat[2][2];
					//d_mat[2][2] = (zt_c + zc) / ( complex(nt_c,0) * zc);
				}
			}
			else if (solver_method==SM_NR)
			{
				complex Izt = complex(1,0) / zt;
				
				//Pre-inverted
				if (has_phase(PHASE_A))
					//b_mat[0][0] = (zc - zt) / ((zc + zt) * zt);
					b_mat[0][0] = complex(1,0) / zt;//(zc - zt) / ((zc + zt) * zt);
				if (has_phase(PHASE_B))
					//b_mat[1][1] = (zc - zt) / ((zc + zt) * zt);
					b_mat[1][1] = complex(1,0) / zt;//(zc - zt) / ((zc + zt) * zt);
				if (has_phase(PHASE_C))
					//b_mat[2][2] = (zc - zt) / ((zc + zt) * zt);
					b_mat[2][2] = complex(1,0) / zt;//(zc - zt) / ((zc + zt) * zt);

				//Other matrices
				A_mat[0][1] = A_mat[0][2] = A_mat[1][0] = A_mat[1][2] = A_mat[2][0] = A_mat[2][1] = 0.0;
				a_mat[0][1] = a_mat[0][2] = a_mat[1][0] = a_mat[1][2] = a_mat[2][0] = a_mat[2][1] = 0.0;
				c_mat[0][1] = c_mat[0][2] = c_mat[1][0] = c_mat[1][2] = c_mat[2][0] = c_mat[2][1] = 0.0;

				if (has_phase(PHASE_A))
				{
					//a_mat[0][0] = ( complex(nt,0) * (zc + zt)) / (zc - zt);
					a_mat[0][0] = 0;
					d_mat[0][0] = Izt / nt / nt;
					A_mat[0][0] = complex(nt,0);//* (zc) / (zc + zt);
					c_mat[0][0] = nt;
				}
				if (has_phase(PHASE_B))
				{
					//a_mat[1][1] = ( complex(nt,0) * (zc + zt)) / (zc - zt);
					a_mat[1][1] = 0;
					d_mat[1][1] = Izt / nt / nt;
					A_mat[1][1] = complex(nt,0);// * (zc) / (zc + zt);
					c_mat[1][1] = nt;
				}
				if (has_phase(PHASE_C))
				{
					//a_mat[2][2] = ( complex(nt,0) * (zc + zt)) / (zc - zt);
					a_mat[2][2] = 0;
					d_mat[2][2] = Izt / nt / nt;
					A_mat[2][2] = complex(nt,0);//* (zc) / (zc + zt);
					c_mat[2][2] = nt;
				}


			}
			else 
			{
				GL_THROW("Unsupported solver method");
				/*  TROUBLESHOOT
				An unsupported solver type was detected.  Valid solver types are FBS
				(forward-back sweep), GS (Gauss-Seidel), and NR (Newton-Raphson).  Please use
				one of these methods and consider submitting a bug report for the solver type you tried.
				*/
			}

			B_mat[0][0] = zt*zc/(zt+zc);
			B_mat[1][1] = zt*zc/(zt+zc);
			B_mat[2][2] = zt*zc/(zt+zc);

			break;
		case transformer_configuration::DELTA_DELTA:

			if (solver_method==SM_FBS)
			{
				a_mat[0][0] = a_mat[1][1] = a_mat[2][2] = nt * 2.0 / 3.0;
				a_mat[0][1] = a_mat[0][2] = a_mat[1][0] = a_mat[1][2] = a_mat[2][0] = a_mat[2][1] = -nt / 3.0;

				b_mat[0][0] = b_mat[1][1] = zt * nt;
				b_mat[2][0] = b_mat[2][1] = zt * -nt;

				d_mat[0][0] = d_mat[1][1] = d_mat[2][2] = complex(1.0) / nt;

				A_mat[0][0] = A_mat[1][1] = A_mat[2][2] = complex(2.0) / (nt * 3.0);
				A_mat[0][1] = A_mat[0][2] = A_mat[1][0] = A_mat[1][2] = A_mat[2][0] = A_mat[2][1] = complex(-1.0) / (nt * 3.0);

				B_mat[0][0] = B_mat[1][1] = zt;
				B_mat[2][0] = B_mat[2][1] = -zt;
			}
			else if (solver_method==SM_NR)
			{
				//Calculate admittance matrix
				complex Izt = complex(1,0) / zt;

				b_mat[0][0] = b_mat[1][1] = b_mat[2][2] = Izt;

				//used for power loss calculations
				//a_mat[0][0] = a_mat[1][1] = a_mat[2][2] = nt;

				//Pre-inverted matrix for power losses
				d_mat[0][0] = Izt / nt / nt;
				d_mat[1][1] = Izt / nt / nt;
				d_mat[2][2] = Izt / nt / nt;

				//Current conversion matrix
				A_mat[0][0] = A_mat[1][1] = A_mat[2][2] = nt;

			}
			else 
			{
				GL_THROW("Unsupported solver method");
				/*  TROUBLESHOOT
				Only FBS and NR are currently supported.
				*/
				// Defined above
			}
			break;
		case transformer_configuration::DELTA_GWYE:
			
			if (solver_method==SM_FBS)
			{
				if (nt>1.0)//step down transformer
				{
					nt *= sqrt(3.0);
					
					a_mat[0][1] = a_mat[1][2] = a_mat[2][0] = -nt * 2.0 / 3.0;
					a_mat[0][2] = a_mat[1][0] = a_mat[2][1] = -nt / 3.0;

					b_mat[0][1] = b_mat[1][2] = b_mat[2][0] = zt * -nt * 2.0 / 3.0;    
					b_mat[0][2] = b_mat[1][0] = b_mat[2][1] = zt * -nt / 3.0;    
					
					d_mat[0][0] = d_mat[1][1] = d_mat[2][2] = complex(1.0) / nt;
					d_mat[0][1] = d_mat[1][2] = d_mat[2][0] = complex(-1.0) / nt;

					A_mat[0][0] = A_mat[1][1] = A_mat[2][2] = complex(1.0) / nt;
					A_mat[0][2] = A_mat[1][0] = A_mat[2][1] = complex(-1.0) / nt;

					B_mat[0][0] = B_mat[1][1] = B_mat[2][2] = zt;
				}
				else {//step up transformer
					nt *= sqrt(3.0);
					
					a_mat[0][0] = a_mat[1][1] = a_mat[2][2] = nt * 2.0 / 3.0;
					a_mat[0][1] = a_mat[1][2] = a_mat[2][0] = nt / 3.0;

					b_mat[0][0] = b_mat[1][1] = b_mat[2][2] = zt * nt * 2.0 / 3.0;    
					b_mat[0][1] = b_mat[1][2] = b_mat[2][0] = zt * nt/ 3.0;    
					
					d_mat[0][0] = d_mat[1][1] = d_mat[2][2] = complex(1.0) / nt;
					d_mat[0][2] = d_mat[1][0] = d_mat[2][1] = complex(-1.0) / nt;

					A_mat[0][0] = A_mat[1][1] = A_mat[2][2] = complex(1.0) / nt;
					A_mat[0][1] = A_mat[1][2] = A_mat[2][0] = complex(-1.0) / nt;

					B_mat[0][0] = B_mat[1][1] = B_mat[2][2] = zt;
				}
			}
			else if (solver_method==SM_NR)
			{
				complex Izt = complex(1.0,0) / zt;

				complex alphaval = voltage_ratio * sqrt(3.0);

				nt *= sqrt(3.0);	//Adjustment for other matrices

				if (voltage_ratio>1.0) //Step down
				{
					//High->low voltage change
					c_mat[0][0] = c_mat[1][1] = c_mat[2][2] = complex(1.0) / alphaval;
					c_mat[0][2] = c_mat[1][0] = c_mat[2][1] = complex(-1.0) / alphaval;
					c_mat[0][1] = c_mat[1][2] = c_mat[2][0] = 0.0;

					//Y-based impedance model
					b_mat[0][0] = b_mat[1][1] = b_mat[2][2] = Izt;
					b_mat[0][1] = b_mat[0][2] = b_mat[1][0] = 0.0;
					b_mat[1][2] = b_mat[2][0] = b_mat[2][1] = 0.0;

					//I-low to I-high change
					B_mat[0][0] = B_mat[1][1] = B_mat[2][2] = complex(1.0) / alphaval;
					B_mat[0][1] = B_mat[1][2] = B_mat[2][0] = complex(-1.0) / alphaval;
					B_mat[0][2] = B_mat[1][0] = B_mat[2][1] = 0.0;

					//Other matrices (stolen from above)
					equalm(c_mat,a_mat);
					equalm(B_mat,d_mat);

					A_mat[0][0] = A_mat[1][1] = A_mat[2][2] = complex(1.0) / nt;
					A_mat[0][2] = A_mat[1][0] = A_mat[2][1] = complex(-1.0) / nt;
				}
				else //assume step up
				{
					//Low->high voltage change
					c_mat[0][0] = c_mat[1][1] = c_mat[2][2] = complex(1.0) / alphaval;
					c_mat[0][1] = c_mat[1][2] = c_mat[2][0] = complex(-1.0) / alphaval;
					c_mat[0][2] = c_mat[1][0] = c_mat[2][1] = 0.0;

					//Impedance matrix
					b_mat[0][0] = b_mat[1][1] = b_mat[2][2] = Izt;
					b_mat[0][1] = b_mat[0][2] = b_mat[1][0] = 0.0;
					b_mat[1][2] = b_mat[2][0] = b_mat[2][1] = 0.0;

					//I-high to I-low change
					B_mat[0][0] = B_mat[1][1] = B_mat[2][2] = complex(1.0) / alphaval;
					B_mat[0][2] = B_mat[1][0] = B_mat[2][1] = complex(-1.0) / alphaval;
					B_mat[0][1] = B_mat[1][2] = B_mat[2][0] = 0.0;

					//Other matrices (stolen from above)
					equalm(c_mat,a_mat);
					equalm(B_mat,d_mat);

					A_mat[0][0] = A_mat[1][1] = A_mat[2][2] = complex(1.0) / nt;
					A_mat[0][1] = A_mat[1][2] = A_mat[2][0] = complex(-1.0) / nt;
				}
			}
			else 
			{
				GL_THROW("Unsupported solver method");
				/*  TROUBLESHOOT
				Only FBS and NR are currently supported.
				*/
				// Defined above
			}
		
			break;
		case transformer_configuration::SINGLE_PHASE_CENTER_TAPPED:
			if (solver_method==SM_FBS)
			{
				if (has_phase(PHASE_A|PHASE_B)) // delta AB
				{
					GL_THROW("delta split tap is not supported yet");
					/*  TROUBLESHOOT
					This type of transformer configuration is not supported yet.
					*/
				}
				else if (has_phase(PHASE_B|PHASE_C)) // delta AB
				{
					GL_THROW("delta split tap is not supported yet");
					/*  TROUBLESHOOT
					This type of transformer configuration is not supported yet.
					*/
				}
				else if (has_phase(PHASE_A|PHASE_C)) // delta AB
				{
					GL_THROW("delta split tap is not supported yet");
					/*  TROUBLESHOOT
					This type of transformer configuration is not supported yet.
					*/
				}
				else if (has_phase(PHASE_A)) // wye-A
				{
					V_basehi = config->V_primary;
					sa_base = config->phaseA_kVA_rating;
					if (sa_base==0)	
						GL_THROW("Split-phase tranformer:%d trying to attach to phase A not defined in the configuration",obj->id);
						/*  TROUBLESHOOT
						A single-phase, center-tapped transformer is attempting to attach to a system with phase A, while
						its phase A is undefined.  Fix the appropriate link or define a new transformer configuration with
						powerA_rating property properly defined.
						*/

					za_basehi = (V_basehi*V_basehi)/(sa_base*1000);
					za_baselo = (V_base * V_base)/(sa_base*1000);

					if (config->impedance1.Re() == 0.0 && config->impedance1.Im() == 0.0)
					{
						z0 = complex(0.5 * config->impedance.Re(),0.8*config->impedance.Im()) * complex(za_basehi,0);
						z1 = complex(config->impedance.Re(),0.4 * config->impedance.Im()) * complex(za_baselo,0);
						z2 = complex(config->impedance.Re(),0.4 * config->impedance.Im()) * complex(za_baselo,0);
					}
					else
					{
						z0 = complex(config->impedance.Re(),config->impedance.Im()) * complex(za_basehi,0);
						z1 = complex(config->impedance1.Re(),config->impedance1.Im()) * complex(za_baselo,0);
						z2 = complex(config->impedance2.Re(),config->impedance2.Im()) * complex(za_baselo,0);
					}

					zc =  complex(za_basehi,0) * complex(config->shunt_impedance.Re(),0) * complex(0,config->shunt_impedance.Im()) / complex(config->shunt_impedance.Re(),config->shunt_impedance.Im());
					zt_b = complex(0,0);
					zt_c = complex(0,0);
					
					a_mat[0][0] = a_mat[1][0] = (z0 / zc + complex(1,0))*nt;
					
					c_mat[0][0] = complex(1,0)*nt / zc;
				
					d_mat[0][0] = complex(1,0)/nt + complex(nt,0)*z1 / zc;
					d_mat[0][1] = complex(-1,0)/nt;

					A_mat[0][0] = A_mat[1][0] =  (zc / (zc + z0) ) * complex(1,0)/nt;
				}

				else if (has_phase(PHASE_B)) // wye-B
				{
					V_basehi = config->V_primary;
					sa_base = config->phaseB_kVA_rating;
					if (sa_base==0)	
						GL_THROW("Split-phase tranformer:%d trying to attach to phase B not defined in the configuration",obj->id);
						/*  TROUBLESHOOT
						A single-phase, center-tapped transformer is attempting to attach to a system with phase B, while
						its phase B is undefined.  Fix the appropriate link or define a new transformer configuration with
						powerB_rating property properly defined.
						*/

					za_basehi = (V_basehi*V_basehi)/(sa_base*1000);
					za_baselo = (V_base * V_base)/(sa_base*1000);
					
					if (config->impedance1.Re() == 0.0 && config->impedance1.Im() == 0.0)
					{
						z0 = complex(0.5 * config->impedance.Re(),0.8*config->impedance.Im()) * complex(za_basehi,0);
						z1 = complex(config->impedance.Re(),0.4 * config->impedance.Im()) * complex(za_baselo,0);
						z2 = complex(config->impedance.Re(),0.4 * config->impedance.Im()) * complex(za_baselo,0);
					}
					else
					{
						z0 = complex(config->impedance.Re(),config->impedance.Im()) * complex(za_basehi,0);
						z1 = complex(config->impedance1.Re(),config->impedance1.Im()) * complex(za_baselo,0);
						z2 = complex(config->impedance2.Re(),config->impedance2.Im()) * complex(za_baselo,0);
					}

					zc =  complex(za_basehi,0) * complex(config->shunt_impedance.Re(),0) * complex(0,config->shunt_impedance.Im()) / complex(config->shunt_impedance.Re(),config->shunt_impedance.Im());
					zt_b = complex(0,0);
					zt_c = complex(0,0);
					
					a_mat[0][1] = a_mat[1][1] = (z0 / zc + complex(1,0))*nt;
				
					c_mat[1][0] = complex(1,0)*nt / zc;

					d_mat[1][0] = complex(1,0)/nt + complex(nt,0)*z1 / zc;
					d_mat[1][1] = complex(-1,0)/nt;

					A_mat[0][1] = A_mat[1][1] = (zc / (zc + z0) ) * complex(1,0)/nt;			
				}
				else if (has_phase(PHASE_C)) // wye-C
				{
					V_basehi = config->V_primary;
					sa_base = config->phaseC_kVA_rating;
					if (sa_base==0)	
						GL_THROW("Split-phase tranformer:%d trying to attach to phase C not defined in the configuration",obj->id);
						/*  TROUBLESHOOT
						A single-phase, center-tapped transformer is attempting to attach to a system with phase C, while
						its phase C is undefined.  Fix the appropriate link or define a new transformer configuration with
						powerC_rating property properly defined.
						*/

					za_basehi = (V_basehi*V_basehi)/(sa_base*1000);
					za_baselo = (V_base * V_base)/(sa_base*1000);

					if (config->impedance1.Re() == 0.0 && config->impedance1.Im() == 0.0)
					{
						z0 = complex(0.5 * config->impedance.Re(),0.8*config->impedance.Im()) * complex(za_basehi,0);
						z1 = complex(config->impedance.Re(),0.4 * config->impedance.Im()) * complex(za_baselo,0);
						z2 = complex(config->impedance.Re(),0.4 * config->impedance.Im()) * complex(za_baselo,0);
					}
					else
					{
						z0 = complex(config->impedance.Re(),config->impedance.Im()) * complex(za_basehi,0);
						z1 = complex(config->impedance1.Re(),config->impedance1.Im()) * complex(za_baselo,0);
						z2 = complex(config->impedance2.Re(),config->impedance2.Im()) * complex(za_baselo,0);
					}

					zc =  complex(za_basehi,0) * complex(config->shunt_impedance.Re(),0) * complex(0,config->shunt_impedance.Im()) / complex(config->shunt_impedance.Re(),config->shunt_impedance.Im());
					zt_b = complex(0,0);
					zt_c = complex(0,0);
					
					a_mat[0][2] = a_mat[1][2] = (z0 / zc + complex(1,0))*nt;
				
					c_mat[2][0] = complex(1,0)*nt / zc;

					d_mat[2][0] = complex(1,0)/nt + complex(nt,0)*z1 / zc;
					d_mat[2][1] = complex(-1,0)/nt;

					A_mat[0][2] = A_mat[1][2] = (zc / (zc + z0) ) * complex(1,0)/nt; 

				}

				b_mat[0][0] = (z0 / zc + complex(1,0))*(z1*nt) + z0/nt;
				b_mat[0][1] = complex(-1,0) * (z0/nt);
				b_mat[0][2] = complex(0,0);
				b_mat[1][0] = (z0/nt);
				b_mat[1][1] = -(z0 / zc + complex(1,0))*(z2*nt) - z0/nt;
				b_mat[1][2] = complex(0,0);
				b_mat[2][0] = complex(0,0);
				b_mat[2][1] = complex(0,0);
				b_mat[2][2] = complex(0,0);

				B_mat[0][0] = (z1) + (z0*zc/((zc + z0)*nt*nt));
				B_mat[0][1] = -(z0*zc/((zc + z0)*nt*nt));
				B_mat[1][0] = (z0*zc/((zc + z0)*nt*nt));
				B_mat[1][1] = complex(-1,0) * ((z2) + (z0*zc/((zc + z0)*nt*nt)));
				B_mat[1][2] = complex(0,0);
				B_mat[2][0] = complex(0,0);
				B_mat[2][1] = complex(0,0);
				B_mat[2][2] = complex(0,0);
			}
			else if (solver_method==SM_GS)	// This doesn't work yet
			{
				GL_THROW("Gauss-Seidel Implementation of Split-Phase is not complete");
				/*  TROUBLESHOOT
				At this time, the Gauss-Seidel method does not support split-phase transformers.
				This will hopefully be a feature in future releases.
				*/
			}
			else if (solver_method==SM_NR)
			{
				V_basehi = config->V_primary;

				if (has_phase(PHASE_A))
				{
					sa_base = config->phaseA_kVA_rating;
					if (sa_base==0)	
						GL_THROW("Split-phase tranformer:%d trying to attach to phase A not defined in the configuration",obj->id);
						/*  TROUBLESHOOT
						A single-phase, center-tapped transformer is attempting to attach to a system with phase A, while
						its phase A is undefined.  Fix the appropriate link or define a new transformer configuration with
						powerA_rating property properly defined.
						*/
				}
				else if (has_phase(PHASE_B))
				{
					sa_base = config->phaseB_kVA_rating;
					if (sa_base==0)	
						GL_THROW("Split-phase tranformer:%d trying to attach to phase B not defined in the configuration",obj->id);
						/*  TROUBLESHOOT
						A single-phase, center-tapped transformer is attempting to attach to a system with phase B, while
						its phase B is undefined.  Fix the appropriate link or define a new transformer configuration with
						powerB_rating property properly defined.
						*/
				}
				else if (has_phase(PHASE_C))
				{
					sa_base = config->phaseC_kVA_rating;
					if (sa_base==0)	
						GL_THROW("Split-phase tranformer:%d trying to attach to phase C not defined in the configuration",obj->id);
						/*  TROUBLESHOOT
						A single-phase, center-tapped transformer is attempting to attach to a system with phase C, while
						its phase C is undefined.  Fix the appropriate link or define a new transformer configuration with
						powerC_rating property properly defined.
						*/
				}

				za_basehi = (V_basehi*V_basehi)/(sa_base*1000);
				za_baselo = (V_base * V_base)/(sa_base*1000);

				if (config->impedance1.Re() == 0.0 && config->impedance1.Im() == 0.0)
				{
					z0 = complex(0.5 * config->impedance.Re(),0.8*config->impedance.Im()) * complex(za_basehi,0);
					z1 = complex(config->impedance.Re(),0.4 * config->impedance.Im()) * complex(za_baselo,0);
					z2 = complex(config->impedance.Re(),0.4 * config->impedance.Im()) * complex(za_baselo,0);
				}
				else
				{
					z0 = complex(config->impedance.Re(),config->impedance.Im()) * complex(za_basehi,0);
					z1 = complex(config->impedance1.Re(),config->impedance1.Im()) * complex(za_baselo,0);
					z2 = complex(config->impedance2.Re(),config->impedance2.Im()) * complex(za_baselo,0);
				}

				
				// Scale zc so you get proper answers
				zc =  complex(za_basehi,0) * complex(config->shunt_impedance.Re(),0) * complex(0,config->shunt_impedance.Im()) / complex(config->shunt_impedance.Re(),config->shunt_impedance.Im());

				//Make intermediate variable for the nasty denominators
				complex indet;
				indet=complex(1.0)/(z1*z2*zc*nt*nt+z0*(z2*zc+z1*zc+z1*z2*nt*nt));

				//Store all information into b_mat (pull it out later) - phases handled in link
				// Yto_00	Yto_01   To_Y00
				// Yto_10	Yto_11	 To_Y10
				// From_Y00	From_Y01 YFrom_00
				//
				// Translates into			b_mat*[V1; V2; VSource] = [I1; I2; ISource]
				//      V1 V2 Vsource			
				// I1
				// I2
				// ISource

				//Put in a_mat first, it gets scaled
				//Yto
				a_mat[0][0] = -(z2*zc*nt*nt+z0*zc+z0*z2*nt*nt);	//-z2*nt*nt-z0;
				a_mat[0][1] = z0*zc;							//z0;
				a_mat[1][0] = -z0*zc;							//-z0;
				a_mat[1][1] = (z1*zc*nt*nt+z0*zc+z0*z1*nt*nt);	//z1*nt*nt+z0;

				//To_Y
				a_mat[0][2] = z2*zc*nt;		//z2*nt;
				a_mat[1][2] = -z1*zc*nt;	//-z1*nt;

				//From_Y
				a_mat[2][0] = -z2*zc*nt;	//-z2*nt;
				a_mat[2][1] = -z1*zc*nt;	//-z1*nt;

				//Yfrom
				a_mat[2][2] = (z2*zc+z1*zc+z1*z2*nt*nt);	//z1+z2;

				//Form it into b_mat
				for (char xindex=0; xindex<3; xindex++)
				{
					for (char yindex=0; yindex<3; yindex++)
					{
						b_mat[xindex][yindex]=a_mat[xindex][yindex]*indet;
						a_mat[xindex][yindex]=0.0;
					}
				}
			}	
			else 
			{
				GL_THROW("Unsupported solver method");
				/*  TROUBLESHOOT
				Only FBS and NR are currently supported.
				*/
				//defined above
			}

			break;
		default:
			throw "unknown transformer connect type";
			/*  TROUBLESHOOT
			An unknown transformer configuration was provided.  Please ensure you are using
			only the defined types of transformer.  Refer to the documentation of use the command flag
			--modhelp powerflow:transformer_configuration to see valid transformer types.
			*/
	}

#ifdef _TESTING
	extern bool show_matrix_values;
	if (show_matrix_values)
	{
		gl_testmsg("transformer:\ta matrix");
		print_matrix(a_mat);
		gl_testmsg("transformer:\tA matrix");
		print_matrix(A_mat);
		gl_testmsg("transformer:\tb matrix");
		print_matrix(b_mat);
		gl_testmsg("transformer:\tB matrix");
		print_matrix(B_mat);
		gl_testmsg("transformer:\td matrix");
		print_matrix(d_mat);
	}
#endif

	//retrieve all the thermal model inputs from transformer_config
	if(use_thermal_model && config->coolant_type!=1){
		GL_THROW("transformer:%d (%s) coolant_type specified is not handled",obj->id,obj->name);
		/*  TROUBLESHOOT
		When using the thermal aging model, the coolant type must be specifed.  Please select a supported
		coolant type and add it to your transformer configuration.  Currently, only MINERAL_OIL is supported.
		*/
	}
	if(use_thermal_model) {
		if (config->coolant_type==1){
			if(config->core_coil_weight<=0){
				GL_THROW("weight of the core and coil assembly for transformer configuration %s must be greater than zero",configuration->name);
				/*  TROUBLESHOOT
				When using the thermal aging model, the core weight must be specified.  Please specify core_coil_weight
				in your transformer configuration.
				*/
			}
			if(config->tank_fittings_weight<=0){
				GL_THROW("weight of the tank fittings for transformer configuration %s must be greater than zero",configuration->name);
				/*  TROUBLESHOOT
				When using the thermal aging model, the core weight must be specified.  Please specify tank_fittings_weight
				in your transformer configuration.
				*/
			}
			if(config->oil_vol<=0){
				GL_THROW("the oil volume for transformer configuration %s must be greater than zero",configuration->name);
				/*  TROUBLESHOOT
				When using the thermal aging model, the oil volume must be specified as a value of zero or greater.
				Please specify oil_volume in your transformer configuration.
				*/
			}

			if(config->cooling_type==1 || config->cooling_type==2){
				thermal_capacity = (0.06 * config->core_coil_weight) + (0.04 * config->tank_fittings_weight) + (1.33 * config->oil_vol);
				if(config->cooling_type==1){
					m = n = 0.8;
				} else if(config->cooling_type==2){
					m = 0.8;
					n = 0.9;
				} else {
					GL_THROW("cooling_type not specified for transformer configuration %s",configuration->name);
					/*  TROUBLESHOOT
					When using the thermal aging model, the a cooling_type must be specified.
					Please specify cooling_type in your transformer configuration.
					*/
				}
			} else if(config->cooling_type>2){
				thermal_capacity = (0.06 * config->core_coil_weight) + (0.06 * config->tank_fittings_weight) + (1.93 * config->oil_vol);
				if(config->cooling_type==3 || config->cooling_type==4){
					m = 0.8;
					n = 0.9;
				} else if(config->cooling_type==5 || config->cooling_type==6){
					m = n = 1.0;
				} else {
					GL_THROW("cooling_type not specified for transformer configuration %s",configuration->name);
					/*  TROUBLESHOOT
					When using the thermal aging model, the a cooling_type must be specified.
					Please specify cooling_type in your transformer configuration.
					*/
				}
			} else {
				GL_THROW("cooling_type not specified for transformer configuration %s",configuration->name);
				/*  TROUBLESHOOT
				When using the thermal aging model, the a cooling_type must be specified.
				Please specify cooling_type in your transformer configuration.
				*/
			}
			if(config->full_load_loss==0 && config->no_load_loss==0 && config->impedance.Re()==0 && config->shunt_impedance.Re()==0){
				GL_THROW("full-load and no-load losses for transformer configuration %s must be nonzero",configuration->name);
				/*  TROUBLESHOOT
				When using the thermal aging model, full_load_losses and no_load_losses must be specified.
				*/
			} else if(config->full_load_loss!=0 && config->no_load_loss!=0){
				R = config->full_load_loss/config->no_load_loss;
			} else if(config->impedance.Re()!=0 && config->shunt_impedance.Re()!=0)
				R = config->impedance.Re()*config->shunt_impedance.Re();
			if(config->t_W==NULL || config->dtheta_TO_R==NULL){
				GL_THROW("winding time constant or rated top-oil hotspot rise for transformer configuration %s must be nonzero",configuration->name);
				/*  TROUBLESHOOT
				When using the thermal aging model, the rated_winding_time_constant or the rated_top_oil_rise must be given as a non-zero value.
				Please specify one or the other in your transformer configuration.
				*/
			}
			if(config->t_W<=0){
				GL_THROW("%s: transformer_configuration winding time constant must be greater than zero",configuration->name);
				/*  TROUBLESHOOT
				When using the thermal aging model, the rated_winding_time_constant must be given as a greater-than-zero value.
				Please specify as a positive value in your transformer configuration.
				*/
			}

			// fetch the climate data
			if(climate==NULL){
				//See if a climate object exists
				climate_list = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);

				if (climate_list==NULL)
				{
					//Warn
					gl_warning("No climate data found - using static temperature");
					/*  TROUBLESHOOT
					While attempting to map a climate object for the transformer aging model, no climate object could be
					found.  Static temperature data will be used instead.  Please check your code or manually specify a climate
					object if this is not desired.
					*/

					//Nope, just use default static
				ptheta_A = &default_outdoor_temperature; // default static temperature
				}
				else if (climate_list->hit_count >= 1)
				{
					//Link up
					climate = gl_find_next(climate_list,NULL);

					//Make sure it worked
					if (climate==NULL)
					{
						//Warn
						gl_warning("No climate data found - using static temperature");
						/*  TROUBLESHOOT
						While attempting to map a climate object for the transformer aging model, no climate object could be
						found.  Static temperature data will be used instead.  Please check your code or manually specify a climate
						object if this is not desired.
						*/

						//Nope, just use default static
						ptheta_A = &default_outdoor_temperature; // default static temperature
					}
					else	//Found one, map the property
					{
						//Link it up
						fetch_double(&ptheta_A, "temperature", climate);

						//Make sure it worked
						if (ptheta_A == NULL)
						{
							//Warn
							gl_warning("No climate data found - using static temperature");
							/*  TROUBLESHOOT
							While attempting to map a climate object for the transformer aging model, no climate object could be
							found.  Static temperature data will be used instead.  Please check your code or manually specify a climate
							object if this is not desired.
							*/

							//Nope, just use default static
							ptheta_A = &default_outdoor_temperature; // default static temperature
						}
					}
				}
				else	//Must not have found one
				{
					//Warn
					gl_warning("No climate data found - using static temperature");
					/*  TROUBLESHOOT
					While attempting to map a climate object for the transformer aging model, no climate object could be
					found.  Static temperature data will be used instead.  Please check your code or manually specify a climate
					object if this is not desired.
					*/

					//Nope, just use default static
					ptheta_A = &default_outdoor_temperature; // default static temperature
				}

			} else {
				fetch_double(&ptheta_A, "temperature", climate);

				//Make sure it worked
				if (ptheta_A == NULL)
				{
					//Warn
					gl_warning("No climate data found - using static temperature");
					/*  TROUBLESHOOT
					While attempting to map a climate object for the transformer aging model, no climate object could be
					found.  Static temperature data will be used instead.  Please check your code or manually specify a climate
					object if this is not desired.
					*/

					//Nope, just use default static
					ptheta_A = &default_outdoor_temperature; // default static temperature
				}
			} 
			temp_A = *ptheta_A;
			amb_temp = (temp_A-32.0)*(5.0/9.0);
			if(B_age==0)
				B_age = 15000;//default value from the IEEE Std C57.91-1995
			//default rise in temperature is zero.
			if(theta_TO==0)
				theta_TO = amb_temp;
			if(theta_H==0)
				theta_H = theta_TO;
			if(config->dtheta_H_AR==0)
				config->dtheta_H_AR = 80;// default for an average rise of 65 degrees C is 80 degrees C
			
			return_at = gl_globalclock;
			if(aging_step<1)
				aging_step = 300;//default granularity is 5 minutes.
			t_TOR = thermal_capacity*(config->dtheta_TO_R)/((config->full_load_loss)*(config->kVA_rating)*1000);
			dtheta_H_R = config->dtheta_H_AR - config->dtheta_TO_R;
		}
		else {
			// shouldn't have gotten here, but for completeness
			GL_THROW("transformer:%d (%s) coolant_type specified is not handled",obj->id,obj->name);
			/*  TROUBLESHOOT
			When using the thermal aging model, the coolant type must be specifed.  Please select a supported
			coolant type and add it to your transformer configuration.  Currently, only MINERAL_OIL is supported.
			*/
		}
	}
	//grab initial start time
	simulation_start_time = gl_globalclock;

	return 1;
}

TIMESTAMP transformer::postsync(TIMESTAMP t0)
{	
	OBJECT *obj = OBJECTHDR(this);
	double time_left;
	TIMESTAMP trans_end;
	if(use_thermal_model){
		TIMESTAMP result = link_object::postsync(t0);
		temp_A = *ptheta_A;
		amb_temp = (temp_A-32.0)*(5.0/9.0);
		if((t0 >= simulation_start_time) && (t0 != time_before)){
			if(time_before != 0){
				dt = (double)((t0-time_before)*TS_SECOND)/3600;// calculate the change in time in hours

				//calculate the loss of life
				if(config->installed_insulation_life<=0) {
					gl_error("%s: transformer configuration installed insulation life must be greater than zero",configuration->name);
					/*  TROUBLESHOOT
					During the simulation, installed_insulation_life was found to be zero or negative.  Either this was specified
					incorrectly in the model - check to see that installed_insulation_life was set as a positive value in your configuration
					file - or a major error occurred in the model.  If the latter, please post on the GridLAB-D forums.
					*/
					return TS_INVALID;
				}
				life_loss += F_AA*dt*100/config->installed_insulation_life;
				if(life_loss < 100){
					//calculate the top-oil hot spot temperature using previous parameters from the previous timestep 
					dtheta_TO = (dtheta_TO_U - dtheta_TO_i)*(1-exp(-dt/t_TO))+dtheta_TO_i;
					theta_TO = last_temp + dtheta_TO;

					//calculate the winding hot spot temperature
					dtheta_H = (dtheta_H_U - dtheta_H_i)*(1-exp(-dt/config->t_W))+dtheta_H_i;
					theta_H = last_temp + dtheta_TO + dtheta_H;

					// calculate the power ratio for the current timestep, temperature asymptotes, and time constants.
					K = power_out.Mag()/(1000*config->kVA_rating);

					//calculate the inital change in hot spot temperatures.
					dtheta_TO_i = dtheta_TO;
					dtheta_H_i = dtheta_H;

					dtheta_TO_U = (config->dtheta_TO_R)*pow(((K*K*R+1)/(R+1)),n);
					t_TO = t_TOR*(((dtheta_TO_U/config->dtheta_TO_R)-(dtheta_TO_i/config->dtheta_TO_R))/(pow((dtheta_TO_U/config->dtheta_TO_R),(1/n)) - pow((dtheta_TO_i/config->dtheta_TO_R),(1/n))));
					dtheta_H_U = dtheta_H_R*pow(K,2*m);
				} else if(life_loss >= 100){
					gl_warning("%s: The transformer has reached its operational lifespan. Resetting transformer lifetime parameters.",obj->name);
					/*  TROUBLESHOOT
					During the simulation, while using the transformer thermal aging model, the transformer reached the end of its
					life.  It was assumed that the transformer was replaced with a new transformer of the same make, model, etc. to 
					prevent failure of the simulation.  All of the lifetime parameters are set to assume a "new" transformer.
					*/
					life_loss = 0;
					dtheta_TO_i = 0;
					dtheta_H_i = 0;
					theta_TO = amb_temp;
					theta_H = amb_temp;
					dtheta_TO_U = (config->dtheta_TO_R)*pow(((K*K*R+1)/(R+1)),n);
					t_TO = t_TOR*(((dtheta_TO_U/config->dtheta_TO_R)-(dtheta_TO_i/config->dtheta_TO_R))/(pow((dtheta_TO_U/config->dtheta_TO_R),(1/n)) - pow((dtheta_TO_i/config->dtheta_TO_R),(1/n))));
					dtheta_H_U = dtheta_H_R*pow(K,2*m);
					transformer_replacements += 1;
				}
				last_temp = amb_temp;

			} else {
				// calculate the power ratio for the current timestep, temperature asymptotes, and time constants.
				K = power_out.Mag()/(1000*config->kVA_rating);

				//calculate the inital change in hot spot temperatures.
				dtheta_TO_i = theta_TO - amb_temp;
				if(dtheta_TO_i  < 0){
					theta_TO = amb_temp;
					dtheta_TO_i = 0;
				}
				dtheta_H_i = theta_H - theta_TO;
				

				dtheta_TO_U = (config->dtheta_TO_R)*pow(((K*K*R+1)/(R+1)),n);
				t_TO = t_TOR*(((dtheta_TO_U/config->dtheta_TO_R)-(dtheta_TO_i/config->dtheta_TO_R))/(pow((dtheta_TO_U/config->dtheta_TO_R),(1/n)) - pow((dtheta_TO_i/config->dtheta_TO_R),(1/n))));
				dtheta_H_U = dtheta_H_R*pow(K,2*m);

				last_temp = amb_temp;
			}
			time_before = t0;
		}
		//figure out how much time is left before the transformer lifespan is reached
		F_AA = exp((B_age/383.14)-(B_age/(theta_H+273.14)));
		time_left = (config->installed_insulation_life)*(100-life_loss)/(100*F_AA)*3600;
		if(time_left < 1){
			time_left = 1;
		}
		trans_end = t0 + (TIMESTAMP)(floor(time_left+0.5));
		// figure out when to return
 		if(t0>=return_at){
			return_at += (TIMESTAMP)(floor(aging_step + 0.5));
			if(trans_end<return_at){
				return_at = trans_end;
			}
			if(return_at<t0){
				gl_error("%s: transformer granularity is too small for the minimum timestep specified",obj->name);
				/*  TROUBLESHOOT
				During the simulation, the transformer lifetime model resulted in an   If the latter, please post on the GridLAB-D forums.
				*/
				return TS_INVALID;
			}
			if(result<return_at){
				return result;
			} else {
				return return_at;
			}
		} else {
			if(trans_end<return_at){
				return_at = trans_end;
			}
			if(result<return_at){
				return result;
			} else {
				return return_at;
			}
		}
	} else {
		return link_object::postsync(t0);
	}
}
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: transformer
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/



/* This can be added back in after tape has been moved to commit
EXPORT TIMESTAMP commit_transformer(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{	
	if ((solver_method==SM_FBS) || (solver_method==SM_NR))
	{	
		transformer *plink = OBJECTDATA(obj,transformer);
		if (plink->has_phase(PHASE_S))
			plink->calculate_power_splitphase();
		else
			plink->calculate_power();
	}
	return TS_NEVER;
}
*/
EXPORT int create_transformer(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(transformer::oclass);
		if (*obj!=NULL)
		{
			transformer *my = OBJECTDATA(*obj,transformer);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(transformer);
}



/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_transformer(OBJECT *obj)
{
	try {
		transformer *my = OBJECTDATA(obj,transformer);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(transformer);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_transformer(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		transformer *pObj = OBJECTDATA(obj,transformer);
		TIMESTAMP t1 = TS_NEVER;
		switch (pass) {
		case PC_PRETOPDOWN:
			return pObj->presync(t0);
		case PC_BOTTOMUP:
			return pObj->sync(t0);
		case PC_POSTTOPDOWN:
			t1 = pObj->postsync(t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
	} 
	SYNC_CATCHALL(transformer);
}

EXPORT void power_calculation(OBJECT *thisobj)
{
	transformer *transformerobj = OBJECTDATA(thisobj,transformer);
	transformerobj->calculate_power();
}

EXPORT int isa_transformer(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,transformer)->isa(classname);
}

/**@}**/
