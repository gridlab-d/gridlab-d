// header stuff here

#ifndef _VIOLATION_RECORDER_H_
#define _VIOLATION_RECORDER_H_

#include "tape.h"
#include "../powerflow/transformer_configuration.h"
#include "../powerflow/transformer.h"
#include "../powerflow/line.h"
#include <new>

EXPORT void new_violation_recorder(MODULE *);

#ifdef __cplusplus

#define VIOLATION0		0x00
#define VIOLATION1		0x01
#define VIOLATION2		0x02
#define VIOLATION3		0x04
#define VIOLATION4		0x08
#define VIOLATION5		0x10
#define VIOLATION6		0x20
#define VIOLATION7		0x40
#define VIOLATION8		0x80
#define ALLVIOLATIONS	0xFF

#define XFMR		1
#define OHLN		2
#define UGLN		3
#define TPXL		4
#define NODE		5
#define TPXN		6
#define TPXM		7
#define INVT		8
#define CMTR		9

/** Phases configuration indicators
	The phase indicators identify which phases a powerflow component
	is connected to.  Most combinations of bits are allowed, but some
	don't make sense (e.g., D|N, S|N)c

	STOLEN FROM POWERFLOW.h

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

#define AR 0x002
#define BR 0x020
#define CR 0x200

#define POWER		0x01
#define CURRENT		0x02

#define sign(x) ((x > 0) - (x < 0))
#define l2(x) (log((double) x) / log(2.0))

//class quickobjlist{
//public:
//	quickobjlist(){obj = 0; next = 0; memset(&prop, 0, sizeof(PROPERTY));}
//	quickobjlist(OBJECT *o, PROPERTY *p){obj = o; next = 0; memcpy(&prop, p, sizeof(PROPERTY));}
//	~quickobjlist(){if(next != 0) delete next;}
//	void tack(OBJECT *o, PROPERTY *p){if(next){next->tack(o, p);} else {next = new quickobjlist(o, p);}}
//	OBJECT *obj;
//	PROPERTY prop;
//	quickobjlist *next;
//};

class vobjlist{
public:
	vobjlist(){
		obj = 0;
		next = 0; 
		ref_obj=0;
		for( int i = 0; i < 3; i++ ){
			last_v[i] = 0;
			last_t[i] = 0;
			last_s[i] = 0;
		}
	}
	vobjlist(OBJECT *o){
		obj = o; 
		next = 0; 
		ref_obj=0;
		for( int i = 0; i < 3; i++ ){
			last_v[i] = 0;
			last_t[i] = 0;
			last_s[i] = 0;
		}
	}
	~vobjlist(){if(next != 0) delete next;}
	void tack(OBJECT *o) {
		if (obj) {
			if(next){
				next->tack(o);
			} else {
				//Core-callback allocation - replicates the function, but done manually here
				next = (vobjlist *)gl_malloc(sizeof(vobjlist));

				if (next == NULL)
				{
					GL_THROW("violation_recorder - Failed to allocate space for object list to check");
					/*  TROUBLESHOOT
					While attempting to allocate the memory for an object list within the violation recorder, an error occurred.  Please check your
					file and try again.  If the error persists, please submit you code and a bug report via the ticketing system.
					*/
				}

				//"Constructor"
				new (next) vobjlist(o);
			}
		} else {
			obj = o;
		}
	}
	void add_ref(OBJECT *o) {
		ref_obj = o;
	}
	int length() {
		if(next){
			return 1+next->length();
		}
		if (obj) {
			return 1;
		}
		return 0;
	}
	void update_last(int i, double v, TIMESTAMP t, int s) { // ugh.. this is a bit ugly
		if (i>2 || i<0)
			return;
		last_v[i] = v;
		last_t[i] = t;
		last_s[i] = s;
	}
	OBJECT *obj;
	vobjlist *next;
	OBJECT *ref_obj;
	double last_v[3];
	TIMESTAMP last_t[3];
	int last_s[3];

};

class uniqueList{
public:
	uniqueList(){
		name = 0;
		next = 0;
	}
	uniqueList(char *n){
		name = n;
		next = 0;
	}
	~uniqueList(){if(next != 0) delete next;}
	void insert(char *n) {
		if (name && strcmp(name, n) != 0) {
			if(next){
				next->insert(n);
			} else {
				//Core-callback allocation - replicates the function, but done manually here
				next = (uniqueList *)gl_malloc(sizeof(uniqueList));

				if (next == NULL)
				{
					GL_THROW("violation_recorder - Failed to allocate space for unique list");
					/*  TROUBLESHOOT
					While attempting to allocate the memory for a list of unique objects within the violation recorder, an error occurred.  Please check your
					file and try again.  If the error persists, please submit you code and a bug report via the ticketing system.
					*/
				}

				//"Constructor"
				new (next) uniqueList(n);
			}
		} else {
			name = n;
		}
	}
	int length() {
		if(next){
			return 1+next->length();
		}
		if(name) {
			return 1;
		}
		return 0;
	}
	char *name;
	uniqueList *next;
};

class violation_recorder{
public:
	static violation_recorder *defaults;
	static CLASS *oclass, *pclass;

	violation_recorder(MODULE *);
	int create();
	int init(OBJECT *);
	int finalize(OBJECT *obj);
	int isa(char *);
	TIMESTAMP postsync(TIMESTAMP, TIMESTAMP);

