/** $Id: office.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file office.h
	@addtogroup office 
	@ingroup commercial
 @{
 **/

#ifndef _HOUSE_H
#define _HOUSE_H

#include "gridlabd.h"

class multizone;

/* each electric load must have an enduse member */
typedef struct s_endusex {
	double power_factor;		/**< ENDUSE power factor */
	complex energy;				/**< ENDUSE cumulative energy (Wh) */
	complex power;				/**< ENDUSE power (W) */
	complex demand;				/**< ENDUSE peak power demand (W) */
	complex constant_power;		/**< ENDUSE constant power part*/
	complex constant_current;	/**< ENDUSE constant current part*/
	complex constant_admittance;	/**< ENDUSE addmittance part*/
	double heatgain;
	double heatgain_fraction;
} ENDUSE;

/*****************************************************
 ENDUSE: HVAC
 *****************************************************/
/* HVAC has three mode */
typedef enum {
	HC_OFF,						/**< HVAC is off */
	HC_HEAT,					/**< HVAC is heating */
	HC_AUX,						/**< HVAC is supplemental heating */
	HC_COOL,					/**< HVAC is cooling */
	HC_ECON,					/**< HVAC is economizing */
	HC_VENT,					/**< HVAC is ventilating */
} HCMODE;

/* HVAC design parameters */
typedef struct s_hvacdesign {
	double design_temperature;	/**< design temperature (F) */
	double balance_temperature;	/**< balance temperature (F) */
	double efficiency;			/**< BTU output efficiency (BTU/W) */
	double cop;					/**< coefficient of performance (pu) */
	double capacity;			/**< BTU capacity (constant portion) */
	double capacity_perF;		/**< BTU capacity (temperature dependent portion) */
} HVACDESIGN;

/* HVAC load */
typedef struct s_hvac {
	ENDUSE enduse;				/**< HVAC enduse data */
	HCMODE mode;				/**< HVAC mode */
	HVACDESIGN cooling;			/**< HVAC cooling data */
	HVACDESIGN heating;			/**< HVAC heating data */
	double minimum_ach;			/**< minimum air-changes per hour needed when occupied */
} HVAC;

/*****************************************************
 ENDUSE: LIGHTING
 *****************************************************/
typedef struct s_lighting {
	ENDUSE enduse;				/**< LIGHTS enduse data */
	double capacity;			/**< LIGHTS capacity (W) */
	double fraction;			/**< LIGHTS fraction (puW) */
} LIGHTS;

/*****************************************************
 ENDUSE: PLUGS
 *****************************************************/
typedef struct s_plugs {
	ENDUSE enduse;				/**< PLUGS enduse data */
	double capacity;			/**< PLUGS capacity (W) */
	double fraction;			/**< PLUGS fraction (puW) */
} PLUGS;

/*****************************************************
 Prevailing conditions
 *****************************************************/
typedef struct s_conditions {
	double *pTemperature;		/**< output temperature (F) */
	double *pHumidity;			/**< relative humidity */
	double *pSolar;				/**< solar radiation (BTU/h) */
	double out_temp;			/**< outside air temperature (F) */
	double air_temperature;		/**< indoor air temperature (F) */
	double mass_temperature;	/**< indoor mass temperature (F) */
	double occupancy;			/**< number of people */
	double temperature_change;	/**< change in temperature (F/h) */
} CONDITIONS;

/*****************************************************
 Controls
 *****************************************************/
typedef struct s_controls {
	double cooling_setpoint;		/**< cooling setpoint (F) */
	double economizer_cutin;		/**< outdoor temperature at which economizer starts (F) */
	double heating_setpoint;		/**< heating setpoint (F) */
	double auxiliary_cutin;			/**< temperature difference at which aux heat starts (F) */
	double setpoint_deadband;		/**< deadband temperature (F) */
	double ventilation_fraction;	/**< ventilation fraction (pu) */
	double lighting_fraction;		/**< lighting fraction (pu) */
} CONTROLS;

/*****************************************************
 Zone information
 *****************************************************/
typedef struct s_zonedata {
	struct {
		double floor_area;		/**< total floor area (sf) */
		double floor_height;	/**< floor-ceiling height (ft) */
		double exterior_ua;		/**< exterior wall UA (BTU/F.h) */
		double interior_ua;		/**< interior mass UA (BTU/F.h) */
		double interior_mass;	/**< interior mass (BTU/F) */
		double window_area[9];	/**< windows areas (sf); see climate::CLTD for index order */
		double glazing_coeff;	/**< windows glazing coefficient (pu) */
		double occupants;		/**< occupants design capacity */
		char256 schedule;		/**< occupancy schedule (e.g., "ddd hhh; ddd hhh..." */
	} design;
	HVAC hvac;					/**< HVAC enduse */
	LIGHTS lights;				/**< LIGHTS enduse */
	PLUGS plugs;				/**< PLUGS enduse */
	ENDUSE total;				/**< total enduses */
	CONDITIONS current;			/**< current conditions */
	CONTROLS control;			/**< controllers */
} ZONEDATA;

class office : public gld_object {
	GL_STRUCT(ZONEDATA,zone);
public:
	static double warn_low_temp;
	static double warn_high_temp;
	static bool warn_control;
public:
	/* buffers */
#define SET_OCCUPIED(D,H) (occupied[H]|=(1<<D)) /* Sunday=0, ..., Holiday=7 */
#define CLR_OCCUPIED(D,H) (occupied[H]&=~(1<<D))
#define IS_OCCUPIED(D,H) ((occupied[H]&(1<<D))?1:0)
	char occupied[24];		/**< internal bitmap buffer for occupancy schedule */
	complex *pVoltage;
	complex *pCurrent;
private:
	double TcoolOn, TcoolOff; // hvac cooling on, off temperature [degF]
	double TheatOn, TheatOff; // hvac heating on, off temperature [degF]
	double cop;
	double Qi, Qh, Qs, Qz; // heating gains: plugs/lights, hvac, solar, multi-zone
	double Teq, Tevent;
	double r1, r2, k1, k2;
	double c1, c2, c3, c4, c5, c6, c7, dTi;
	void update_control_setpoints();
	TIMESTAMP update_lighting(TIMESTAMP t1);
	TIMESTAMP update_plugs(TIMESTAMP t1);
	double update_hvac();
public:
	static CLASS *oclass;
	static office *defaults;
	office(MODULE *module);
	int create();
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t1);
	inline TIMESTAMP postsync(TIMESTAMP t1) { return TS_NEVER; };
	TIMESTAMP plc(TIMESTAMP t1);

	friend class multizone;
};

#endif

/**@}*/
