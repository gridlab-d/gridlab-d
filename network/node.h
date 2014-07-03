/** $Id: node.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 **/

#ifndef _NODE_H
#define _NODE_H

#include "link.h"

typedef struct s_linklist {
	class link *data;
	struct s_linklist *next;
} LINKLIST;

typedef enum {PQ=0, PQV=1, PV=2, SWING=3} BUSTYPE;
class node {
public:
	complex V; /* voltage */
	complex S; /* power injection */
	double G; /* shunt conductance */
	double B; /* shunt susceptance */
	double Qmax_MVAR; /* maximum MVAR limit */
	double Qmin_MVAR; /* minimum MVAR limit */
	BUSTYPE type; /* 1=PQ, 2=PV, 3=SWING */
	int16 flow_area_num; /* load flow area */
	int16 loss_zone_num; /* loss zone number */
	double base_kV; /* base kV */
	double desired_kV; /* desired kV of remote bus */
	OBJECT *remote_bus_id; /* remote bus */
	LINKLIST *linklist;

	/* these values are used by state estimation */
	complex Vobs;	/* observed voltage */
	double Vstdev;	/* observed voltage standard deviation; 0 means no observation */

protected:
	complex Ys; /* self admittance (on-diagonal)*/
	complex YVs; /* load/gen partials (off-diagonals x remote V) */
	double r2; /* sum of R^2 of observations residuals */
	unsigned short n_obs; /* number of observations made */
	double Sr2; /* power residuals */
	unsigned short n_inj;
public:
	static CLASS *oclass;
	static node *defaults;
	static CLASS *pclass;
public:
	node(MODULE *mod);
	~node();
	int create();
	int init(OBJECT *parent);

	void attach(link *pLink);

	void add_obs_residual(node *pNode);
	void del_obs_residual(node *pNode);
	double get_obs_residual(void) const;
	double get_obs_probability(void) const;

	void add_inj_residual(node *pNode);
	void del_inj_residual(node *pNode);
	double get_inj_residual(void) const;
	double get_inj_probability(void) const;

	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	friend class link;
};

GLOBAL CLASS *node_class INIT(NULL);
GLOBAL OBJECT *last_node INIT(NULL);

#endif
