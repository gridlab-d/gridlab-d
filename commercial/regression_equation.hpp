/* 
 * File:   regression_equation.hpp
 * Author: CARNELLR
 *
 * Created on September 24, 2014, 2:01 PM
 */

#ifndef REGRESSIONEQUATION_H
#define	REGRESSIONEQUATION_H

#include <string>
#include <vector>
#include <math.h>
#include <istream>
#include <fstream>
#include <sstream>
#include <stdlib.h>

#define pow2(X) X*X
#define pow3(X) X*X*X
#define pow4(X) X*X*X*X
#define pow5(X) X*X*X*X*X

class RegressionEquation
{
public:
    RegressionEquation();
    RegressionEquation(int buildingType, int climateZone, int naicsCode);
    void setBuildingType(int buildingType);
    void setClimateZone(int climateZone);
    void setNaicsCode(int naicsCode);
    
    /**
     * 
     * @param temperature_C temperature in Celcius
     * @param dayofweek Sunday is 0 through Saturday is 6
     * @param hour midnight is 0 through 11:00 pm is 23
     * @param season spring is 0 through winter is 3
     * @return the load in kw/hr for the indicated hour
     */
    double calculate(double temperature_C, int dayofweek, int hour, int season, double annualAverageDailyUsage);
    
    std::vector<double> & getParameters()
    {
        return parameters;
    }
    
    std::vector<double> & getCoefficients()
    {
        return coefficients;
    }
    
	static void setFilename(std::string filepath);

    static const int SUMMER;
    static const int SPRING;
    static const int FALL;
    static const int WINTER;
    
private:
    void buildParameters(double temperature_C, int dayofweek, int hour, int season);
    static void readFile();
    void findCoefficients();

	std::vector<double> coefficients;
    static const int expectedNumberOfCoefficients;
    std::vector<double> parameters;
    static std::vector<std::vector<std::string> > csvContents;
    
    int m_buildingType;
    int m_climateZone;
    int m_naicsCode;
    static std::string m_filename;
};

#endif	/* SCEREGRESSIONEQUATION_H */

