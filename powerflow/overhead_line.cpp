/** $Id: overhead_line.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file overhead_line.cpp
	@addtogroup overhead_line 
	@ingroup line

	@{
**/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <iostream>
using namespace std;

#include "line.h"

CLASS* overhead_line::oclass = NULL;
CLASS* overhead_line::pclass = NULL;

overhead_line::overhead_line(MODULE *mod) : line(mod)
{
	if(oclass == NULL)
	{
		pclass = line::oclass;
		
		oclass = gl_register_class(mod,"overhead_line",sizeof(overhead_line),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class overhead_line";
		else
			oclass->trl = TRL_PROVEN;

        if(gl_publish_variable(oclass,
			PT_INHERIT, "line",
			NULL) < 1) GL_THROW("unable to publish overhead_line properties in %s",__FILE__);
			if (gl_publish_function(oclass,	"create_fault", (FUNCTIONADDR)create_fault_ohline)==NULL)
				GL_THROW("Unable to publish fault creation function");
			if (gl_publish_function(oclass,	"fix_fault", (FUNCTIONADDR)fix_fault_ohline)==NULL)
				GL_THROW("Unable to publish fault restoration function");

			//Publish deltamode functions
			if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_link)==NULL)
				GL_THROW("Unable to publish overhead line deltamode function");
    }
}

int overhead_line::create(void)
{
	int result = line::create();

	return result;
}

int overhead_line::init(OBJECT *parent)
{
	double *temp_rating_value = NULL;
	double temp_rating_continuous = 10000.0;
	double temp_rating_emergency = 20000.0;
	char index;
	OBJECT *temp_obj;
	line::init(parent);
	
	if (!configuration)
		throw "no overhead line configuration specified.";
		/*  TROUBLESHOOT
		No overhead line configuration was specified.  Please use object line_configuration
		with appropriate values to specify an overhead line configuration
		*/

	if (!gl_object_isa(configuration, "line_configuration"))
		throw "invalid line configuration for overhead line";
		/*  TROUBLESHOOT
		The object specified as the configuration for the overhead line is not a valid
		configuration object.  Please ensure you have a line_configuration object selected.
		*/

	//Test the phases
	line_configuration *config = OBJECTDATA(configuration, line_configuration);

	test_phases(config,'A');
	test_phases(config,'B');
	test_phases(config,'C');
	test_phases(config,'N');
	
	if ((!config->line_spacing || !gl_object_isa(config->line_spacing, "line_spacing")) && config->impedance11 == 0.0 && config->impedance22 == 0.0 && config->impedance33 == 0.0)
		throw "invalid or missing line spacing on overhead line";
		/*  TROUBLESHOOT
		The configuration object for the overhead line is missing the line_spacing configuration
		or the item specified in that location is invalid.
		*/

	recalc();

	//Values are populated now - populate link ratings parameter
	for (index=0; index<4; index++)
	{
		if (index==0)
		{
			temp_obj = config->phaseA_conductor;
		}
		else if (index==1)
		{
			temp_obj = config->phaseB_conductor;
		}
		else if (index==2)
		{
			temp_obj = config->phaseC_conductor;
		}
		else	//Must be 3
		{
			temp_obj = config->phaseN_conductor;
		}

		//See if Phase exists
		if (temp_obj != NULL)
		{
			//Get continuous - summer
			temp_rating_value = get_double(temp_obj,"rating.summer.continuous");

			//Check if NULL
			if (temp_rating_value != NULL)
			{
				//Update - if necessary
				if (temp_rating_continuous > *temp_rating_value)
				{
					temp_rating_continuous = *temp_rating_value;
				}
			}

			//Get continuous - winter
			temp_rating_value = get_double(temp_obj,"rating.winter.continuous");

			//Check if NULL
			if (temp_rating_value != NULL)
			{
				//Update - if necessary
				if (temp_rating_continuous > *temp_rating_value)
				{
					temp_rating_continuous = *temp_rating_value;
				}
			}

			//Now get emergency - summer
			temp_rating_value = get_double(temp_obj,"rating.summer.emergency");

			//Check if NULL
			if (temp_rating_value != NULL)
			{
				//Update - if necessary
				if (temp_rating_emergency > *temp_rating_value)
				{
					temp_rating_emergency = *temp_rating_value;
				}
			}

			//Now get emergency - winter
			temp_rating_value = get_double(temp_obj,"rating.winter.emergency");

			//Check if NULL
			if (temp_rating_value != NULL)
			{
				//Update - if necessary
				if (temp_rating_emergency > *temp_rating_value)
				{
					temp_rating_emergency = *temp_rating_value;
				}
			}
		}//End Phase valid
	}//End FOR

	//Populate link array
	link_rating[0] = temp_rating_continuous;
	link_rating[1] = temp_rating_emergency;

	return 1;
}

