/** $Id: transformer.cpp 1211 2009-01-17 00:45:28Z d3x593 $
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

transformer::transformer(MODULE *mod) : link(mod)
{
	if(oclass == NULL)
	{
		pclass = link::oclass;
		
		oclass = gl_register_class(mod,"transformer",sizeof(transformer),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT);
        if(oclass == NULL)
            GL_THROW("unable to register object class implemented by %s",__FILE__);
        
        if(gl_publish_variable(oclass,
			PT_INHERIT, "link",
           	PT_object, "configuration", PADDR(configuration),
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);
    }
}

int transformer::isa(char *classname)
{
	return strcmp(classname,"transformer")==0 || link::isa(classname);
}

int transformer::create()
{
	int result = link::create();
	configuration = NULL;
	return result;
}

int transformer::init(OBJECT *parent)
{
	if (!configuration)
		throw "no transformer configuration specified.";
	if (!gl_object_isa(configuration, "transformer_configuration"))
		throw "invalid transformer configuration";

	double V_base,za_basehi,za_baselo,V_basehi;
	double sa_base;
	double nt, nt_a, nt_b, nt_c, inv_nt_a, inv_nt_b, inv_nt_c;
	complex zt, zt_a, zt_b, zt_c, z0,z1,z2;

	link::init(parent);

	transformer_configuration *config = OBJECTDATA(configuration,
	                                   transformer_configuration);

	V_base = config->V_secondary;
	voltage_ratio = nt = config->V_primary / config->V_secondary;
	zt = (config->impedance * V_base * V_base) / (config->kVA_rating * 1000.0);

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
			} 
			else 
			{
				nt_c = inv_nt_c = 0.0;
				zt_c = complex(0,0);
			}
			
			if (solver_method==SM_FBS)
			{
				b_mat[0][0] = zt_a;
				b_mat[1][1] = zt_b;
				b_mat[2][2] = zt_c;
			}
			else if (solver_method==SM_GS)
			{
				complex Izt = complex(1,0) / zt;

				b_mat[0][0] = b_mat[1][1] = b_mat[2][2] = Izt;
			}
			else
			{
				GL_THROW("Unsupported solver method");
			}

			a_mat[0][0] = nt_a;
			a_mat[1][1] = nt_b;
			a_mat[2][2] = nt_c;

			B_mat[0][0] = zt;
			B_mat[1][1] = zt;
			B_mat[2][2] = zt;

			d_mat[0][0] = A_mat[0][0] = inv_nt_a;
			d_mat[1][1] = A_mat[1][1] = inv_nt_b;
			d_mat[2][2] = A_mat[2][2] = inv_nt_c;

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
			else if (solver_method==SM_GS)
			{
				//Calculate admittance matrix
				complex Izt = complex(1,0) / zt;

				b_mat[0][0] = b_mat[1][1] = b_mat[2][2] = Izt;

				a_mat[0][0] = a_mat[1][1] = a_mat[2][2] = nt * 2.0 / 3.0;
				a_mat[0][1] = a_mat[0][2] = a_mat[1][0] = a_mat[1][2] = a_mat[2][0] = a_mat[2][1] = -nt / 3.0;

				d_mat[0][0] = d_mat[1][1] = d_mat[2][2] = complex(1.0) / nt;

				A_mat[0][0] = A_mat[1][1] = A_mat[2][2] = complex(2.0) / (nt * 3.0);
				A_mat[0][1] = A_mat[0][2] = A_mat[1][0] = A_mat[1][2] = A_mat[2][0] = A_mat[2][1] = complex(-1.0) / (nt * 3.0);

				B_mat[0][0] = B_mat[1][1] = zt;
				B_mat[2][0] = B_mat[2][1] = -zt;
			}
			else
			{
				GL_THROW("Unsupported solver method");
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
			else if (solver_method==SM_GS)
			{
				Regulator_Link = 2;



				complex Izt = complex(1.0,0) / zt;

				complex btemp_mat[3][3];
				double scaleval = 3.0 / ( voltage_ratio * voltage_ratio);

				b_mat[0][0] = b_mat[1][1] = b_mat[2][2] = Izt;
				btemp_mat[0][0] = btemp_mat[0][1] = btemp_mat[0][2] = 0.0;
				btemp_mat[1][0] = btemp_mat[1][1] = btemp_mat[1][2] = 0.0;
				btemp_mat[2][0] = btemp_mat[2][1] = btemp_mat[2][2] = 0.0;

				//c_mat[0][0] = c_mat[1][1] = c_mat[2][2] = complex(2.0) * complex(scaleval) * Izt / 3.0;
				//c_mat[0][1] = c_mat[0][2] = c_mat[1][0] = c_mat[1][2] = c_mat[2][0] = c_mat[2][1] = complex(-scaleval) * Izt / 3.0;

				//c_mat[0][0] = c_mat[1][1] = c_mat[2][2] = Izt * scaleval;

				//btemp_mat[0][0] = btemp_mat[1][1] = btemp_mat[2][2] = 0.0;
				//btemp_mat[0][1] = btemp_mat[1][2] = btemp_mat[2][0] = zt * voltage_ratio * voltage_ratio * -2.0 / 3.0 ;
				//btemp_mat[0][2] = btemp_mat[1][0] = btemp_mat[2][1] = zt * voltage_ratio * voltage_ratio * -1.0 / 3.0;
				inverse(btemp_mat,c_mat);


				btemp_mat[0][0] = btemp_mat[1][1] = btemp_mat[2][2] = 0.0;
				btemp_mat[0][1] = btemp_mat[1][2] = btemp_mat[2][0] = Izt * 2.0 / voltage_ratio / voltage_ratio / 3.0;
				btemp_mat[0][2] = btemp_mat[1][0] = btemp_mat[2][1] = Izt * -1.0 / voltage_ratio / voltage_ratio / 3.0;;
				equalm(btemp_mat,c_mat);

				//a_mat[0][0] = a_mat[1][1] = a_mat[2][2] = -Izt / sqrt(3.0) / (alpha_val*beta_val);
				//a_mat[0][2] = a_mat[1][0] = a_mat[2][1] = Izt / sqrt(3.0) / (alpha_val * beta_val);

				//b_mat[0][0] = b_mat[1][1] = b_mat[2][2] = -Izt/sqrt(3.0);
				//b_mat[0][2] = b_mat[1][0] = b_mat[2][1] = Izt/sqrt(3.0);

				//multiply(scaleval,b_mat,c_mat);

				//voltage_ratio /=sqrt(3.0);


				if (nt>1.0)
				{
					//Apply delta phase shift
					//phaseadjust = complex(sqrt(3.0),-1.0);
					//phaseadjust /=2.0;
					phaseadjust = complex(1.0, -1.0 * sqrt(3.0));
					phaseadjust /=2.0;
				}
				else
				{
					//Apply delta phase shift
					phaseadjust = complex(sqrt(3.0),+1.0);
					phaseadjust /=2.0;
				}

				//Commented out to use this matrix for transferrals
				//a_mat[0][0] = nt;
				//a_mat[1][1] = nt;
				//a_mat[2][2] = nt;

				B_mat[0][0] = zt;
				B_mat[1][1] = zt;
				B_mat[2][2] = zt;

				d_mat[0][0] = A_mat[0][0] = 1.0 / nt;
				d_mat[1][1] = A_mat[1][1] = 1.0 / nt;
				d_mat[2][2] = A_mat[2][2] = 1.0 / nt;

				//Below is non-working code
				//if (nt>1.0)
				//{
				//	//Apply delta phase shift
				//	phaseadjust = complex(sqrt(3.0),-1.0);
				//	phaseadjust /=2.0;


				//	

				//	nt *= sqrt(3.0);

				//	a_mat[0][1] = a_mat[1][2] = a_mat[2][0] = -nt * 2.0 / 3.0;
				//	a_mat[0][2] = a_mat[1][0] = a_mat[2][1] = -nt / 3.0;
				//
				//	d_mat[0][0] = d_mat[1][1] = d_mat[2][2] = complex(1.0) / nt;
				//	d_mat[0][1] = d_mat[1][2] = d_mat[2][0] = complex(-1.0) / nt;

				//	A_mat[0][0] = A_mat[1][1] = A_mat[2][2] = complex(1.0) / nt;
				//	A_mat[0][2] = A_mat[1][0] = A_mat[2][1] = complex(-1.0) / nt;

				//	B_mat[0][0] = B_mat[1][1] = B_mat[2][2] = zt;


				//}
				//else
				//{
				//	//Apply delta phase shift
				//	phaseadjust = complex(sqrt(3.0),+1.0);
				//	phaseadjust /=2.0;

				//	nt *= sqrt(3.0);
				//	
				//	a_mat[0][0] = a_mat[1][1] = a_mat[2][2] = nt * 2.0 / 3.0;
				//	a_mat[0][1] = a_mat[1][2] = a_mat[2][0] = nt / 3.0;

				//	d_mat[0][0] = d_mat[1][1] = d_mat[2][2] = complex(1.0) / nt;
				//	d_mat[0][2] = d_mat[1][0] = d_mat[2][1] = complex(-1.0) / nt;

				//	A_mat[0][0] = A_mat[1][1] = A_mat[2][2] = complex(1.0) / nt;
				//	A_mat[0][1] = A_mat[1][2] = A_mat[2][0] = complex(-1.0) / nt;

				//	B_mat[0][0] = B_mat[1][1] = B_mat[2][2] = zt;
				//}

				//Below is different attempt
				//complex Izt = complex(1,0) / zt;
				//complex Hzt = Izt / voltage_ratio / voltage_ratio;
				//complex temp = complex(3.0, sqrt(3.0));
				//temp /=2.0;

				//complex y[6][6];
				//complex t[6][3];

				//y[0][0] = y[1][1] = y[2][2] = y[3][3] = y[4][4] = y[5][5] = complex(2.0) * Izt; /// complex(3.0) * Izt;
				//y[3][0] = y[4][1] = y[5][2] = y[0][3] = y[1][4] = y[2][5] = 0.0; //complex(-2.0) / complex(3.0) * Izt;
				//
				//y[0][1] = y[0][2] = y[1][0] = y[1][2] = y[2][0] = y[2][1] = -Izt; /// 3.0;
				//y[3][4] = y[3][5] = y[4][3] = y[4][5] = y[5][3] = y[5][4] = -Izt; /// 3.0;
				//y[3][1] = y[3][2] = y[4][0] = y[4][2] = y[5][0] = y[5][1] = 0.0; //Izt / 3.0;
				//y[0][4] = y[0][5] = y[1][3] = y[1][5] = y[2][3] = y[2][4] = 0.0; //Izt / 3.0;

				////t[0][0] = t[2][1] = t[4][2] = complex(1.0);
				////t[0][1] = t[2][2] = t[4][0] = complex(-1.0);
				////t[1][0] = t[3][1] = t[5][2] = complex(1.0) / nt;
				////t[1][1] = t[3][2] = t[5][0] = complex(-1.0) / nt;
				////t[0][2] = t[1][2] = t[2][0] = t[3][0] = t[4][1] = t[5][1] = 0;
				//t[0][1] = t[0][2] = t[1][1] = t[1][2] = t[2][0] = t[2][2] = t[3][0] = t[3][2] = t[4][0] = t[4][1] = t[5][0] = t[5][1] = 0;
				//t[0][0] = t[2][1] = t[4][2] = nt;
				//t[1][0] = t[3][1] = t[5][2] = 1;

				//b_mat[0][0] = (t[0][0]*y[0][0]+t[1][0]*y[1][0]+t[2][0]*y[2][0]+t[3][0]*y[3][0]+t[4][0]*y[4][0]+t[5][0]*y[5][0])*t[0][0]+(t[0][0]*y[0][1]+t[1][0]*y[1][1]+t[2][0]*y[2][1]+t[3][0]*y[3][1]+t[4][0]*y[4][1]+t[5][0]*y[5][1])*t[1][0]+(t[0][0]*y[0][2]+t[1][0]*y[1][2]+t[2][0]*y[2][2]+t[3][0]*y[3][2]+t[4][0]*y[4][2]+t[5][0]*y[5][2])*t[2][0]+(t[0][0]*y[0][3]+t[1][0]*y[1][3]+t[2][0]*y[2][3]+t[3][0]*y[3][3]+t[4][0]*y[4][3]+t[5][0]*y[5][3])*t[3][0]+(t[0][0]*y[0][4]+t[1][0]*y[1][4]+t[2][0]*y[2][4]+t[3][0]*y[3][4]+t[4][0]*y[4][4]+t[5][0]*y[5][4])*t[4][0]+(t[0][0]*y[0][5]+t[1][0]*y[1][5]+t[2][0]*y[2][5]+t[3][0]*y[3][5]+t[4][0]*y[4][5]+t[5][0]*y[5][5])*t[5][0];
				//b_mat[0][1] = (t[0][0]*y[0][0]+t[1][0]*y[1][0]+t[2][0]*y[2][0]+t[3][0]*y[3][0]+t[4][0]*y[4][0]+t[5][0]*y[5][0])*t[0][1]+(t[0][0]*y[0][1]+t[1][0]*y[1][1]+t[2][0]*y[2][1]+t[3][0]*y[3][1]+t[4][0]*y[4][1]+t[5][0]*y[5][1])*t[1][1]+(t[0][0]*y[0][2]+t[1][0]*y[1][2]+t[2][0]*y[2][2]+t[3][0]*y[3][2]+t[4][0]*y[4][2]+t[5][0]*y[5][2])*t[2][1]+(t[0][0]*y[0][3]+t[1][0]*y[1][3]+t[2][0]*y[2][3]+t[3][0]*y[3][3]+t[4][0]*y[4][3]+t[5][0]*y[5][3])*t[3][1]+(t[0][0]*y[0][4]+t[1][0]*y[1][4]+t[2][0]*y[2][4]+t[3][0]*y[3][4]+t[4][0]*y[4][4]+t[5][0]*y[5][4])*t[4][1]+(t[0][0]*y[0][5]+t[1][0]*y[1][5]+t[2][0]*y[2][5]+t[3][0]*y[3][5]+t[4][0]*y[4][5]+t[5][0]*y[5][5])*t[5][1];
				//b_mat[0][2] = (t[0][0]*y[0][0]+t[1][0]*y[1][0]+t[2][0]*y[2][0]+t[3][0]*y[3][0]+t[4][0]*y[4][0]+t[5][0]*y[5][0])*t[0][2]+(t[0][0]*y[0][1]+t[1][0]*y[1][1]+t[2][0]*y[2][1]+t[3][0]*y[3][1]+t[4][0]*y[4][1]+t[5][0]*y[5][1])*t[1][2]+(t[0][0]*y[0][2]+t[1][0]*y[1][2]+t[2][0]*y[2][2]+t[3][0]*y[3][2]+t[4][0]*y[4][2]+t[5][0]*y[5][2])*t[2][2]+(t[0][0]*y[0][3]+t[1][0]*y[1][3]+t[2][0]*y[2][3]+t[3][0]*y[3][3]+t[4][0]*y[4][3]+t[5][0]*y[5][3])*t[3][2]+(t[0][0]*y[0][4]+t[1][0]*y[1][4]+t[2][0]*y[2][4]+t[3][0]*y[3][4]+t[4][0]*y[4][4]+t[5][0]*y[5][4])*t[4][2]+(t[0][0]*y[0][5]+t[1][0]*y[1][5]+t[2][0]*y[2][5]+t[3][0]*y[3][5]+t[4][0]*y[4][5]+t[5][0]*y[5][5])*t[5][2];
				//b_mat[1][0] = (t[0][1]*y[0][0]+t[1][1]*y[1][0]+t[2][1]*y[2][0]+t[3][1]*y[3][0]+t[4][1]*y[4][0]+t[5][1]*y[5][0])*t[0][0]+(t[0][1]*y[0][1]+t[1][1]*y[1][1]+t[2][1]*y[2][1]+t[3][1]*y[3][1]+t[4][1]*y[4][1]+t[5][1]*y[5][1])*t[1][0]+(t[0][1]*y[0][2]+t[1][1]*y[1][2]+t[2][1]*y[2][2]+t[3][1]*y[3][2]+t[4][1]*y[4][2]+t[5][1]*y[5][2])*t[2][0]+(t[0][1]*y[0][3]+t[1][1]*y[1][3]+t[2][1]*y[2][3]+t[3][1]*y[3][3]+t[4][1]*y[4][3]+t[5][1]*y[5][3])*t[3][0]+(t[0][1]*y[0][4]+t[1][1]*y[1][4]+t[2][1]*y[2][4]+t[3][1]*y[3][4]+t[4][1]*y[4][4]+t[5][1]*y[5][4])*t[4][0]+(t[0][1]*y[0][5]+t[1][1]*y[1][5]+t[2][1]*y[2][5]+t[3][1]*y[3][5]+t[4][1]*y[4][5]+t[5][1]*y[5][5])*t[5][0];
				//b_mat[1][1] = (t[0][1]*y[0][0]+t[1][1]*y[1][0]+t[2][1]*y[2][0]+t[3][1]*y[3][0]+t[4][1]*y[4][0]+t[5][1]*y[5][0])*t[0][1]+(t[0][1]*y[0][1]+t[1][1]*y[1][1]+t[2][1]*y[2][1]+t[3][1]*y[3][1]+t[4][1]*y[4][1]+t[5][1]*y[5][1])*t[1][1]+(t[0][1]*y[0][2]+t[1][1]*y[1][2]+t[2][1]*y[2][2]+t[3][1]*y[3][2]+t[4][1]*y[4][2]+t[5][1]*y[5][2])*t[2][1]+(t[0][1]*y[0][3]+t[1][1]*y[1][3]+t[2][1]*y[2][3]+t[3][1]*y[3][3]+t[4][1]*y[4][3]+t[5][1]*y[5][3])*t[3][1]+(t[0][1]*y[0][4]+t[1][1]*y[1][4]+t[2][1]*y[2][4]+t[3][1]*y[3][4]+t[4][1]*y[4][4]+t[5][1]*y[5][4])*t[4][1]+(t[0][1]*y[0][5]+t[1][1]*y[1][5]+t[2][1]*y[2][5]+t[3][1]*y[3][5]+t[4][1]*y[4][5]+t[5][1]*y[5][5])*t[5][1];
				//b_mat[1][2] = (t[0][1]*y[0][0]+t[1][1]*y[1][0]+t[2][1]*y[2][0]+t[3][1]*y[3][0]+t[4][1]*y[4][0]+t[5][1]*y[5][0])*t[0][2]+(t[0][1]*y[0][1]+t[1][1]*y[1][1]+t[2][1]*y[2][1]+t[3][1]*y[3][1]+t[4][1]*y[4][1]+t[5][1]*y[5][1])*t[1][2]+(t[0][1]*y[0][2]+t[1][1]*y[1][2]+t[2][1]*y[2][2]+t[3][1]*y[3][2]+t[4][1]*y[4][2]+t[5][1]*y[5][2])*t[2][2]+(t[0][1]*y[0][3]+t[1][1]*y[1][3]+t[2][1]*y[2][3]+t[3][1]*y[3][3]+t[4][1]*y[4][3]+t[5][1]*y[5][3])*t[3][2]+(t[0][1]*y[0][4]+t[1][1]*y[1][4]+t[2][1]*y[2][4]+t[3][1]*y[3][4]+t[4][1]*y[4][4]+t[5][1]*y[5][4])*t[4][2]+(t[0][1]*y[0][5]+t[1][1]*y[1][5]+t[2][1]*y[2][5]+t[3][1]*y[3][5]+t[4][1]*y[4][5]+t[5][1]*y[5][5])*t[5][2];
				//b_mat[2][0] = (t[0][2]*y[0][0]+t[1][2]*y[1][0]+t[2][2]*y[2][0]+t[3][2]*y[3][0]+t[4][2]*y[4][0]+t[5][2]*y[5][0])*t[0][0]+(t[0][2]*y[0][1]+t[1][2]*y[1][1]+t[2][2]*y[2][1]+t[3][2]*y[3][1]+t[4][2]*y[4][1]+t[5][2]*y[5][1])*t[1][0]+(t[0][2]*y[0][2]+t[1][2]*y[1][2]+t[2][2]*y[2][2]+t[3][2]*y[3][2]+t[4][2]*y[4][2]+t[5][2]*y[5][2])*t[2][0]+(t[0][2]*y[0][3]+t[1][2]*y[1][3]+t[2][2]*y[2][3]+t[3][2]*y[3][3]+t[4][2]*y[4][3]+t[5][2]*y[5][3])*t[3][0]+(t[0][2]*y[0][4]+t[1][2]*y[1][4]+t[2][2]*y[2][4]+t[3][2]*y[3][4]+t[4][2]*y[4][4]+t[5][2]*y[5][4])*t[4][0]+(t[0][2]*y[0][5]+t[1][2]*y[1][5]+t[2][2]*y[2][5]+t[3][2]*y[3][5]+t[4][2]*y[4][5]+t[5][2]*y[5][5])*t[5][0];
				//b_mat[2][1] = (t[0][2]*y[0][0]+t[1][2]*y[1][0]+t[2][2]*y[2][0]+t[3][2]*y[3][0]+t[4][2]*y[4][0]+t[5][2]*y[5][0])*t[0][1]+(t[0][2]*y[0][1]+t[1][2]*y[1][1]+t[2][2]*y[2][1]+t[3][2]*y[3][1]+t[4][2]*y[4][1]+t[5][2]*y[5][1])*t[1][1]+(t[0][2]*y[0][2]+t[1][2]*y[1][2]+t[2][2]*y[2][2]+t[3][2]*y[3][2]+t[4][2]*y[4][2]+t[5][2]*y[5][2])*t[2][1]+(t[0][2]*y[0][3]+t[1][2]*y[1][3]+t[2][2]*y[2][3]+t[3][2]*y[3][3]+t[4][2]*y[4][3]+t[5][2]*y[5][3])*t[3][1]+(t[0][2]*y[0][4]+t[1][2]*y[1][4]+t[2][2]*y[2][4]+t[3][2]*y[3][4]+t[4][2]*y[4][4]+t[5][2]*y[5][4])*t[4][1]+(t[0][2]*y[0][5]+t[1][2]*y[1][5]+t[2][2]*y[2][5]+t[3][2]*y[3][5]+t[4][2]*y[4][5]+t[5][2]*y[5][5])*t[5][1];
				//b_mat[2][2] = (t[0][2]*y[0][0]+t[1][2]*y[1][0]+t[2][2]*y[2][0]+t[3][2]*y[3][0]+t[4][2]*y[4][0]+t[5][2]*y[5][0])*t[0][2]+(t[0][2]*y[0][1]+t[1][2]*y[1][1]+t[2][2]*y[2][1]+t[3][2]*y[3][1]+t[4][2]*y[4][1]+t[5][2]*y[5][1])*t[1][2]+(t[0][2]*y[0][2]+t[1][2]*y[1][2]+t[2][2]*y[2][2]+t[3][2]*y[3][2]+t[4][2]*y[4][2]+t[5][2]*y[5][2])*t[2][2]+(t[0][2]*y[0][3]+t[1][2]*y[1][3]+t[2][2]*y[2][3]+t[3][2]*y[3][3]+t[4][2]*y[4][3]+t[5][2]*y[5][3])*t[3][2]+(t[0][2]*y[0][4]+t[1][2]*y[1][4]+t[2][2]*y[2][4]+t[3][2]*y[3][4]+t[4][2]*y[4][4]+t[5][2]*y[5][4])*t[4][2]+(t[0][2]*y[0][5]+t[1][2]*y[1][5]+t[2][2]*y[2][5]+t[3][2]*y[3][5]+t[4][2]*y[4][5]+t[5][2]*y[5][5])*t[5][2];

				//b_mat[0][0] *=temp;
				//b_mat[0][1] *=temp;
				//b_mat[0][2] *=temp;
				//b_mat[1][0] *=temp;
				//b_mat[1][1] *=temp;
				//b_mat[1][2] *=temp;
				//b_mat[2][0] *=temp;
				//b_mat[2][1] *=temp;
				//b_mat[2][2] *=temp;

			}
			else
			{
				GL_THROW("Unsupported solver method");
			}
		
			break;
		case transformer_configuration::SINGLE_PHASE_CENTER_TAPPED:
			if (has_phase(PHASE_A|PHASE_B)) // delta AB
			{
				throw "delta split tap is not supported yet";
			}
			else if (has_phase(PHASE_B|PHASE_C)) // delta AB
			{
				throw "delta split tap is not supported yet";
			}
			else if (has_phase(PHASE_A|PHASE_C)) // delta AB
			{
				throw "delta split tap is not supported yet";
			}
			else if (has_phase(PHASE_A)) // wye-A
			{
				V_basehi = config->V_primary;
				sa_base = config->phaseA_kVA_rating;
				za_basehi = (V_basehi*V_basehi)/(sa_base*1000);
				za_baselo = (V_base * V_base)/(sa_base*1000);
				z0 = complex(0.5 * config->impedance.Re(),0.8*config->impedance.Im()) * complex(za_basehi,0);
				z1 = complex(config->impedance.Re(),0.4 * config->impedance.Im()) * complex(za_baselo,0);
				z2 = complex(config->impedance.Re(),0.4 * config->impedance.Im()) * complex(za_baselo,0);
				zt_b = complex(0,0);
				zt_c = complex(0,0);
				a_mat[0][0] = a_mat[1][0] = nt;
			
				d_mat[0][0] = complex(1,0)/nt;
				d_mat[0][1] = complex(-1,0)/nt;

				A_mat[0][0] = complex(1,0)/nt;
				A_mat[1][0] = complex(1,0)/nt;
			}

			else if (has_phase(PHASE_B)) // wye-B
			{
				V_basehi = config->V_primary;
				sa_base = config->phaseB_kVA_rating;
				za_basehi = (V_basehi*V_basehi)/(sa_base*1000);
				za_baselo = (V_base * V_base)/(sa_base*1000);
				z0 = complex(0.5 * config->impedance.Re(),0.8*config->impedance.Im()) * complex(za_basehi,0);
				z1 = complex(config->impedance.Re(),0.4 * config->impedance.Im()) * complex(za_baselo,0);
				z2 = complex(config->impedance.Re(),0.4 * config->impedance.Im()) * complex(za_baselo,0);
				zt_b = complex(0,0);
				zt_c = complex(0,0);
				a_mat[0][1] = a_mat[1][1] = nt;
			
				d_mat[1][0] = complex(1,0)/nt;
				d_mat[1][1] = complex(-1,0)/nt;

				A_mat[0][1] = complex(1,0)/nt;
				A_mat[1][1] = complex(1,0)/nt;
			
			}
			else if (has_phase(PHASE_C)) // wye-C
			{
				V_basehi = config->V_primary;
				sa_base = config->phaseC_kVA_rating;
				za_basehi = (V_basehi*V_basehi)/(sa_base*1000);
				za_baselo = (V_base * V_base)/(sa_base*1000);
				z0 = complex(0.5 * config->impedance.Re(),0.8*config->impedance.Im()) * complex(za_basehi,0);
				z1 = complex(config->impedance.Re(),0.4 * config->impedance.Im()) * complex(za_baselo,0);
				z2 = complex(config->impedance.Re(),0.4 * config->impedance.Im()) * complex(za_baselo,0);
				zt_b = complex(0,0);
				zt_c = complex(0,0);
				a_mat[0][2] = a_mat[1][2] = nt;
			
				d_mat[2][0] = complex(1,0)/nt;
				d_mat[2][1] = complex(-1,0)/nt;

				A_mat[0][2] = complex(1,0)/nt;
				A_mat[1][2] = complex(1,0)/nt;
			}

			b_mat[0][0] = (z1*nt)+(z0/nt);
			b_mat[0][1] = complex(-1,0) * (z0/nt);
			b_mat[0][2] = complex(0,0);
			b_mat[1][0] = (z0/nt);
			b_mat[1][1] = complex(-1,0) * ((z2*nt) + z0/nt);
			b_mat[1][2] = complex(0,0);
			b_mat[2][0] = complex(0,0);
			b_mat[2][1] = complex(0,0);
			b_mat[2][2] = complex(0,0);

			B_mat[0][0] = (z1) + (z0/(nt*nt));
			B_mat[0][1] = complex(-1,0) * (z0/(nt*nt));
			B_mat[1][0] = (z0/(nt*nt));
			B_mat[1][1] = complex(-1,0) * ((z2) + (z0/(nt*nt)));

			break;
		default:
			throw "unknown transformer connect type";
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

	return 1;
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
	}
	catch (char *msg)
	{
		gl_error("create_transformer: %s", msg);
	}
	return 0;
}



/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_transformer(OBJECT *obj)
{
	transformer *my = OBJECTDATA(obj,transformer);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		GL_THROW("%s (transformer:%d): %s", my->get_name(), my->get_id(), msg);
		return 0; 
	}
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
	transformer *pObj = OBJECTDATA(obj,transformer);
	try {
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
	} catch (const char *error) {
		GL_THROW("%s (transformer:%d): %s", pObj->get_name(), pObj->get_id(), error);
		return 0; 
	} catch (...) {
		GL_THROW("%s (transformer:%d): %s", pObj->get_name(), pObj->get_id(), "unknown exception");
		return 0;
	}
}

EXPORT int isa_transformer(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,transformer)->isa(classname);
}

/**@}**/
