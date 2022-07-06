#include "gridlabd.h"

#include <stdexcept>

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
			bool return_val = false;
			if (sort_by == 1) {  // sort by power increasing
				return_val = power < rhs.power;
			}
			else if (sort_by == 2) {  // sort by power decreasing
				return_val = power > rhs.power;
			}
			else if (sort_by == 3) { // sort by deviation of voltage from nominal
				return_val = voltage_deviation < rhs.voltage_deviation;
			}
			else if (sort_by == 4) { // sort by furtherst away from nominal in each direction
				return_val = voltage_deviation < rhs.voltage_deviation;
			}
			else {
				throw std::logic_error("sort method is not defined!"); // This fixes no-return state.
			}
			return return_val;
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
	void fetch_double(double **prop, const char *name, OBJECT *parent);
	void fetch_int(int **prop, const char *name, OBJECT *parent);
};

#endif
