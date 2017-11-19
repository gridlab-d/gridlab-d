#ifndef HVACMODEL_HPP_
#define HVACMODEL_HPP_

class HvacModel {
public:

private:
	double interiorExteriorCoupling;
	double interiorMassCoupling;
	double interiorHvacCoupling;
	double interiorSolarCoupling;
	double interiorGains;
	double massInteriorCoupling;
	double massExteriorCoupling;
	double massSolarCoupling;
	double heatSetpointC;
	double coolSetpointC;
	double coolingCapacity;
	double heatingCapacity;
};

#endif