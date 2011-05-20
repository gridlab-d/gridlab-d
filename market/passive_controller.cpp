/** $Id: passive_controller.h 1182 2009-09-09 22:08:36Z mhauer $
	Copyright (C) 2009 Battelle Memorial Institute
	@file passive_controller.cpp
	@addtogroup passive_controller
	@ingroup market

 **/

#include "passive_controller.h"

CLASS *passive_controller::oclass = NULL;

// ref: http://www.digitalmars.com/pnews/read.php?server=news.digitalmars.com&group=c++&artnum=3659
/***************************
*   erf.cpp
*   author:  Steve Strand
*   written: 29-Jan-04
***************************/

#include <math.h>


static const double rel_error= 1E-14;        //calculate 12^N^N 14 significant figures
//you can adjust rel_error to trade off between accuracy and speed
//but don't ask for > 15 figures (assuming usual 52 bit mantissa in a double)

static double tc_erfc(double x)
//erfc(x) = 2/sqrt(pi)*integral(exp(-t^2),t,x,inf)
//        = exp(-x^2)/sqrt(pi) * [1/x+ (1/2)/x+ (2/2)/x+ (3/2)/x+ (4/2)/x+ ...]
//        = 1-erf(x)
//expression inside [] is a continued fraction so '+' means add to denominator only
{
    static const double one_sqrtpi=  0.564189583547756287;        // 1/sqrt(pi)
//    if (fabs(x) < 2.2) {
//        return 1.0 - erf(x);        //use series when fabs(x) < 2.2
//    }
    if (x < 0.0) {               //continued fraction only valid for x>0
        return 2.0 - tc_erfc(-x);
    }
    double a=1, b=x;                //last two convergent numerators
    double c=x, d=x*x+0.5;          //last two convergent denominators
    double q1, q2= b/d;             //last two convergents (a/c and b/d)
    double n= 1.0, t;
    do {
        t= a*n+b*x;
        a= b;
        b= t;
        t= c*n+d*x;
        c= d;
        d= t;
        n+= 0.5;
        q1= q2;
        q2= b/d;
      } while (fabs(q1-q2)/q2 > rel_error);
    return one_sqrtpi*exp(-x*x)*q2;
} 

static double tc_erf(double x)
//erf(x) = 2/sqrt(pi)*integral(exp(-t^2),t,0,x)
//       = 2/sqrt(pi)*[x - x^3/3 + x^5/5*2! - x^7/7*3! + ...]
//       = 1-erfc(x)
{
    static const double two_sqrtpi=  1.128379167095512574;        // 2/sqrt(pi)
    if (fabs(x) > 2.2) {
        return 1.0 - tc_erfc(x);        //use continued fraction when fabs(x) > 2.2
    }
    double sum= x, term= x, xsqr= x*x;
    int j= 1;
    do {
        term*= xsqr/j;
        sum-= term/(2*j+1);
        ++j;
        term*= xsqr/j;
        sum+= term/(2*j+1);
        ++j;
    } while (fabs(term)/sum > rel_error);
    return two_sqrtpi*sum;
}