	int commit(TIMESTAMP);
public:
	double dInterval;
	double dFlush_interval;
	int32 limit;
	char256 filename;
	char256 summary;
	char256 virtual_substation;
	bool strict;
	set violation_flag;
	bool echo;
	double xfrmr_thermal_limit_upper;
	double xfrmr_thermal_limit_lower;
	double line_thermal_limit_upper;
	double line_thermal_limit_lower;
	double node_instantaneous_voltage_limit_upper;
	double node_instantaneous_voltage_limit_lower;
	double node_continuous_voltage_limit_upper;
	double node_continuous_voltage_limit_lower;
	double node_continuous_voltage_interval;
	double secondary_dist_voltage_rise_upper_limit;
	double secondary_dist_voltage_rise_lower_limit;
	double substation_breaker_A_limit;
	double substation_breaker_B_limit;
	double substation_breaker_C_limit;
	double substation_pf_lower_limit;
	int violation_count[8][10];
	double inverter_v_chng_per_interval_upper_bound;
	double inverter_v_chng_per_interval_lower_bound;
	double inverter_v_chng_interval;
	TIMESTAMP violation_start_delay;
private:
	int write_header();
	int flush_line();
	int write_footer();
	int pass_error_check();
	int check_violations(TIMESTAMP);
	int check_violation_1(TIMESTAMP);
	int check_violation_2(TIMESTAMP);
	int check_violation_3(TIMESTAMP);
	int check_violation_4(TIMESTAMP);
	int check_violation_5(TIMESTAMP);
	int check_violation_6(TIMESTAMP);
	int check_violation_7(TIMESTAMP);
	int check_violation_8(TIMESTAMP);
	int check_line_thermal_limit(TIMESTAMP, vobjlist *, uniqueList *, int, double, double);
	int check_xfrmr_thermal_limit(TIMESTAMP, vobjlist *, uniqueList *, int, double, double);
	int check_reverse_flow_violation(TIMESTAMP, int, double, char*);
	int write_to_stream (TIMESTAMP, bool, char *, ...);
	double get_observed_double_value(OBJECT *, PROPERTY *);
	complex get_observed_complex_value(OBJECT *, PROPERTY *);
	int make_object_list(int, char *, vobjlist *);
	int assoc_meter_w_xfrmr_node(vobjlist *, vobjlist *, vobjlist *);
	int find_substation_node(char256, vobjlist *);
	int has_phase(OBJECT *, int);
	int fails_static_condition (OBJECT *, char *, double, double, double, double *);
	int fails_static_condition (double, double, double, double, double *);
	int fails_dynamic_condition (vobjlist *, int, char *, TIMESTAMP, double, double, double, double, double *);
	int fails_continuous_condition (vobjlist *, int, char *, TIMESTAMP, double, double, double, double, double *);
	int increment_violation (int);
	int increment_violation (int, int);
	int get_violation_count(int);
	int get_violation_count(int, int);
	int write_summary();
	//Memory allocation functions - functionalized for ease of use (copy-paste-itis)
	vobjlist *vobjlist_alloc_fxn(vobjlist *input_list);
	uniqueList *uniqueList_alloc_fxn(uniqueList *input_unlist);
private:
	FILE *rec_file;
//	quickobjlist *xfrmr_phase_a_obj_list;
//	quickobjlist *xfrmr_phase_b_obj_list;
//	quickobjlist *xfrmr_phase_c_obj_list;
//	quickobjlist *ohl_phase_a_obj_list;
//	quickobjlist *ohl_phase_b_obj_list;
//	quickobjlist *ohl_phase_c_obj_list;
//	quickobjlist *ugl_phase_a_obj_list;
//	quickobjlist *ugl_phase_b_obj_list;
//	quickobjlist *ugl_phase_c_obj_list;
//	quickobjlist *tplxl_phase_a_obj_list;
//	quickobjlist *tplxl_phase_b_obj_list;
//	quickobjlist *tplxl_phase_c_obj_list;

	vobjlist *xfrmr_obj_list;
	vobjlist *ohl_obj_list;
	vobjlist *ugl_obj_list;
	vobjlist *tplxl_obj_list;
	vobjlist *node_obj_list;
	vobjlist *tplx_node_obj_list;
	vobjlist *tplx_mtr_obj_list;
	vobjlist *comm_mtr_obj_list;
	vobjlist *inverter_obj_list;
	vobjlist *powerflow_obj_list;
	OBJECT *link_monitor_obj;
	uniqueList *xfrmr_list_v1;
	uniqueList *ohl_list_v1;
	uniqueList *ugl_list_v1;
	uniqueList *tplxl_list_v1;
	uniqueList *node_list_v2;
	uniqueList *tplx_node_list_v2;
	uniqueList *tplx_meter_list_v2;
	uniqueList *comm_meter_list_v2;
	uniqueList *tplx_node_list_v3;
	uniqueList *tplx_meter_list_v3;
	uniqueList *comm_meter_list_v3;
	uniqueList *inverter_list_v6;
	uniqueList *tplx_meter_list_v7;
	uniqueList *comm_meter_list_v7;

	int write_count;
	TIMESTAMP next_write;
	TIMESTAMP last_write;
	TIMESTAMP last_flush;
	TIMESTAMP write_interval;
	TIMESTAMP flush_interval;
	int32 write_ct;
	TAPESTATUS tape_status; // TS_INIT/OPEN/DONE/ERROR
	char *prev_line_buffer;
	char *line_buffer;
	size_t line_size;
	bool interval_write;
	TIMESTAMP sim_start;
};

#endif // C++

#endif // _VIOLATION_RECORDER_H_

// EOF
