/** $Id: powerflow_object.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file powerflow.h
	@addtogroup powerflow_object

	@{
 **/

#ifndef _POWERFLOWOBJECT_H
#define _POWERFLOWOBJECT_H

#include "gridlabd.h"

// DPC: this is to remain undefined until GS solver method is reconciled with reliability module
//#define SUPPORT_OUTAGES /**< defined when reliability module is supported */

#ifdef SUPPORT_OUTAGES
/** Operating condition flags 
	Bits are combined to create abnormal conditions, e.g.,
	\verbatim OC_PHASE_A_CONTACT|OC_PHASE_B_CONTACT \endverbatim
	is used to indicate that phase A and B are in contact (i.e., shorted).
 **/
#define OC_NORMAL			0x0000	/**< operating condition flag; no problems */
/* contact flags */
#define OC_CONTACT			0x1000	/**< contact condition flag (must be combined with at least 2 connections */
#define OC_PHASE_A_CONTACT	0x1001	/**< phase A contact */
#define OC_PHASE_B_CONTACT	0x1002	/**< phase B contact */
#define OC_PHASE_C_CONTACT	0x1004	/**< phase C contact */
#define OC_NEUTRAL_CONTACT	0x1008	/**< neutral contact */
#define OC_SPLIT_1_CONTACT	0x1010	/**< triplex line 1 contact */
#define OC_SPLIT_2_CONTACT	0x1020	/**< triplex line 2 contact */
#define OC_SPLIT_N_CONTACT	0x1040	/**< triplex neutral contact */
#define OC_GROUND_CONTACT	0x1080	/**< ground contact */
#define OC_CONTACT_ALL		0x10ff	/**< any contact */
/* open flags */
#define OC_OPEN				0x2000	/**< open condition flag (combined with at least 1 phase connections) */
#define OC_PHASE_A_OPEN		0x2001	/**< phase A connection open */
#define OC_PHASE_B_OPEN		0x2002	/**< phase B connection open */
#define OC_PHASE_C_OPEN		0x2004	/**< phase C connection open */
#define OC_NEUTRAL_OPEN		0x2008	/**< netural connection open */
#define OC_SPLIT_1_OPEN		0x2010	/**< triplex line 1 connection open */
#define OC_SPLIT_2_OPEN		0x2020	/**< triplex line 2 connection open */
#define OC_SPLIT_N_OPEN		0x2040	/**< triplex line 4 connection open */
#define OC_GROUND_OPEN		0x2080	/**< ground connection open */
#define OC_OPEN_ALL			0x20ff	/**< any connection open */
/* condition mask */
#define OC_INFO				0x3000	/**< all condition info */
#endif

/** Phases configuration indicators
	The phase indicators identify which phases a powerflow component
	is connected to.  Most combinations of bits are allowed, but some
	don't make sense (e.g., D|N, S|N)c
 **/
#define NO_PHASE	0x0000		/**< no phase info */
/* three phase configurations */
#define PHASE_A		0x0001		/**< A phase connection */
#define PHASE_B		0x0002		/**< B phase connection */
#define PHASE_C		0x0004		/**< C phase connection */
#define PHASE_ABC	0x0007		/**< three phases connection */
#define PHASE_N		0x0008		/**< N phase connected */
#define PHASE_ABCN	0x000f		/**< three phases neutral connection */
/* split phase configurations */
#define PHASE_S1	0x0010		/**< split line 1 connection */
#define PHASE_S2	0x0020		/**< split line 2 connection */
#define PHASE_SN	0x0040		/**< split line neutral connection */
#define PHASE_S		0x0070		/**< Split phase connection */
#define GROUND		0x0080		/**< ground line connection */
/* delta configuration */
#define PHASE_D		0x0100		/**< delta connection (requires ABCN) */
/* phase info mask */
#define PHASE_INFO	0x01ff		/**< all phase info */

/** Powerflow solution mode indicators
	The solution modes are used to indicate the solution needed for the powerflow
	object.  Whenever the object is in PS_NORMAL state, the powerflow solver
	will use the normal solution method.  PS_OUTAGE is used to indicate that
	there is no expectation the normal solution method will work.  Alternate solutions 
	methods may be indicated using other object-specified PS codes
 **/
#define PS_NORMAL	0x00		/**< normal powerflow solution */
#ifdef SUPPORT_OUTAGES
#define PS_OUTAGE	0x01		/**< outage powerflow solution */
#endif
/* Note: Only define solution modes that are known to ALL objects.
   Add object specific modes to the appropriate objects 
   */

class powerflow_object : public gld_object
{
public:
	set phases;				/**< device phases (see PHASE codes) */
	double nominal_voltage;	/**< nominal voltage */
#ifdef SUPPORT_OUTAGES
	set condition;			/**< operating condition (see OC codes) */
	enumeration solution;	/**< solution code (PS_NORMAL=0, class-specific solution mode code>0) */
#endif
public:
	static CLASS *oclass; 
	static CLASS *pclass;
#ifdef SUPPORT_OUTAGES
	/* is_normal checks whether the current operating condition is normal */
	inline bool is_normal(void) const { return condition==OC_NORMAL;};
	
	/* is_open checks whether particulars phases are open (no connection when there should be one) */
	inline bool is_open(set ph=PHASE_INFO) const { return (condition&OC_OPEN)!=0 && (condition&ph)==ph;};

	/* is_open_any checks whether any phase is open (no connection when there should be one) */
	inline bool is_open_any(set ph=PHASE_INFO) const { return (condition&OC_OPEN)!=0 && (condition&ph)!=0;};

	/* is_contact_any checks whether particulars phases are contacting another */
	inline bool is_contact(set ph=PHASE_INFO) const { return (condition&OC_CONTACT)!=0 && (condition&ph)==ph;};
	
	/* is_contact_any checks whether any phase is contacting another */
	inline bool is_contact_any(set ph=PHASE_INFO) const { return (condition&OC_CONTACT)!=0 && (condition&ph)!=0;};
#endif
	/* get_name acquires the name of an object or 'unnamed' if non set */
	inline const char *get_name(void) const { static char tmp[64]; OBJECT *obj=OBJECTHDR(this); return obj->name?obj->name:(sprintf(tmp,"%s:%d",obj->oclass->name,obj->id)>0?tmp:"(unknown)");};
	
	/* get_id acquires the object's id */
	inline unsigned int get_id(void) const {return OBJECTHDR(this)->id;};
	
	/* has_phase checks whether a particular phase is configured */
	inline bool has_phase(set ph, set flags=0) const { return ((flags?flags:phases)& ph)==ph; };
	
	/* phase_index returns the array index of a phase if configured (use flags to test another variable) */
	inline unsigned int phase_index(set ph=0) const { return ((unsigned int)((ph?ph:phases)&PHASE_ABC)>>2)%3;};
	
	/* get_phases acquires the phase part of the flags given */
	inline set get_phases(set flags) const { return flags&PHASE_INFO;};

	inline OBJECT* objecthdr() const { return OBJECTHDR(this);}
public:
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	powerflow_object(MODULE *mod);
	powerflow_object(CLASS *cl=oclass);
	int isa(char *classname);

	int kmldump(FILE *fp);
};

#endif // _POWERFLOWOBJECT_H
/**@}*/