void overhead_line::recalc(void)
{
	line_configuration *config = OBJECTDATA(configuration, line_configuration);
	complex Zabc_mat[3][3], Yabc_mat[3][3];
	OBJECT *obj = OBJECTHDR(this);

	// Zero out Zabc_mat and Yabc_mat. Un-needed phases will be left zeroed.
	for (int i = 0; i < 3; i++) 
	{
		for (int j = 0; j < 3; j++) 
		{
			Zabc_mat[i][j] = 0.0;
			Yabc_mat[i][j] = 0.0;
		}
	}

	// Set auxillary matrices
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			a_mat[i][j] = 0.0;
			d_mat[i][j] = 0.0;
			A_mat[i][j] = 0.0;
			c_mat[i][j] = 0.0;
			B_mat[i][j] = b_mat[i][j];
		}
	}
	
	if (config->impedance11 != 0 || config->impedance22 != 0 || config->impedance33 != 0)
	{
		// Load Zabc_mat and Yabc_mat based on the z11-z33 and c11-c33 line config parameters
		load_matrix_based_configuration(Zabc_mat, Yabc_mat);

		//Other auxilliary by phase
		if (has_phase(PHASE_A))
		{
			a_mat[0][0] = 1.0;
			d_mat[0][0] = 1.0;
			A_mat[0][0] = 1.0;
		}

		if (has_phase(PHASE_B))
		{
			a_mat[1][1] = 1.0;
			d_mat[1][1] = 1.0;
			A_mat[1][1] = 1.0;
		}

		if (has_phase(PHASE_C))
		{
			a_mat[2][2] = 1.0;
			d_mat[2][2] = 1.0;
			A_mat[2][2] = 1.0;
		}
	}
	else
	{
		// Use Kersting's equations to define the z-matrix
		double dab, dbc, dac, dan, dbn, dcn;
		double gmr_a, gmr_b, gmr_c, gmr_n, res_a, res_b, res_c, res_n;
		complex z_aa, z_ab, z_ac, z_an, z_bb, z_bc, z_bn, z_cc, z_cn, z_nn;
		double p_aa, p_ab, p_ac, p_an, p_bb, p_bc, p_bn, p_cc, p_cn, p_nn;
		double daap, dabp, dacp, danp, dbbp, dbcp, dbnp, dccp, dcnp, dnnp, diamA, diamB, diamC, diamN;
		complex P_mat[3][3];
		bool valid_capacitance = false;	//Assume capacitance is invalid by default
		double freq_coeff_real, freq_coeff_imag, freq_additive_term;
		line_spacing *spacing_val = NULL;
		double miles = length / 5280.0;
		double cap_coeff;
		complex cap_freq_mult;
		
		//Calculate coefficients for self and mutual impedance - incorporates frequency values
		//Per Kersting (4.39) and (4.40)
		freq_coeff_real = 0.00158836*nominal_frequency;
		freq_coeff_imag = 0.00202237*nominal_frequency;
		freq_additive_term = log(EARTH_RESISTIVITY/nominal_frequency)/2.0 + 7.6786;

		#define GMR(ph) (has_phase(PHASE_##ph) && config->phase##ph##_conductor ? \
			OBJECTDATA(config->phase##ph##_conductor, overhead_line_conductor)->geometric_mean_radius : 0.0)
		#define RES(ph) (has_phase(PHASE_##ph) && config->phase##ph##_conductor ? \
			OBJECTDATA(config->phase##ph##_conductor, overhead_line_conductor)->resistance : 0.0)
		#define DIST(ph1, ph2) (has_phase(PHASE_##ph1) && has_phase(PHASE_##ph2) && config->line_spacing ? \
			OBJECTDATA(config->line_spacing, line_spacing)->distance_##ph1##to##ph2 : 0.0)
		#define DIAM(ph) (has_phase(PHASE_##ph) && config->phase##ph##_conductor ? \
			OBJECTDATA(config->phase##ph##_conductor, overhead_line_conductor)->cable_diameter : 0.0)

		gmr_a = GMR(A);
		gmr_b = GMR(B);
		gmr_c = GMR(C);
		gmr_n = GMR(N);
		//res_a = (status==IMPEDANCE_CHANGED && (affected_phases&PHASE_A)) ? resistance/miles : RES(A);
		//res_b = (status==IMPEDANCE_CHANGED && (affected_phases&PHASE_A)) ? resistance/miles : RES(B);
		//res_c = (status==IMPEDANCE_CHANGED && (affected_phases&PHASE_A)) ? resistance/miles : RES(C);
		//res_n = (status==IMPEDANCE_CHANGED && (affected_phases&PHASE_A)) ? resistance/miles : RES(N);
		res_a = RES(A);
		res_b = RES(B);
		res_c = RES(C);
		res_n = RES(N);
		dab = DIST(A, B);
		dbc = DIST(B, C);
		dac = DIST(A, C);
		dan = DIST(A, N);
		dbn = DIST(B, N);
		dcn = DIST(C, N);

		diamA = DIAM(A);
		diamB = DIAM(B);
		diamC = DIAM(C);
		diamN = DIAM(N);

		#undef GMR
		#undef RES
		#undef DIST
		#undef DIAM

		if (use_line_cap == true)
		{
			//If capacitance calculations desired, compute overall coefficient
			cap_coeff = 1.0/(PERMITIVITTY_AIR*2.0*PI);

			//Set capacitor frequency/distance/scaling factor (rad/s*S)
			cap_freq_mult = complex(0,(2.0*PI*nominal_frequency*0.000001*miles));

			//Extract line spacing (nned for capacitance)
			spacing_val = OBJECTDATA(config->line_spacing, line_spacing);

			//Make sure it worked
			if (spacing_val == NULL)
			{
				GL_THROW("Line spacing not found, capacitance calculations failed!");
				/*  TROUBLESHOOT
				The line spacing values could not be properly mapped.  Capacitance values
				can not be calculated for this line (and other values may be in error).  Please
				specific a proper line spacing and try again.
				*/
			}

			//Start with the assumption the capacitance is valid for appropriate phases
			valid_capacitance = true;
		}
		//Defaulted else - not really needed, since if not use_line_cap, these values are irrelevant

		if (has_phase(PHASE_A)) {
			if (gmr_a > 0.0 && res_a > 0.0)
				z_aa = complex(res_a + freq_coeff_real, freq_coeff_imag * (log(1.0 / gmr_a) + freq_additive_term));
			else
				z_aa = 0.0;
			if (has_phase(PHASE_B) && dab > 0.0)
				z_ab = complex(freq_coeff_real, freq_coeff_imag * (log(1.0 / dab) + freq_additive_term));
			else
				z_ab = 0.0;
			if (has_phase(PHASE_C) && dac > 0.0)
				z_ac = complex(freq_coeff_real, freq_coeff_imag * (log(1.0 / dac) + freq_additive_term));
			else
				z_ac = 0.0;
			if (has_phase(PHASE_N) && dan > 0.0)
				z_an = complex(freq_coeff_real, freq_coeff_imag * (log(1.0 / dan) + freq_additive_term));
			else
				z_an = 0.0;

			//capacitance equations
			if (use_line_cap == true)
			{
				if ((diamA > 0.0) && (spacing_val->distance_AtoE > 0.0))
				{
					//Define values - self image
					daap = 2.0 * spacing_val->distance_AtoE;
					p_aa = cap_coeff * log(daap/diamA*24.0);

					//Get distances to relevant images
					if (has_phase(PHASE_B))
					{
						//Calc distance
						dabp = calc_image_dist(spacing_val->distance_AtoE,spacing_val->distance_BtoE,spacing_val->distance_AtoB);

						//calculate the contribution
						if (spacing_val->distance_AtoB != 0)
						{
							p_ab = cap_coeff * log(dabp/(spacing_val->distance_AtoB));
						}
						else
						{
							p_ab = 0.0;
						}
					}
					else
					{
						p_ab = 0.0;
					}

					if (has_phase(PHASE_C))
					{
						//Calculate distance
						dacp = calc_image_dist(spacing_val->distance_AtoE,spacing_val->distance_CtoE,spacing_val->distance_AtoC);

						//calculate the contribution
						if (spacing_val->distance_AtoC != 0)
						{
							p_ac = cap_coeff * log(dacp/(spacing_val->distance_AtoC));
						}
						else
						{
							p_ac = 0.0;
						}
					}
					else
					{
						p_ac = 0.0;
					}

					if (has_phase(PHASE_N))
					{
						//Calculate the distance
						danp = calc_image_dist(spacing_val->distance_AtoE,spacing_val->distance_NtoE,spacing_val->distance_AtoN);

						//calculate the contribution
						if (spacing_val->distance_AtoN != 0)
						{
							p_an = cap_coeff * log(danp/(spacing_val->distance_AtoN));
						}
						else
						{
							p_an = 0.0;
						}
					}
					else
					{
						p_an = 0.0;
					}
				}
				else	//If diamA is 0, nothing is valid, make all zero
				{
					valid_capacitance = false;	//Failed one line of it, so don't include capacitance anywhere
					
					gl_warning("Shunt capacitance of overhead line:%s not calculated - invalid values",OBJECTHDR(this)->name);
					/*  TROUBLESHOOT
					While attempting to calculate the shunt capacitance for an overhead line, an invalid parameter was encountered.
					To calculate shunt capacitance, ensure the condutor to earth distance for each phase is defined, as well as the
					diameter of that phases's cable.
					*/

					p_aa = p_ab = p_ac = p_an = 0.0;
				}
			}
			//Defaulted else - not desired, so don't set anything
		} else {
			p_aa = p_ab = p_ac = p_an = 0.0;
			z_aa = z_ab = z_ac = z_an = 0.0;
		}

		if (has_phase(PHASE_B)) {
			if (gmr_b > 0.0 && res_b > 0.0)
				z_bb = complex(res_b + freq_coeff_real, freq_coeff_imag * (log(1.0 / gmr_b) + freq_additive_term));
			else
				z_bb = 0.0;
			if (has_phase(PHASE_C) && dbc > 0.0)
				z_bc = complex(freq_coeff_real, freq_coeff_imag * (log(1.0 / dbc) + freq_additive_term));
			else
				z_bc = 0.0;
			if (has_phase(PHASE_N) && dbn > 0.0)
				z_bn = complex(freq_coeff_real, freq_coeff_imag * (log(1.0 / dbn) + freq_additive_term));
			else
				z_bn = 0.0;

			//capacitance equations
			if (use_line_cap == true)
			{
				if ((diamB > 0.0) && (spacing_val->distance_BtoE > 0.0))
				{
					//Define values - self image
					dbbp = 2.0 * spacing_val->distance_BtoE;
					p_bb = cap_coeff * log(dbbp/diamB*24.0);

					//Get distances to relevant images - assuming weren't done above
					if (has_phase(PHASE_C))
					{
						//Calculate distance
						dbcp = calc_image_dist(spacing_val->distance_BtoE,spacing_val->distance_CtoE,spacing_val->distance_BtoC);

						//calculate the contribution
						if (spacing_val->distance_BtoC != 0)
						{
							p_bc = cap_coeff * log(dbcp/(spacing_val->distance_BtoC));
						}
						else
						{
							p_bc = 0.0;
						}
					}
					else
					{
						p_bc = 0.0;
					}

					if (has_phase(PHASE_N))
					{
						//Calculate distance
						dbnp = calc_image_dist(spacing_val->distance_BtoE,spacing_val->distance_NtoE,spacing_val->distance_BtoN);

						//calculate the contribution
						if (spacing_val->distance_BtoN != 0)
						{
							p_bn = cap_coeff * log(dbnp/(spacing_val->distance_BtoN));
						}
						else
						{
							p_bn = 0.0;
						}

					}
					else
					{
						p_bn = 0.0;
					}
				}
				else	//If diamb is 0, nothing is valid, make all zero
				{
					valid_capacitance = false;	//Failed one line of it, so don't include capacitance anywhere
					
					gl_warning("Shunt capacitance of overhead line:%s not calculated - invalid values",OBJECTHDR(this)->name);
					//Defined above

					p_bb = p_bc = p_bn = 0.0;
				}
			}
			//Defaulted else - not desired, so don't set anything
		} else {
			p_bb = p_bc = p_bn = 0.0;
			z_bb = z_bc = z_bn = 0.0;
		}

		if (has_phase(PHASE_C)) {
			if (gmr_c > 0.0 && res_c > 0.0)
				z_cc = complex(res_c + freq_coeff_real, freq_coeff_imag * (log(1.0 / gmr_c) + freq_additive_term));
			else
				z_cc = 0.0;
			if (has_phase(PHASE_N) && dcn > 0.0)
				z_cn = complex(freq_coeff_real, freq_coeff_imag * (log(1.0 / dcn) + freq_additive_term));
			else
				z_cn = 0.0;

			//capacitance equations
			if (use_line_cap == true)
			{
				if ((diamC > 0.0) && (spacing_val->distance_CtoE > 0.0))
				{
					//Define values - self image
					dccp = 2.0 * spacing_val->distance_CtoE;
					p_cc = cap_coeff * log(dccp/diamC*24.0);

					//Get distances to relevant images - assuming weren't done above
					if (has_phase(PHASE_N))
					{
						//Calculate the distance
						dcnp = calc_image_dist(spacing_val->distance_CtoE,spacing_val->distance_NtoE,spacing_val->distance_CtoN);

						//calculate the contribution
						if (spacing_val->distance_CtoN != 0)
						{
							p_cn = cap_coeff * log(dcnp/(spacing_val->distance_CtoN));
						}
						else
						{
							p_cn = 0.0;
						}
					}
					else
					{
						p_cn = 0.0;
					}
				}
				else	//If diamC is 0, nothing is valid, make all zero
				{
					valid_capacitance = false;	//Failed one line of it, so don't include capacitance anywhere

					gl_warning("Shunt capacitance of overhead line:%s not calculated - invalid values",OBJECTHDR(this)->name);
					//Defined above

					p_cc = p_cn = 0.0;
				}
			}
			//Defaulted else - not desired, so don't set anything
		} else {
			p_cc = p_cn = 0.0;
			z_cc = z_cn = 0.0;
		}

		complex z_nn_inv = 0;
		if (has_phase(PHASE_N) && gmr_n > 0.0 && res_n > 0.0){
			z_nn = complex(res_n + freq_coeff_real, freq_coeff_imag * (log(1.0 / gmr_n) + freq_additive_term));
			z_nn_inv = z_nn^(-1.0);

			//capacitance equations
			if (use_line_cap == true)
			{
				if ((diamN > 0.0) && (spacing_val->distance_NtoE > 0.0))
				{
					//Define values - self image
					dnnp = 2.0 * spacing_val->distance_NtoE;
					p_nn = cap_coeff * log(dnnp/diamN*24.0);
				}
				else	//If diamN is 0, nothing is valid, make all zero
				{
					valid_capacitance = false;	//Failed one line of it, so don't include capacitance anywhere

					gl_warning("Shunt capacitance of overhead line:%s not calculated - invalid values",OBJECTHDR(this)->name);
					//Defined above

					p_nn = 0.0;
				}
			}
			//Defaulted else - not desired, so don't set anything
		}
		else
		{
			p_nn = 0.0;
			z_nn = 0.0;
		}

		//Update impedance
		Zabc_mat[0][0] = (z_aa - z_an * z_an * z_nn_inv) * miles;
		Zabc_mat[0][1] = (z_ab - z_an * z_bn * z_nn_inv) * miles;
		Zabc_mat[0][2] = (z_ac - z_an * z_cn * z_nn_inv) * miles;
		Zabc_mat[1][0] = (z_ab - z_bn * z_an * z_nn_inv) * miles;
		Zabc_mat[1][1] = (z_bb - z_bn * z_bn * z_nn_inv) * miles;
		Zabc_mat[1][2] = (z_bc - z_bn * z_cn * z_nn_inv) * miles;
		Zabc_mat[2][0] = (z_ac - z_cn * z_an * z_nn_inv) * miles;
		Zabc_mat[2][1] = (z_bc - z_cn * z_bn * z_nn_inv) * miles;
		Zabc_mat[2][2] = (z_cc - z_cn * z_cn * z_nn_inv) * miles;

		// If we have valid capacitance values and line capacitance is turned on then
		// calculate Yabc_mat otherwise just leave is zeroed out.

		if (valid_capacitance == true && use_line_cap == true)
		{
			if (p_nn != 0.0)
			{
				//Create the Pabc matrix
				P_mat[0][0] = p_aa - p_an*p_an/p_nn;
				P_mat[0][1] = P_mat[1][0] = p_ab - p_an*p_bn/p_nn;
				P_mat[0][2] = P_mat[2][0] = p_ac - p_an*p_cn/p_nn;

				P_mat[1][1] = p_bb - p_bn*p_bn/p_nn;
				P_mat[1][2] = P_mat[2][1] = p_bc - p_bn*p_cn/p_nn;

				P_mat[2][2] = p_cc - p_cn*p_cn/p_nn;
			}
			else //Neutral must have had issues, ignore it
			{
				//Create the Pabc matrix
				P_mat[0][0] = p_aa;
				P_mat[0][1] = P_mat[1][0] = p_ab;
				P_mat[0][2] = P_mat[2][0] = p_ac;

				P_mat[1][1] = p_bb;
				P_mat[1][2] = P_mat[2][1] = p_bc;

				P_mat[2][2] = p_cc;
			}

			//Now appropriately invert it - scale for frequency, distance, and microSiemens as well as per Kersting (5.14) and (5.15) 
			if (has_phase(PHASE_A) && !has_phase(PHASE_B) && !has_phase(PHASE_C)) //only A
				Yabc_mat[0][0] = complex(1.0) / P_mat[0][0] * cap_freq_mult;
			else if (!has_phase(PHASE_A) && has_phase(PHASE_B) && !has_phase(PHASE_C)) //only B
				Yabc_mat[1][1] = complex(1.0) / P_mat[1][1] * cap_freq_mult;
			else if (!has_phase(PHASE_A) && !has_phase(PHASE_B) && has_phase(PHASE_C)) //only C
				Yabc_mat[2][2] = complex(1.0) / P_mat[2][2] * cap_freq_mult;
			else if (has_phase(PHASE_A) && !has_phase(PHASE_B) && has_phase(PHASE_C)) //has A & C
			{
				complex detvalue = P_mat[0][0]*P_mat[2][2] - P_mat[0][2]*P_mat[2][0];

				Yabc_mat[0][0] = P_mat[2][2] / detvalue * cap_freq_mult;
				Yabc_mat[0][2] = P_mat[0][2] * -1.0 / detvalue * cap_freq_mult;
				Yabc_mat[2][0] = P_mat[2][0] * -1.0 / detvalue * cap_freq_mult;
				Yabc_mat[2][2] = P_mat[0][0] / detvalue * cap_freq_mult;
			}
			else if (has_phase(PHASE_A) && has_phase(PHASE_B) && !has_phase(PHASE_C)) //has A & B
			{
				complex detvalue = P_mat[0][0]*P_mat[1][1] - P_mat[0][1]*P_mat[1][0];

				Yabc_mat[0][0] = P_mat[1][1] / detvalue * cap_freq_mult;
				Yabc_mat[0][1] = P_mat[0][1] * -1.0 / detvalue * cap_freq_mult;
				Yabc_mat[1][0] = P_mat[1][0] * -1.0 / detvalue * cap_freq_mult;
				Yabc_mat[1][1] = P_mat[0][0] / detvalue * cap_freq_mult;
			}
			else if (!has_phase(PHASE_A) && has_phase(PHASE_B) && has_phase(PHASE_C))	//has B & C
			{
				complex detvalue = P_mat[1][1]*P_mat[2][2] - P_mat[1][2]*P_mat[2][1];

				Yabc_mat[1][1] = P_mat[2][2] / detvalue * cap_freq_mult;
				Yabc_mat[1][2] = P_mat[1][2] * -1.0 / detvalue * cap_freq_mult;
				Yabc_mat[2][1] = P_mat[2][1] * -1.0 / detvalue * cap_freq_mult;
				Yabc_mat[2][2] = P_mat[1][1] / detvalue * cap_freq_mult;

				//Other auxilliary by phase
				if (has_phase(PHASE_A))
				{
					a_mat[0][0] = 1.0;
					d_mat[0][0] = 1.0;
					A_mat[0][0] = 1.0;
				}

				if (has_phase(PHASE_B))
				{
					a_mat[1][1] = 1.0;
					d_mat[1][1] = 1.0;
					A_mat[1][1] = 1.0;
				}

				if (has_phase(PHASE_C))
				{
					a_mat[2][2] = 1.0;
					d_mat[2][2] = 1.0;
					A_mat[2][2] = 1.0;
				}
			}
			else if ((has_phase(PHASE_A) && has_phase(PHASE_B) && has_phase(PHASE_C)) || (has_phase(PHASE_D))) //has ABC or D (D=ABC)
			{
				complex detvalue = P_mat[0][0]*P_mat[1][1]*P_mat[2][2] - P_mat[0][0]*P_mat[1][2]*P_mat[2][1] - P_mat[0][1]*P_mat[1][0]*P_mat[2][2] + P_mat[0][1]*P_mat[2][0]*P_mat[1][2] + P_mat[1][0]*P_mat[0][2]*P_mat[2][1] - P_mat[0][2]*P_mat[1][1]*P_mat[2][0];

				//Invert it
				Yabc_mat[0][0] = (P_mat[1][1]*P_mat[2][2] - P_mat[1][2]*P_mat[2][1]) / detvalue * cap_freq_mult;
				Yabc_mat[0][1] = (P_mat[0][2]*P_mat[2][1] - P_mat[0][1]*P_mat[2][2]) / detvalue * cap_freq_mult;
				Yabc_mat[0][2] = (P_mat[0][1]*P_mat[1][2] - P_mat[0][2]*P_mat[1][1]) / detvalue * cap_freq_mult;
				Yabc_mat[1][0] = (P_mat[2][0]*P_mat[1][2] - P_mat[1][0]*P_mat[2][2]) / detvalue * cap_freq_mult;
				Yabc_mat[1][1] = (P_mat[0][0]*P_mat[2][2] - P_mat[0][2]*P_mat[2][0]) / detvalue * cap_freq_mult;
				Yabc_mat[1][2] = (P_mat[1][0]*P_mat[0][2] - P_mat[0][0]*P_mat[1][2]) / detvalue * cap_freq_mult;
				Yabc_mat[2][0] = (P_mat[1][0]*P_mat[2][1] - P_mat[1][1]*P_mat[2][0]) / detvalue * cap_freq_mult;
				Yabc_mat[2][1] = (P_mat[0][1]*P_mat[2][0] - P_mat[0][0]*P_mat[2][1]) / detvalue * cap_freq_mult;
				Yabc_mat[2][2] = (P_mat[0][0]*P_mat[1][1] - P_mat[0][1]*P_mat[1][0]) / detvalue * cap_freq_mult;
			}

			//Other auxilliary by phase
			if (has_phase(PHASE_A))
			{
				a_mat[0][0] = 1.0;
				d_mat[0][0] = 1.0;
				A_mat[0][0] = 1.0;
			}

			if (has_phase(PHASE_B))
			{
				a_mat[1][1] = 1.0;
				d_mat[1][1] = 1.0;
				A_mat[1][1] = 1.0;
			}

			if (has_phase(PHASE_C))
			{
				a_mat[2][2] = 1.0;
				d_mat[2][2] = 1.0;
				A_mat[2][2] = 1.0;
			}
		}
	}

	// Calculate line matrixies A_mat, B_mat, a_mat, b_mat, c_mat and d_mat based on Zabc_mat and Yabc_mat
	recalc_line_matricies(Zabc_mat, Yabc_mat);
	
	//Check for negative resistance in the line's impedance matrix
	bool neg_res = false;
	for (int n = 0; n < 3; n++){
		for (int m = 0; m < 3; m++){
			if(b_mat[n][m].Re() < 0.0){
				neg_res = true;
			}
		}
	}
	
	if(neg_res == true){
		gl_warning("INIT: overhead_line:%s has a negative resistance in it's impedance matrix. This will result in unusual behavior. Please check the line's geometry and cable parameters.", obj->name);
		/*  TROUBLESHOOT
		A negative resistance value was found for one or more the real parts of the overhead_line's impedance matrix.
		While this is numerically possible, it is a physical impossibility. This resulted most likely from a improperly 
		defined conductor cable. Please check and modify the conductor objects used for this line to correct this issue.
		*/
	}

#ifdef _TESTING
	if (show_matrix_values)
	{
		OBJECT *obj = GETOBJECT(this);

		gl_testmsg("overhead_line: %s a matrix",obj->name);
		print_matrix(a_mat);

		gl_testmsg("overhead_line: %s A matrix",obj->name);
		print_matrix(A_mat);

		gl_testmsg("overhead_line: %s b matrix",obj->name);
		print_matrix(b_mat);

		gl_testmsg("overhead_line: %s B matrix",obj->name);
		print_matrix(B_mat);

		gl_testmsg("overhead_line: %s c matrix",obj->name);
		print_matrix(c_mat);

		gl_testmsg("overhead_line: %s d matrix",obj->name);
		print_matrix(d_mat);
	}
#endif
}

int overhead_line::isa(char *classname)
{
	return strcmp(classname,"overhead_line")==0 || line::isa(classname);
}

/**
* test_phases is called to ensure all necessary conductors are included in the
* configuration object and are of the proper type.
*
* @param config the line configuration object
* @param ph the phase to check
*/
void overhead_line::test_phases(line_configuration *config, const char ph)
{
	bool condCheck, condNotPres;
	OBJECT *obj = GETOBJECT(this);

	if (ph=='A')
	{
		if (config->impedance11 == 0.0)
		{
			condCheck = (config->phaseA_conductor && !gl_object_isa(config->phaseA_conductor, "overhead_line_conductor"));
			condNotPres = ((!config->phaseA_conductor) && has_phase(PHASE_A));
		}
		else
		{
			condCheck = false;
			condNotPres = false;
		}
	}
	else if (ph=='B')
	{
		if (config->impedance22 == 0.0)
		{
			condCheck = (config->phaseB_conductor && !gl_object_isa(config->phaseB_conductor, "overhead_line_conductor"));
			condNotPres = ((!config->phaseB_conductor) && has_phase(PHASE_B));
		}
		else
		{
			condCheck = false;
			condNotPres = false;
		}
	}
	else if (ph=='C')
	{
		if (config->impedance33 == 0.0)
		{
			condCheck = (config->phaseC_conductor && !gl_object_isa(config->phaseC_conductor, "overhead_line_conductor"));
			condNotPres = ((!config->phaseC_conductor) && has_phase(PHASE_C));
		}
		else
		{
			condCheck = false;
			condNotPres = false;
		}
	}
	else if (ph=='N')
	{
		if (config->impedance11 == 0.0 && config->impedance22 == 0.0 && config->impedance33 == 0.0)
		{
			condCheck = (config->phaseN_conductor && !gl_object_isa(config->phaseN_conductor, "overhead_line_conductor"));
			condNotPres = ((!config->phaseN_conductor) && has_phase(PHASE_N));
		}
		else
		{
			condCheck = false;
			condNotPres = false;
		}
	}
	//Nothing else down here.  Should never get anything besides ABCN to check

	if (condCheck==true)
		GL_THROW("invalid conductor for phase %c of overhead line %s",ph,obj->name);
		/*	TROUBLESHOOT  The conductor specified for the indicated phase is not necessarily an overhead line conductor, it may be an underground or triplex-line only conductor */

	if (condNotPres==true)
		GL_THROW("missing conductor for phase %c of overhead line %s",ph,obj->name);
		/*  TROUBLESHOOT
		The conductor specified for the indicated phase for the overhead line is missing
		or invalid.
		*/
}

/**
* determines distance to images for calculation of
* line capacitance calculations
*
* Calculates from line 1 to line 2
*
* * Inputs *
* dist1_to_e - distance from line 1 to earth
* dist2_to_e - distance from line 2 to earth
* dist1_to_2 - distance from line 1 to line 2
* 
* * Outputs *
* dist1_to_2p - distance from line 1 to line 2's image - same as line 2 to line 1's image (isosceles trapezoid)
*/
double overhead_line::calc_image_dist(double dist1_to_e, double dist2_to_e, double dist1_to_2)
{
	double dist1_to_1p, dist2_to_2p, dist1_to_2p;

	//Distance to self images
	dist1_to_1p = 2.0 * dist1_to_e;	//Image to 1 is just reflection underground
	dist2_to_2p = 2.0 * dist2_to_e;	//image to 2 is just reflection underground

	//Diagonals should be the same, since forms isosceles trapezoid - sqrt(c^2 + ab) - a & b are double sides (above ground to its image)
	dist1_to_2p = sqrt(dist1_to_2*dist1_to_2 + dist1_to_1p*dist2_to_2p);	//sqrt(c^2 + ab)

	//Put the value back
	return dist1_to_2p;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: overhead_line
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/


/* This can be added back in after tape has been moved to commit

EXPORT TIMESTAMP commit_overhead_line(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	if ((solver_method==SM_FBS) || (solver_method==SM_NR))
	{
		overhead_line *plink = OBJECTDATA(obj,overhead_line);
		plink->calculate_power();
	}
	return TS_NEVER;
}
*/
EXPORT int create_overhead_line(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(overhead_line::oclass);
		if (*obj!=NULL)
		{
			overhead_line *my = OBJECTDATA(*obj,overhead_line);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(overhead_line);
}

EXPORT TIMESTAMP sync_overhead_line(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		overhead_line *pObj = OBJECTDATA(obj,overhead_line);
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
	SYNC_CATCHALL(overhead_line);
}

EXPORT int init_overhead_line(OBJECT *obj)
{
	try {
		overhead_line *my = OBJECTDATA(obj,overhead_line);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(overhead_line);
}

EXPORT int isa_overhead_line(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,line)->isa(classname);
}

EXPORT int recalc_overhead_line(OBJECT *obj)
{
	OBJECTDATA(obj,overhead_line)->recalc();
	return 1;
}
EXPORT int create_fault_ohline(OBJECT *thisobj, OBJECT **protect_obj, char *fault_type, int *implemented_fault, TIMESTAMP *repair_time, void *Extra_Data)
{
	int retval;

	//Link to ourselves
	overhead_line *thisline = OBJECTDATA(thisobj,overhead_line);

	//Try to fault up
	retval = thisline->link_fault_on(protect_obj, fault_type, implemented_fault, repair_time, Extra_Data);

	return retval;
}
EXPORT int fix_fault_ohline(OBJECT *thisobj, int *implemented_fault, char *imp_fault_name, void* Extra_Data)
{
	int retval;

	//Link to ourselves
	overhead_line *thisline = OBJECTDATA(thisobj,overhead_line);

	//Clear the fault
	retval = thisline->link_fault_off(implemented_fault, imp_fault_name, Extra_Data);
	
	//Clear the fault type
	*implemented_fault = -1;

	return retval;
}
/**@}**/
