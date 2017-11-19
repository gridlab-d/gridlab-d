#include "regression_equation.hpp"
#include "commercial_exception.hpp"
#include "commercial_library.hpp"
#include <iostream>
#include <sstream>

const int RegressionEquation::SUMMER = 1;
const int RegressionEquation::SPRING = 0;
const int RegressionEquation::FALL = 2;
const int RegressionEquation::WINTER = 3;
const int RegressionEquation::expectedNumberOfCoefficients = 2021;
std::vector<std::vector<std::string> > RegressionEquation::csvContents;
std::string RegressionEquation::m_filename = "";

RegressionEquation::RegressionEquation()
{
    m_buildingType = 0;
    m_climateZone = 0;
    m_naicsCode = 0;
    parameters = std::vector<double>(expectedNumberOfCoefficients);
    coefficients = std::vector<double>(expectedNumberOfCoefficients);
}

RegressionEquation::RegressionEquation(int buildingType, int climateZone, int naicsCode)
{
    m_buildingType = buildingType;
    m_climateZone = climateZone;
    m_naicsCode = naicsCode;
    parameters = std::vector<double>(expectedNumberOfCoefficients);
    coefficients = std::vector<double>(expectedNumberOfCoefficients);
	findCoefficients();
}

void RegressionEquation::setBuildingType(int buildingType)
{
    m_buildingType = buildingType;
}

void RegressionEquation::setClimateZone(int climateZone)
{
    m_climateZone = climateZone;
}

void RegressionEquation::setNaicsCode(int naicsCode)
{
    m_naicsCode = naicsCode;
}

void RegressionEquation::setFilename(std::string filepath) {
	if (filepath != m_filename) {
		m_filename = filepath;
		readFile();
	}
}

void RegressionEquation::readFile()
{
    if (m_filename.empty())
    {
		GL_THROW("Coefficients file not specified");
    }
    csvContents.clear();
    std::ifstream ifs(m_filename.c_str(), std::ifstream::in);
	if (!ifs.good()) {
		GL_THROW("Unable to access coefficients file");
	}

    while (ifs.good() && !ifs.eof())
    {

		std::string templine;
		std::vector<std::string> tempvector = std::vector<std::string>();

        std::getline(ifs, templine);
        std::stringstream ss(templine);
        while (ss.good())
        {
			std::string temp;
            std::getline(ss, temp, ',');

			tempvector.push_back(temp);
			temp.clear();
        }

        csvContents.push_back(tempvector);
        tempvector.clear();
    }

	double stop=1;
}

void RegressionEquation::findCoefficients()
{
    if (m_climateZone <= 0 || m_buildingType < 0)
    {
		std::cout << "Invalid Climate zone (" << m_climateZone << ") and/or building type (" << m_buildingType << ")" << std::endl
			<< "Climate zone must be greater than 0; Building type must be greater than or equal to 0";
        return;
    }
    
	bool found = false;
    if (m_naicsCode > 0)
    {
        // find the row with buildingType and ClimateZone and naicsCode and put the numbers into coefficients
        std::vector<std::vector<std::string> >::const_iterator i;
        std::vector<std::string>::const_iterator j;
        std::vector<double>::iterator k;
        // +1 to skip the header
        for (i = csvContents.begin()+1; i != csvContents.end(); ++i)
        {
            if ((*i).size() == expectedNumberOfCoefficients + 4 && 
                    atoi((*i)[1].c_str()) == m_climateZone && 
                    atoi((*i)[2].c_str()) == m_buildingType && 
                    atoi((*i)[3].c_str()) == m_naicsCode)
            {
				for (j = (*i).begin() + 4, k = coefficients.begin(); j != (*i).end(), k != coefficients.end(); ++j, ++k)
				{
					(*k) = atof((*j).c_str());
				}
				found = true;
				break;
            }
        }
    }

	if (!found)
	{
		std::cout << "Can't find coefficients for NAICS code " << m_naicsCode << ". Using default values for building type " << m_buildingType << " and climate zone " << m_climateZone << "." << std::endl;
		//gl_warning("Naics code %d unknown or unspecified. Using default values for building type %d and climate zone %d.", m_naicsCode, m_buildingType, m_climateZone);
        // find the row with the buidlingType and climateZone and naicsCode = NA and put the numbers into coefficients
        std::vector<std::vector<std::string> >::const_iterator i;
        std::vector<std::string>::const_iterator j;
        std::vector<double>::iterator k;
        for (i = csvContents.begin()+1; i != csvContents.end(); ++i)
        {
            if ((*i).size() == expectedNumberOfCoefficients + 4)
            {
                std::string sClimateZone = (*i)[1];
                std::string sBuildingType = (*i)[2];
                if (atoi(sClimateZone.c_str()) == m_climateZone && 
                        atoi(sBuildingType.c_str()) == m_buildingType &&
                        (*i)[3] == "NA")
                {
                    for (j = (*i).begin() + 4, k = coefficients.begin(); j != (*i).end(), k != coefficients.end(); ++j, ++k)
                    {
                        (*k) = atof((*j).c_str());
                    }
					found = true;
                    break;
                }
            }
        }
    }

	if (!found) {
		std::ostringstream message;
		message << "No coefficients found for climate zone " << m_climateZone 
			<< " and building type " << m_buildingType;
		std::cout << message.str() << std::endl;
	}
}

double RegressionEquation::calculate(double temperature_C, int dayofweek, int hour, int season, double annualAverageDailyUsage)
{
    buildParameters(temperature_C, dayofweek, hour, season);
    std::vector<double>::iterator i;
    std::vector<double>::iterator j;
    double sum = 0.0;
    for (i = parameters.begin(), j = coefficients.begin(); i != parameters.end(), j != coefficients.end(); ++i, ++j)
    {
        sum += (*i)*(*j);
    }
    return exp(sum) * annualAverageDailyUsage;
}

void RegressionEquation::buildParameters(double temperature_C, int dayofweek, int hour, int month)
{
	int nMonths = 11;
    int nDays = 6;
    int nHours = 23;
    
    parameters.assign(parameters.size(), 0.0);
    
    int count = 0;
    parameters[count] = 1.0; // intercept
    if (hour != 0)
    {
        parameters[count + hour] = 1.0;
    }
    count += nHours;
    if (month != 0)
    {
        parameters[count + month] = 1.0;
    }
    count += nMonths;
    if (dayofweek != 0)
    {
        parameters[count + dayofweek] = 1.0;
    }
    count += nDays;
    parameters[++count] = temperature_C;
    parameters[++count] = pow2(temperature_C);
    parameters[++count] = pow3(temperature_C);
    parameters[++count] = pow4(temperature_C);
    parameters[++count] = pow5(temperature_C);

    if (hour != 0 && month != 0)
    {
        parameters[count + (month - 1)*nHours + hour] = 1.0;
    }
    count += nHours * nMonths;
    
    if (hour != 0 && dayofweek != 0)
    {
        parameters[count + (dayofweek - 1)*nHours + hour] = 1.0;
    }
    count += nHours * nDays;
    
    if (month != 0 && dayofweek != 0)
    {
        parameters[count + (dayofweek - 1)*nMonths + month] = 1.0;
    }
    count += nMonths * nDays;
    
    if (hour != 0 && dayofweek != 0 && month != 0)
    {
        parameters[count + (dayofweek - 1)*nMonths*nHours + (month - 1)*nHours + hour] = 1.0;
    }
}
