#include "gridlabd.h"

#ifndef _collect_h_
#define _collect_h_

/** Supply/Demand collect */
class collect {
public:
	typedef struct supervisor_bid {
		int sort_by;	/**< determines which member to sort by */
		OBJECT *from;	/**< object from which bid was received */
		double power;	/**< bid power */
		double voltage_deviation;	/**< bid voltage deviation */
		int state;	/**< bid state */
		bool operator<(const supervisor_bid& rhs) const {
			if (sort_by == 1) {  // sort by power increasing
				return power < rhs.power;
			}
			else if (sort_by == 2) {  // sort by power decreasing
				return power > rhs.power;
			}
			else if (sort_by == 3) { // sort by deviation of voltage from nominal
				return voltage_deviation < rhs.voltage_deviation; 
			}
			else if (sort_by == 4) { // sort by furtherst away from nominal in each direction
				return voltage_deviation < rhs.voltage_deviation; 
			}
			else {
				gl_error("sort method is not defined!");
			}
		}
	} SUPERVISORBID;
	collect(void);
	~collect(void);
	void clear(void);
	int submit_on(OBJECT *from, double power, double voltage_deviation, int key, int state);
	int submit_off(OBJECT *from, double power, double voltage_deviation, int key, int state);
	int calculate_freq_thresholds(double droop, double nom_freq, double frequency_deadband, int PFC_mode);
	int sort(int sort_mode);
private:
	int len_on;
	int len_off;
	int number_of_bids_on;
	int number_of_bids_off;
	SUPERVISORBID *supervisor_bid_on;
	SUPERVISORBID *supervisor_bid_off;
	void fetch_double(double **prop, char *name, OBJECT *parent);
	void fetch_int(int **prop, char *name, OBJECT *parent);
};

#endif