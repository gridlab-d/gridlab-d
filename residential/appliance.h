/** $Id: appliance.h 4738 2014-07-03 00:55:39Z dchassin $
    Copyright (C) 2012 Battelle Northwest
 **/

#ifndef _APPLIANCE_H
#define _APPLIANCE_H

#include "residential_enduse.h"

class appliance : public residential_enduse
{
private:
	//GL_STRUCT(Eigen::MatrixXcd,power);
	//GL_STRUCT(Eigen::MatrixXcd,impedance);
	//GL_STRUCT(Eigen::MatrixXcd,current);
	//GL_STRUCT(Eigen::MatrixXd,duration);
	//GL_STRUCT(Eigen::MatrixXd,transition);
	//GL_STRUCT(Eigen::MatrixXd,heatgain);
private:
	TIMESTAMP next_t;
	unsigned int n_states;
	unsigned int state;
	double *transition_probabilities;
private:
	void update_next_t(void);
	void update_power(void);
	void update_state(void);
	int shared_init(OBJECT *parent); ///<Shared initialization for non-published variables used by both checkpoint_init and init
public:
	appliance(MODULE *module);
	~appliance() { if (defaults) delete defaults; }
	int create();
	int init(OBJECT *parent);
	int checkpoint_init(OBJECT *parent);
	int isa(char *classname);
	int precommit(TIMESTAMP t1);
	inline TIMESTAMP presync(TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t1) { return TS_NEVER; };
	inline TIMESTAMP postsync(TIMESTAMP t1) { return TS_NEVER; };
	int prenotify(PROPERTY *prop, char *value){ return 1;} ;
	int postnotify(PROPERTY *prop, char *value);
public:
	static CLASS *oclass, *pclass;
	static appliance *defaults;

public:
	static inline appliance* get_defaults() {
		if (!defaults) {
			defaults = new appliance(); // Initialize lazily
		}
		return defaults;
	}

	appliance() {}

protected:
	Eigen::MatrixXcd power;  // Member variable of type `Eigen::MatrixXcd`.

public:
	// Static inline method to calculate the byte offset of the member `power`.
	static inline size_t get_power_offset(void) {
		appliance* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->power)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Inline function to get the value of `power` with thread safety.
	inline Eigen::MatrixXcd get_power(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx); // Shared lock for read access
		return power;
	}

	// Inline method to return a `gld_property` object for `power`.
	inline gld_property get_power_property(void) {
		return gld_property(my(), std::string("power").c_str());
	}

	// Inline method to set the `power` property directly using an `Eigen::MatrixXcd` object.
	inline void set_power(const Eigen::MatrixXcd& p) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx); // Exclusive lock for write access
		power = p;
	}
protected:
	Eigen::MatrixXcd impedance;  // Member variable of type `Eigen::MatrixXcd`.

public:
	// Static inline method to calculate the byte offset of the member `impedance`.
	static inline size_t get_impedance_offset(void) {
		appliance* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->impedance)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Inline function to get the value of `impedance` with thread safety.
	inline Eigen::MatrixXcd get_impedance(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx); // Shared lock for read access
		return impedance;
	}

	// Inline method to return a `gld_property` object for `impedance`.
	inline gld_property get_impedance_property(void) {
		return gld_property(my(), std::string("impedance").c_str());
	}

	// Inline method to set the `impedance` property directly using an `Eigen::MatrixXcd` object.
	inline void set_impedance(const Eigen::MatrixXcd& p) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx); // Exclusive lock for write access
		impedance = p;
	}
protected:
	Eigen::MatrixXcd current;  // Member variable of type `Eigen::MatrixXcd`.

public:
	// Static inline method to calculate the byte offset of the member `current`.
	static inline size_t get_current_offset(void) {
		appliance* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->current)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Inline function to get the value of `current` with thread safety.
	inline Eigen::MatrixXcd get_current(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx); // Shared lock for read access
		return current;
	}

	// Inline method to return a `gld_property` object for `current`.
	inline gld_property get_current_property(void) {
		return gld_property(my(), std::string("current").c_str());
	}

	// Inline method to set the `current` property directly using an `Eigen::MatrixXcd` object.
	inline void set_current(const Eigen::MatrixXcd& p) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx); // Exclusive lock for write access
		current = p;
	}
protected:
	Eigen::MatrixXd duration;  // Member variable of type `Eigen::MatrixXd`.

public:
	// Static inline method to calculate the byte offset of the member `duration`.
	static inline size_t get_duration_offset(void) {
		appliance* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->duration)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Inline function to get the value of `duration` with thread safety.
	inline Eigen::MatrixXd get_duration(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx); // Shared lock for read access
		return duration;
	}

	// Inline method to return a `gld_property` object for `duration`.
	inline gld_property get_duration_property(void) {
		return gld_property(my(), std::string("duration").c_str());
	}

	// Inline method to set the `duration` property directly using an `Eigen::MatrixXd` object.
	inline void set_duration(const Eigen::MatrixXd& p) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx); // Exclusive lock for write access
		duration = p;
	}
protected:
	Eigen::MatrixXd transition;  // Member variable of type `Eigen::MatrixXd`.

public:
	// Static inline method to calculate the byte offset of the member `transition`.
	static inline size_t get_transition_offset(void) {
		appliance* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->transition)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Inline function to get the value of `transition` with thread safety.
	inline Eigen::MatrixXd get_transition(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx); // Shared lock for read access
		return transition;
	}

	// Inline method to return a `gld_property` object for `transition`.
	inline gld_property get_transition_property(void) {
		return gld_property(my(), std::string("transition").c_str());
	}

	// Inline method to set the `transition` property directly using an `Eigen::MatrixXd` object.
	inline void set_transition(const Eigen::MatrixXd& p) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx); // Exclusive lock for write access
		transition = p;
	}
protected:
	Eigen::MatrixXd heatgain;  // Member variable of type `Eigen::MatrixXd`.

public:
	// Static inline method to calculate the byte offset of the member `heatgain`.
	static inline size_t get_heatgain_offset(void) {
		appliance* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&(current_defaults->heatgain)) -
			reinterpret_cast<const char*>(current_defaults);
	}

	// Inline function to get the value of `heatgain` with thread safety.
	inline Eigen::MatrixXd get_heatgain(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx); // Shared lock for read access
		return heatgain;
	}

	// Inline method to return a `gld_property` object for `heatgain`.
	inline gld_property get_heatgain_property(void) {
		return gld_property(my(), std::string("heatgain").c_str());
	}

	// Inline method to set the `heatgain` property directly using an `Eigen::MatrixXd` object.
	inline void set_heatgain(const Eigen::MatrixXd& p) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx); // Exclusive lock for write access
		heatgain = p;
	}


};

#endif
