#include "commercial_library.hpp"
#include "regression_equation.hpp"

#ifndef COMMERCIAL_OBJECT_HPP_
#define COMMERCIAL_OBJECT_HPP_

class CommercialObject: public ICommercialObject {
public:
	CommercialObject(int climateZone, int buildingType, int naicsCode, double averageUsage);

	virtual double calculate(double outdoorTemp, int dayofweek, int hour, int season);
	virtual void advance(TIMESTAMP t0);
	virtual void release();

	virtual void setHvacMode(int mode);
	virtual void setAvgUsage(double usage);
	virtual const char* getErrorMessage();
private:
	CommercialObject() {}
	double internalTemp;
	double power;
	double averageUsage;
	RegressionEquation equation;
	int hvacMode;
	std::string errorMessage;

	void setErrorMessage(std::string message);
};

#endif