passive_controller::passive_controller(MODULE *mod)
{
	if(oclass == NULL){
		oclass = gl_register_class(mod,"passive_controller",sizeof(passive_controller),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
		if (oclass==NULL)
			throw "unable to register class passive_controller";
		else
			oclass->trl = TRL_QUALIFIED;

		if(gl_publish_variable(oclass,
			// series inputs
/**/		PT_int32,"input_state",PADDR(input_state),
/**/		PT_double,"input_setpoint",PADDR(input_setpoint),
/**/		PT_bool,"input_chained",PADDR(input_chained),
			// outputs
			PT_double,"observation",PADDR(observation),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the observed value",
			PT_double,"mean_observation",PADDR(obs_mean),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the observed mean value",
			PT_double,"stdev_observation",PADDR(obs_stdev),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the observed standard deviation value",
			PT_double,"expectation",PADDR(expectation),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the observed expected value",
			// inputs
/**/		PT_double,"sensitivity",PADDR(sensitivity),PT_DEPRECATED,PT_DESCRIPTION,"the sensitivity of the control actuator to observation deviations",
			PT_double,"period[s]",PADDR(dPeriod),PT_DESCRIPTION,"the cycle period for the controller logic",
/**/		PT_char32,"expectation_prop",PADDR(expectation_propname),PT_DEPRECATED,PT_DESCRIPTION,"the name of the property to observe for the expected value",
/**/		PT_object,"expectation_obj",PADDR(expectation_object),PT_DEPRECATED,PT_DESCRIPTION,"the object to watch for the expectation property",
			PT_char32,"expectation_property",PADDR(expectation_propname),PT_DESCRIPTION,"the name of the property to observe for the expected value",
			PT_object,"expectation_object",PADDR(expectation_object),PT_DESCRIPTION,"the object to watch for the expectation property",
/**/		PT_char32,"setpoint_prop",PADDR(output_setpoint_propname),PT_DEPRECATED,PT_DESCRIPTION,"the name of the setpoint property in the parent object",
			PT_char32,"setpoint",PADDR(output_setpoint_propname),PT_DESCRIPTION,"the name of the setpoint property in the parent object",
/**/		PT_char32,"state_prop",PADDR(output_state_propname),PT_DEPRECATED,PT_DESCRIPTION,"the name of the actuator property in the parent object",
			PT_char32,"state_property",PADDR(output_state_propname),PT_DESCRIPTION,"the name of the actuator property in the parent object",
/**/		PT_object,"observation_obj",PADDR(observation_object),PT_DEPRECATED,PT_DESCRIPTION,"the object to observe",
/**/		PT_char32,"observation_prop",PADDR(observation_propname),PT_DEPRECATED,PT_DESCRIPTION,"the name of the observation property",
			PT_object,"observation_object",PADDR(observation_object),PT_DESCRIPTION,"the object to observe",
			PT_char32,"observation_property",PADDR(observation_propname),PT_DESCRIPTION,"the name of the observation property",
/**/		PT_char32,"mean_observation_prop",PADDR(observation_mean_propname),PT_DEPRECATED,PT_DESCRIPTION,"the name of the mean observation property",
			PT_char32,"stdev_observation_prop",PADDR(observation_stdev_propname),PT_DEPRECATED,PT_DESCRIPTION,"the name of the standard deviation observation property",
			PT_char32,"stdev_observation_property",PADDR(observation_stdev_propname),PT_DESCRIPTION,"the name of the standard deviation observation property",
/**/		PT_int32,"cycle_length",PADDR(cycle_length),PT_DEPRECATED,PT_DESCRIPTION,"length of time between processing cycles",
			PT_double,"base_setpoint",PADDR(base_setpoint),PT_DESCRIPTION,"the base setpoint to base control off of",
			PT_double,"critical_day",PADDR(critical_day),PT_DESCRIPTION,"used to switch between TOU and CPP days, 1 is CPP, 0 is TOU",
			PT_bool,"two_tier_cpp",PADDR(check_two_tier_cpp),			
			PT_double,"daily_elasticity",PADDR(dailyElasticity),
			PT_double,"sub_elasticity_first_second",PADDR(subElasticityFirstSecond),
			PT_double,"sub_elasticity_first_third",PADDR(subElasticityFirstThird),
			PT_int32,"second_tier_hours",PADDR(secondTierHours),
			PT_int32,"third_tier_hours",PADDR(thirdTierHours),
			PT_int32,"first_tier_hours",PADDR(firstTierHours),
			PT_double,"first_tier_price",PADDR(firstTierPrice),
			PT_double,"second_tier_price",PADDR(secondTierPrice),
			PT_double,"third_tier_price",PADDR(thirdTierPrice),
			PT_double,"old_first_tier_price",PADDR(oldFirstTierPrice),
			PT_double,"old_second_tier_price",PADDR(oldSecondTierPrice),
			PT_double,"old_third_tier_price",PADDR(oldThirdTierPrice),
			PT_double,"Percent_change_in_price",PADDR(dailyElasticityMultiplier),
			PT_double,"Percent_change_in_peakoffpeak_ratio",PADDR(peakPriceMultiplier_test),
			PT_double,"Percent_change_in_Criticalpeakoffpeak_ratio",PADDR(criticalpeakPriceMultiplier_test),
			PT_bool,"linearize_elasticity",PADDR(linearizeElasticity),
			PT_double,"price_offset",PADDR(price_offset),		
			PT_bool,"pool_pump_model",PADDR(pool_pump_model),PT_DESCRIPTION,"Boolean flag for turning on the pool pump version of the DUTYCYCLE control",
			PT_double,"base_duty_cycle",PADDR(base_duty_cycle),PT_DESCRIPTION,"This is the duty cycle before modification due to the price signal",

			// probabilistic control inputs
			PT_enumeration, "distribution_type",PADDR(distribution_type),
				PT_KEYWORD, "NORMAL", PDT_NORMAL,
				PT_KEYWORD, "EXPONENTIAL", PDT_EXPONENTIAL,
				PT_KEYWORD, "UNIFORM", PDT_UNIFORM,
			PT_double, "comfort_level",PADDR(comfort_level),
			// specific inputs
			PT_double,"range_high",PADDR(range_high),
			PT_double,"range_low",PADDR(range_low),
			PT_double,"ramp_high",PADDR(ramp_high),
			PT_double,"ramp_low",PADDR(ramp_low),
			// specific outputs
/**/		PT_double,"prob_off",PADDR(prob_off),
/**/		PT_int32,"output_state",PADDR(output_state),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the target setpoint given the input observations",
/**/		PT_double,"output_setpoint",PADDR(output_setpoint),
			// enums
			PT_enumeration,"control_mode",PADDR(control_mode),PT_DESCRIPTION,"the control mode to use for determining controller action",
				PT_KEYWORD,"NONE",CM_NONE,
				PT_KEYWORD,"RAMP",CM_RAMP,
				PT_KEYWORD,"DUTYCYCLE",CM_DUTYCYCLE,
				PT_KEYWORD,"PROBABILITY_OFF",CM_PROBOFF,
				PT_KEYWORD,"ELASTICITY_MODEL",CM_ELASTICITY_MODEL,
			NULL) < 1)
		{
				GL_THROW("unable to publish properties in %s",__FILE__);
		}
		memset(this,0,sizeof(passive_controller));
	}
}

int passive_controller::create(){
	memset(this, 0, sizeof(passive_controller));
	sensitivity = 1.0;
	comfort_level = 1.0;
	zipLoadParent = false;
	pool_pump_model = false;
	check_two_tier_cpp = false;
	linearizeElasticity = false;
	return 1;
}

int passive_controller::init(OBJECT *parent){
	
	OBJECT *hdr = OBJECTHDR(this);
	PROPERTY *enduseProperty;

	if(parent == NULL){
		gl_error("passive_controller has no parent and will be operating in 'dummy' mode");
	} else {
		if(output_state_propname[0] == 0 && output_setpoint_propname[0] == 0){
			GL_THROW("passive_controller has no output properties");
		}
		// expectation_addr
		if(expectation_object != 0){
			expectation_property = gl_get_property(expectation_object, expectation_propname);
			if(expectation_property == 0){
				GL_THROW("passive_controller cannot find its expectation property");
			}
			expectation_addr = (void *)((unsigned int64)expectation_object + sizeof(OBJECT) + (unsigned int64)expectation_property->addr);
		}

		if(observation_object != 0){
			// observation_addr
			observation_prop = gl_get_property(observation_object, observation_propname);
			if(observation_prop != 0){
				observation_addr = (void *)((unsigned int64)observation_object + sizeof(OBJECT) + (unsigned int64)observation_prop->addr);
			}
			if (pool_pump_model == false)
			{
				// observation_mean_addr
				observation_mean_prop = gl_get_property(observation_object, observation_mean_propname);
				if(observation_mean_prop != 0){
					observation_mean_addr = (void *)((unsigned int64)observation_object + sizeof(OBJECT) + (unsigned int64)observation_mean_prop->addr);
				}
				// observation_stdev_addr
				stdev_observation_property = gl_get_property(observation_object, observation_stdev_propname);
				if(stdev_observation_property != 0){
					observation_stdev_addr = (void *)((unsigned int64)observation_object + sizeof(OBJECT) + (unsigned int64)stdev_observation_property->addr);
				}
			}

		}
		// output_state
		if(output_state_propname[0] != 0){
			output_state_prop = gl_get_property(parent, output_state_propname);
			if(output_state_prop == NULL){
				GL_THROW("passive_controller parent \"%s\" does not contain property \"%s\"", 
					(parent->name ? parent->name : "anon"), output_state_propname);
			}
			output_state_addr = (void *)((unsigned int64)parent + sizeof(OBJECT) + (unsigned int64)output_state_prop->addr);
		}
		
		// output_setpoint
		if (control_mode != this->CM_PROBOFF && control_mode != this->CM_ELASTICITY_MODEL)
		{
			if(output_setpoint_propname[0] == 0 && output_setpoint_propname[0] == 0){
				GL_THROW("passive_controller has no output properties");
			}
			if(output_setpoint_propname[0] != 0){
				output_setpoint_property = gl_get_property(parent, output_setpoint_propname);
				if(output_setpoint_property == NULL){
					GL_THROW("passive_controller parent \"%s\" does not contain property \"%s\"", 
						(parent->name ? parent->name : "anon"), output_setpoint_propname);
				}
				output_setpoint_addr = (void *)((unsigned int64)parent + sizeof(OBJECT) + (unsigned int64)output_setpoint_property->addr);
			}
		}
	}

	if (pool_pump_model == false && control_mode != CM_ELASTICITY_MODEL)
		gl_set_dependent(hdr, expectation_object);
	gl_set_dependent(hdr, observation_object);

	if(observation_object == NULL){
		GL_THROW("passive_controller observation_object object is undefined, and can not function");
	}
	
	//
	// make sure that the observation_object and expectable are ranked above the controller
	//

	if(base_setpoint != 0.0){
		orig_setpoint = 1;
	}

	if (pool_pump_model == true)
	{
		if (control_mode != CM_DUTYCYCLE)
			GL_THROW("pool pump mode must be used with control mode set to DUTYCYCLE");
		if (firstTierHours == 0 || secondTierHours == 0)
			GL_THROW("Please set first and second tier hours in pool pump duty cycle mode");
		if (firstTierPrice == 0 || secondTierPrice == 0)
			GL_THROW("Please set first and second tier prices in pool pump duty cycle mode");
	}

	if(dPeriod == 0.0){
		dPeriod = 300.0;
		period = 300; // five minutes
	} else {
		period = (TIMESTAMP)floor(dPeriod + 0.5);
	}

	if (gl_object_isa(parent,"ZIPload","residential"))
	{
		zipLoadParent = true;
	}
	
	if(zipLoadParent == true && control_mode == CM_ELASTICITY_MODEL){
	
		ThirdTierArraySize = 0;
		SecondTierArraySize = 0;
		FirstTierArraySize = 0;

		elasticityPeriod = 24;

		if(subElasticityFirstSecond > 0)
			gl_warning("The peak to offpeak Substitution Elasticity is positive.  While this is allowed, it is typically the reverse of convention, as an increase in peak to offpeak price ratio generally produces a reduction in peak load to offpeak load. This is indicated by a negative peak to offpeak Substitution Elasticity value.");

		if(subElasticityFirstThird > 0)
			gl_warning("The critical peak to offpeak Substitution Elasticity is positive.  While this is allowed, it is typically the reverse of convention, as an increase in critical peak to offpeak price ratio generally produces a reduction in peak load to offpeak load. This is indicated by a negative critical peak to offpeak Substitution Elasticity value.");

		if(dailyElasticity > 0)
			gl_warning("The Daily Elasticity is positive.  While this is allowed, it is typically the reverse of convention, as an increase in daily price generally produces a reduction in daily load. This is indicated by a negative Daily Elasticity value.");

		if(critical_day >= 0.5){

			if (firstTierPrice == 0)
					GL_THROW("Please set first tier price in the Elasticity Model");

			if (oldFirstTierPrice == 0)
					GL_THROW("Please set old first tier price in the Elasticity Model");

				
			if (thirdTierHours == 0)
				GL_THROW("Please set third tier hours in the Elasticity Model");

			if (thirdTierPrice == 0)
				GL_THROW("Please set third tier price in the Elasticity Model");

			if (oldThirdTierPrice == 0){

				oldThirdTierPrice = oldFirstTierPrice;
				gl_warning("Old third tier price is missing. System will assume the old pricing scheme was a fixed pricing scheme and use the old first tier price");

			}

			if(check_two_tier_cpp != true){
				
				if (secondTierHours == 0)
					GL_THROW("Please set second tier hours in the Elasticity Model");

				if (secondTierPrice == 0)
					GL_THROW("Please set second tier price in the Elasticity Model");

				if (oldSecondTierPrice == 0){

					oldSecondTierPrice = oldFirstTierPrice;
					gl_warning("Old second tier price is missing. System will assume the old pricing scheme was a fixed pricing scheme and use the old first tier price");

				}				
				
				if(firstTierHours == 0){
					firstTierHours = elasticityPeriod - thirdTierHours - secondTierHours;
				}
				else{
					if((thirdTierHours+secondTierHours+firstTierHours) != elasticityPeriod)
					GL_THROW("Please set the tier hours correctly in the Elasticity Model");
				}

				SecondTierArraySize = (int)(((secondTierHours * 3600) / period));

				//Calculation of the price ratios and change in price ratios.
				//This will not change for the course of the simulation
				oldPriceRatioSecondFirst = oldSecondTierPrice/oldFirstTierPrice;
				newPriceRatioSecondFirst = secondTierPrice/firstTierPrice;

				if(linearizeElasticity == true)
				{
					peakPriceMultiplier = pow(newPriceRatioSecondFirst/oldPriceRatioSecondFirst,subElasticityFirstSecond);
				}
				else{

					peakPriceMultiplier = 1 + ((subElasticityFirstSecond)*(newPriceRatioSecondFirst-oldPriceRatioSecondFirst)/oldPriceRatioSecondFirst);
					priceDiffSecond = secondTierPrice - oldSecondTierPrice; 
				}

			}
			else{

				if(firstTierHours == 0){
					firstTierHours = elasticityPeriod - thirdTierHours;
				}
				else{
					if((thirdTierHours+firstTierHours) != elasticityPeriod)
					GL_THROW("Please set the tier hours correctly in the Elasticity Model");
				}					
			}

			ThirdTierArraySize = (int)(((thirdTierHours * 3600) / period));			

			//Calculation of the price ratios and change in price ratios.
			//This will not change for the course of the simulation
			oldPriceRatioThirdFirst = oldThirdTierPrice/oldFirstTierPrice;
			newPriceRatioThirdFirst = thirdTierPrice/firstTierPrice;

			if(linearizeElasticity == true)
			{
				criticalPriceMultiplier = pow(newPriceRatioThirdFirst/oldPriceRatioThirdFirst,subElasticityFirstThird);
			}
			else{

				criticalPriceMultiplier = 1 + ((subElasticityFirstThird)*(newPriceRatioThirdFirst-oldPriceRatioThirdFirst)/oldPriceRatioThirdFirst);
				priceDiffThird = thirdTierPrice - oldThirdTierPrice; 	
			}

		}
		else{			

			if (firstTierPrice == 0)
					GL_THROW("Please set first tier price in the Elasticity Model");

			if (oldFirstTierPrice == 0)
					GL_THROW("Please set old first tier price in the Elasticity Model");
			
			if(firstTierHours == 0){
				firstTierHours = elasticityPeriod - secondTierHours;
			}
			else{
				if((secondTierHours+firstTierHours) != elasticityPeriod)
				GL_THROW("Please set the tier hours correctly in the Elasticity Model");
			}

			if (secondTierHours == 0)
				GL_THROW("Please set second tier hours in the Elasticity Model");

			if (secondTierPrice == 0)
				GL_THROW("Please set second tier price in the Elasticity Model");

			if (oldSecondTierPrice == 0){
					oldSecondTierPrice = oldFirstTierPrice;
					gl_warning("Old second tier price is missing. System will assume the old pricing scheme was a fixed pricing scheme and use the old first tier price");
			}

			SecondTierArraySize = (int)(((secondTierHours * 3600) / period));
				
			//Calculation of the price ratios and change in price ratios.
			//This will not change for the course of the simulation
			oldPriceRatioSecondFirst = oldSecondTierPrice/oldFirstTierPrice;
			newPriceRatioSecondFirst = secondTierPrice/firstTierPrice;

			if(linearizeElasticity == true)
			{
				peakPriceMultiplier = pow(newPriceRatioSecondFirst/oldPriceRatioSecondFirst,subElasticityFirstSecond);
			}
			else{
				peakPriceMultiplier = 1 + ((subElasticityFirstSecond)*(newPriceRatioSecondFirst-oldPriceRatioSecondFirst)/oldPriceRatioSecondFirst);
				priceDiffSecond = secondTierPrice - oldSecondTierPrice; 				
			}
		}

		FirstTierArraySize = (int)(((firstTierHours * 3600) / period));

		priceDiffFirst = firstTierPrice - oldFirstTierPrice; 	
	
		ArraySize = (int)(((elasticityPeriod * 3600) / period));

		if(price_offset==0) price_offset = 10E-6 ;	
		
		tier_prices = (double *)gl_malloc(ArraySize*sizeof(double));

		if (tier_prices == NULL)
			GL_THROW("Failure to allocate tier_prices array");

		cleared_load = (double *)gl_malloc(ArraySize*sizeof(double));

		if (cleared_load == NULL)
			GL_THROW("Failure to allocate cleared_load array");

		offPeakLoad = (double *)gl_malloc(FirstTierArraySize*sizeof(double));

		if (offPeakLoad == NULL)
			GL_THROW("Failure to allocate offPeakLoad array");

		peakLoad = (double *)gl_malloc(SecondTierArraySize*sizeof(double));

		if (peakLoad == NULL)
			GL_THROW("Failure to allocate peakLoad array");

		criticalPeakLoad = (double *)gl_malloc(ThirdTierArraySize*sizeof(double));

		if (criticalPeakLoad == NULL)
			GL_THROW("Failure to allocate criticalPeakLoad array");	

		//Link up to parent object
		enduseProperty = gl_get_property(parent,"base_power");
		if (enduseProperty == NULL)
			GL_THROW("Unable to map base power property");
		
		current_load_enduse = (enduse*)GETADDR(parent,enduseProperty);

		//Initialize the array locations
		ArrayIndex = 0;
		ThirdTierArrayIndex = 0;
		SecondTierArrayIndex = 0;
		FirstTierArrayIndex = 0;

		for(int32 i=0; i < ArraySize; i++){
			
			tier_prices[i] = 0;
			cleared_load[i] = 0;	
		}

		for(int32 i=0; i < FirstTierArraySize; i++){
			offPeakLoad[i] = 0;
		}
		
		for(int32 i=0; i < SecondTierArraySize; i++){
			peakLoad[i] = 0;
		}

		for(int32 i=0; i < ThirdTierArraySize; i++){
			criticalPeakLoad[i] = 0;
		}	
		
	}

	return 1;
}


int passive_controller::isa(char *classname)
{
	return strcmp(classname,"passive_controller")==0;
}


TIMESTAMP passive_controller::presync(TIMESTAMP t0, TIMESTAMP t1){

	if(starttime == 0)
		starttime = t0;	

	// determine output based on control mode
	if(last_cycle == 0 || t1 >= last_cycle + period || period == 0){
		last_cycle = t1; // advance cycle time
			// get observations
		if(observation_addr != 0){
			observation = *(double *)observation_addr;
		} else {
			observation = 0;
		}
		if(observation_mean_addr != 0){
			obs_mean = *(double *)observation_mean_addr;
		} else {
			obs_mean = 0;
		}
		if(observation_stdev_addr != 0){
			obs_stdev = *(double *)observation_stdev_addr;
		} else {
			obs_stdev = 0;
		}

		// get expectation
		if(expectation_addr != 0){
			expectation = *(double *)expectation_addr;
		} else {
			expectation = 0;
		}

		switch(control_mode){
			case CM_NONE:
				// no control ~ let it slide
				break;
			case CM_RAMP:
				if(calc_ramp(t0, t1) != 0){
					GL_THROW("error occured when handling the ramp control mode");
				}
				break;
			case CM_PROBOFF:
				if(calc_proboff(t0, t1) != 0){
					GL_THROW("error occured when handling the probabilistic cutoff control mode");
				}
				break;
			case CM_DUTYCYCLE:
				if(calc_dutycycle(t0, t1) != 0){
					GL_THROW("error occured when handling the duty cycle control mode");
				}
				break;
			case CM_ELASTICITY_MODEL:
				if(calc_elasticity(t0, t1) != 0){
					GL_THROW("error occured when handling the elasticity model control mode");
				}
				break;
			default:
				GL_THROW("passive_controller has entered an invalid control mode");
				break;
		}

		// determine if input is chained first
		if(output_setpoint_addr != 0){
			*(double *)output_setpoint_addr = output_setpoint;
		}
		if(output_state_addr != 0){
			*(int *)output_state_addr = output_state;
		}
	}
	return (period > 0 ? last_cycle+period : TS_NEVER);
}

TIMESTAMP passive_controller::sync(TIMESTAMP t0, TIMESTAMP t1){
#if 0
	// determine output based on control mode
	if(t1 <= last_cycle + period || period == 0){
		last_cycle = t1; // advance cycle time
		switch(control_mode){
			case CM_NONE:
				// no control ~ let it slide
				break;
			case CM_RAMP:
				if(calc_ramp(t0, t1) != 0){
					GL_THROW("error occured when handling the ramp control mode");
				}
				break;
			case CM_PROBOFF:
				if(calc_proboff(t0, t1) != 0){
					GL_THROW("error occured when handling the probabilistic cutoff control mode");
				}
				break;
			case CM_DUTYCYCLE:
				if(calc_dutycycle(t0, t1) != 0){
					GL_THROW("error occured when handling the duty cycle control mode");
				}
				break;
			default:
				GL_THROW("passive_controller has entered an invalid control mode");
				break;
		}

		// determine if input is chained first
		if(output_setpoint_addr != 0){
			*(double *)output_setpoint_addr = output_setpoint;
		}
		if(output_state_addr != 0){
			*(int *)output_state_addr = output_state;
		}
	}
	return (period > 0 ? last_cycle+period : TS_NEVER);
#endif

	return TS_NEVER;

}

TIMESTAMP passive_controller::postsync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

int passive_controller::calc_elasticity(TIMESTAMP t0, TIMESTAMP t1){

	if(zipLoadParent == true){
		
			double dt = (double)(t1 - t0);

				
			if((t1-starttime)%period==0 && (t1!=starttime) && starttime>0 && dt > 0){

				totalClearedLoad = 0;

				cleared_load[ArrayIndex] = *(double *)current_load_enduse;

				predicted_load =  *(double *)current_load_enduse;
				tier_prices[ArrayIndex]	= observation;		

				if(ArraySize-1 == ArrayIndex){
					ArrayIndex = 0;
				}
				else{
					ArrayIndex++;
				}

				for(int32 i=0; i < ArraySize; i++){					
					totalClearedLoad += cleared_load[i]*period;
					//pDailyAverage += (tier_prices[i]*((double)period/3600))*cleared_load[i];							
				}

				//totalClearedLoad = totalClearedLoad/(elasticityPeriod*3600);
				
				if(fabs(observation - thirdTierPrice) <= price_offset){
						
					totalCriticalPeakLoad = 0;
					
					criticalPeakLoad[ThirdTierArrayIndex] = *(double *)current_load_enduse;

					if(ThirdTierArraySize-1 == ThirdTierArrayIndex){
						ThirdTierArrayIndex = 0;
					}
					else{
						ThirdTierArrayIndex++;
					}				
			
					for(int32 i=0; i < ThirdTierArraySize; i++){					
						totalCriticalPeakLoad += criticalPeakLoad[i]*period;					
					}

					//totalCriticalPeakLoad = totalCriticalPeakLoad/(thirdTierHours*3600);
					
				}
				else if(fabs(observation - secondTierPrice) <= price_offset){

					totalPeakLoad = 0;

					peakLoad[SecondTierArrayIndex] = *(double *)current_load_enduse;

					if(SecondTierArraySize-1 == SecondTierArrayIndex){
						SecondTierArrayIndex = 0;
					}
					else{
						SecondTierArrayIndex++;
					}

					for(int32 i=0; i < SecondTierArraySize; i++){					
						totalPeakLoad += peakLoad[i]*period;					
					}

					//totalPeakLoad = totalPeakLoad/(secondTierHours*3600);
				}
				else if(fabs(observation - firstTierPrice) <= price_offset){
						
					totalOffPeakLoad = 0;

					offPeakLoad[FirstTierArrayIndex] = *(double *)current_load_enduse;
					
					if(FirstTierArraySize-1 == FirstTierArrayIndex){
						FirstTierArrayIndex = 0;
					}
					else{
						FirstTierArrayIndex++;
					}

					for(int32 i=0; i < FirstTierArraySize; i++){					
						totalOffPeakLoad += offPeakLoad[i]*period;					
					}

					//totalOffPeakLoad = totalOffPeakLoad/(firstTierHours*3600);
				}	
				else{
					GL_THROW("Market price signal does not match with any of the tier prices");
				}
			}

			if(starttime>0){
				
				dailyElasticityMultiplier = 0;
				double qDailyNew = 0;
				double qNewSecond = 0;
				double qNewFirst = 0;
				double qNewThird = 0;

				if(critical_day >= 0.5){
						
					if(check_two_tier_cpp == true){

						if(linearizeElasticity == true)
						{
							dailyElasticityMultiplier = (totalCriticalPeakLoad*thirdTierPrice + totalOffPeakLoad*firstTierPrice)/(totalCriticalPeakLoad*oldThirdTierPrice + totalOffPeakLoad*oldFirstTierPrice);
						}
						else
						{
							dailyElasticityMultiplier = (totalCriticalPeakLoad*priceDiffThird + totalOffPeakLoad*priceDiffFirst)/(totalCriticalPeakLoad*thirdTierPrice + totalOffPeakLoad*firstTierPrice);
						}

					}
					else{

						if(linearizeElasticity == true)
						{
							dailyElasticityMultiplier = (totalCriticalPeakLoad*thirdTierPrice + totalPeakLoad*secondTierPrice + totalOffPeakLoad*firstTierPrice)/(totalCriticalPeakLoad*oldThirdTierPrice  + totalPeakLoad*oldSecondTierPrice + totalOffPeakLoad*oldFirstTierPrice);
						}
						else
						{
							dailyElasticityMultiplier = (totalCriticalPeakLoad*priceDiffThird + totalPeakLoad*priceDiffSecond + totalOffPeakLoad*priceDiffFirst)/(totalPeakLoad*secondTierPrice + totalCriticalPeakLoad*thirdTierPrice + totalOffPeakLoad*firstTierPrice);
						}
					}
				}
				else{

					if(linearizeElasticity == true)
					{
						dailyElasticityMultiplier = (totalPeakLoad*secondTierPrice + totalOffPeakLoad*firstTierPrice)/(totalPeakLoad*oldSecondTierPrice + totalOffPeakLoad*oldFirstTierPrice);
					}
					else
					{
						dailyElasticityMultiplier = (totalPeakLoad*priceDiffSecond + totalOffPeakLoad*priceDiffFirst)/(totalPeakLoad*secondTierPrice + totalOffPeakLoad*firstTierPrice);
					}
				}

				if(linearizeElasticity == true)
				{
					qDailyNew = totalClearedLoad*pow(dailyElasticityMultiplier,dailyElasticity);

				}
				else{

					qDailyNew = totalClearedLoad*(1 + ((dailyElasticity)*dailyElasticityMultiplier));
				}

				if(qDailyNew > 0.0){

					double tempDivider = 0;
					double tempPeakMultiplier = 0;
					double tempCriticalPeakMultiplier = 0;

					if(critical_day >= 0.5){	

						tempCriticalPeakMultiplier = (totalCriticalPeakLoad/totalOffPeakLoad)*criticalPriceMultiplier;

						if(check_two_tier_cpp != true){

							tempPeakMultiplier = (totalPeakLoad/totalOffPeakLoad)*peakPriceMultiplier;	

						}
					}
					else{

						tempPeakMultiplier = (totalPeakLoad/totalOffPeakLoad)*peakPriceMultiplier;	

					}

					//criticalpeakPriceMultiplier_test = tempCriticalPeakMultiplier;

					/*if(tempCriticalPeakMultiplier <= 0.0){

						tempCriticalPeakMultiplier = 0.0;
					}*/

					//peakPriceMultiplier_test = tempPeakMultiplier;

					tempDivider = 1 + tempCriticalPeakMultiplier + tempPeakMultiplier;

					qNewFirst = qDailyNew/(tempDivider);			
										
					if(fabs(observation - firstTierPrice) <= price_offset){

						/*double temp = 1 + (qNewFirst/firstTierHours - totalOffPeakLoad)/totalOffPeakLoad;

						if(temp < 0){
							*(double *)output_state_addr = temp;
						}
						else{
							*(double *)output_state_addr = temp;
						}*/

						*(double *)output_state_addr = 1 + (qNewFirst - totalOffPeakLoad)/totalOffPeakLoad;
					
					}
					else if(fabs(observation - secondTierPrice) <= price_offset){
						
						qNewSecond = qNewFirst*tempPeakMultiplier;
						
						/*double temp = 1 + (qNewSecond/secondTierHours - totalPeakLoad)/totalPeakLoad;

						if(temp < 0){
							*(double *)output_state_addr = temp;
						}
						else{
							*(double *)output_state_addr = temp;
						}*/

						*(double *)output_state_addr = 1 + (qNewSecond - totalPeakLoad)/totalPeakLoad;
												
					}
					else if(fabs(observation - thirdTierPrice) <= price_offset){

						qNewThird = qNewFirst*tempCriticalPeakMultiplier;				
						
						/*double temp = 1 + (qNewThird/thirdTierHours - totalCriticalPeakLoad)/totalCriticalPeakLoad;						
						
						if(temp < 0){
							*(double *)output_state_addr = temp;
						}
						else{
							*(double *)output_state_addr = temp;
						}*/

						*(double *)output_state_addr =  1 + (qNewThird - totalCriticalPeakLoad)/totalCriticalPeakLoad;						

					}	
					else{					
						GL_THROW("Market price signal does not match with any of the tier prices");				
					}
							
				}
				else{

					*(double *)output_state_addr = 1;

				}
					
			}		
		
		}
		
		return 0;

}

int passive_controller::calc_ramp(TIMESTAMP t0, TIMESTAMP t1){
	double T_limit;
	double T_set;
	double ramp;
	double set_change;
	extern double bid_offset;

	if(!orig_setpoint){
		base_setpoint = *(double *)output_setpoint_addr;
		orig_setpoint = 1;
	}

	// "olypen style" ramp with k_high, k_low, rng_high, rng_low
	if(expectation_addr == 0 || observation_addr == 0 || observation_stdev_addr == 0){
		gl_error("insufficient input for ramp control mode");
		return -1;
	}
	if(ramp_high * ramp_low < 0 || range_high * range_low > 0){ // zero is legit
		gl_warning("invalid ramp parameters");
	}

	/*
	if(observation > expectation){ // net ramp direction
		ramp = ramp_high;
	} else {
		ramp = ramp_low;
	}
	*/

	//T_limit = (observation > expectation && ramp > 0.0 ? range_high : range_low);
	if(observation > expectation){
		if(ramp_low > 0.0){
			T_limit = range_high;
			ramp = ramp_high;
		} else {
			T_limit = range_low;
			ramp = ramp_low;
		}
		//T_limit = (ramp_low > 0.0 ? range_high : range_low);
	} else if (observation < expectation){
		if(ramp_low < 0.0){
			T_limit = range_high;
			ramp = ramp_high;
		} else {
			T_limit = range_low;
			ramp = ramp_low;
		}
		//T_limit = (ramp_low 0.0 ? range_low : range_high);
	} else {
		output_setpoint = base_setpoint;
		output_state = 0;
		return 0;
	}

	if(T_limit == 0){
		// zero range short-circuit
		output_setpoint = base_setpoint;
		output_state = 0;
		return 0;
	}

	T_set = base_setpoint;

	// is legit to set expectation to the mean
	if(obs_stdev < bid_offset){
		set_change = 0.0;
	} else {
		set_change = (observation - expectation) * fabs(T_limit) / (ramp * obs_stdev);
	}
	if(set_change > range_high){
		set_change = range_high;
	}
	if(set_change < range_low){
		set_change = range_low;
	}
	output_setpoint = T_set + set_change;
	output_state = 0;
	return 0;
}


int passive_controller::calc_dutycycle(TIMESTAMP t0, TIMESTAMP t1){
	
	if (pool_pump_model == true)
	{
		OBJECT *hdr = OBJECTHDR(this);
		
		if(output_state_addr != 0){
			output_state = *(int *)output_state_addr;
		} else {
			output_state = 0;
		}
		
		if(output_setpoint_addr != 0){
			output_setpoint = *(double *)output_setpoint_addr;
		} else {
			output_setpoint = 0;
		}

		// Note:  this method is only currently compatible w/ 2 tier TOU or 2 tier TOU + CPP
		if (thirdTierPrice != 0.0 && observation >= thirdTierPrice) // we're in CPP
		{
			output_state = 1; // turn on override
			//if (output_setpoint == -1) // if recovery_duty_cycle hasn't been set, use this value
			//	output_setpoint = recovery_level;
		}
		else if (observation >= secondTierPrice) // we're in high TOU
		{
			if (output_state == 1)
				output_state = -1; // turn off the override and let the pool pump recover
			else if (output_state == 0) // we're back to normal operation
			{
				double hour_ratio = (double) secondTierHours / (firstTierHours + secondTierHours);
				output_setpoint = base_duty_cycle * (firstTierPrice / (firstTierPrice + secondTierPrice)) / hour_ratio;
			}

		}
		else // we're in low TOU
		{
			if (output_state == 1)
				output_state = -1; // turn off the override and let the pool pump recover
			else if (output_state == 0) // we're back to normal operation
			{
				double hour_ratio = (double) firstTierHours / (firstTierHours + secondTierHours);
				output_setpoint = base_duty_cycle * (secondTierPrice / (firstTierPrice + secondTierPrice)) / hour_ratio;
			}
		}
	}
	return 0;
}


int passive_controller::calc_proboff(TIMESTAMP t0, TIMESTAMP t1){
	if(observation_addr == 0 || expectation_addr == 0 || observation_stdev_addr == 0){
		GL_THROW("insufficient input for probabilistic shutoff control mode");
		return -1;
	}
	double erf_in, erf_out;
	double cdf_out;
	double r;
	extern double bid_offset;

	
	switch(distribution_type){
		case PDT_NORMAL:
			// N is the cumulative normal distribution
			// k_w is a comfort setting
			// r is compared to a uniformly random number on [0,1.0)
			// erf_in = (x-mean) / (var^2 * sqrt(2))
			if(obs_stdev < bid_offset){ // short circuit
				if(observation > expectation){
					output_state = -1;
					prob_off = 1.0;
				} else {
					output_state = 0;
					prob_off = 0.0;
				}
				return 0;
			}
			erf_in = (observation - expectation) / (obs_stdev*obs_stdev * sqrt(2.0));
			erf_out = tc_erf(erf_in);
			cdf_out = 0.5 * (1 + erf_out);
			prob_off = comfort_level * (cdf_out-0.5);			
			break;
		case PDT_EXPONENTIAL:
			prob_off = comfort_level * (1 - exp(-observation/expectation));
			break;
		case PDT_UNIFORM:
			prob_off = comfort_level * (observation - expectation) / expectation;
			break;
		default:
			gl_error("");
	}
	if(prob_off < 0 || !isfinite(prob_off)){
		prob_off = 0.0;
	}
	r = gl_random_uniform(0.0,1.0);
	if(r < prob_off){
		output_state = -1; // off?
	} else {
		output_state = 0; //Normal
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_passive_controller(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(passive_controller::oclass);
		if (*obj!=NULL)
		{
			passive_controller *my = OBJECTDATA(*obj,passive_controller);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(passive_controller);
}

EXPORT int init_passive_controller(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL){
			return OBJECTDATA(obj,passive_controller)->init(parent);
		}
		else
			return 0;
	}
	INIT_CATCHALL(passive_controller);
}

EXPORT int isa_passive_controller(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,passive_controller)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_passive_controller(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	passive_controller *my = OBJECTDATA(obj,passive_controller);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock, t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			obj->clock = t1;
			break;
		default:
			//GL_THROW("invalid pass request (%d)", pass);
			gl_error("invalid pass request (%d)", pass);
			t2 = TS_INVALID;
			break;
		}
		return t2;
	}
	SYNC_CATCHALL(passive_controller);
}

// EOF

