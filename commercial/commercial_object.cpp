#include "commercial_object.hpp"
#include "commercial_exception.hpp"
#include "commercial_library.hpp"
#include <iostream>

CommercialObject::CommercialObject(int climateZone, int buildingType, int naicsCode, double usage) {
	equation = RegressionEquation(buildingType, climateZone, naicsCode);
	hvacMode = HVACMODE_SIMPLE;
	this->averageUsage = usage;
}

double CommercialObject::calculate(double outdoorTemp, int dayofweek, int hour, int season) {
	double totalLoad = 0.0;
	double temp = outdoorTemp;
	if (hvacMode != HVACMODE_SIMPLE) {
		temp = 18.0; // 18C should get the base load only, no HVAC
	} 
	totalLoad += equation.calculate(temp, dayofweek, hour, season, averageUsage);

	return totalLoad;
}

void CommercialObject::advance(TIMESTAMP t0) {
}

void CommercialObject::release() {
	delete this;
}

void CommercialObject::setHvacMode(int mode) {
	hvacMode = mode;
}

void CommercialObject::setAvgUsage(double usage) {
	averageUsage = usage;
}

const char* CommercialObject::getErrorMessage() {
	return errorMessage.c_str();
}

void CommercialObject::setErrorMessage(std::string message) {
	this->errorMessage = message;
}

//**** API FUNCTIONS

ICommercialObject* APIENTRY getCommercialObject(int climateZone, int buildingType, int naicsCode, double usage) {
	return new CommercialObject(climateZone, buildingType, naicsCode, usage);
}

int APIENTRY setCoefficientPath(char* filepath) {
	try {
		RegressionEquation::setFilename(std::string(filepath));
	} catch (CommercialException ex) {
		return ex.getErrorCode();
	}
	return COMMERCIAL_SUCCESS;
}
