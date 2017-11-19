#pragma once
#ifndef COMMERCIAL_LIBRARY_HPP_
#define COMMERCIAL_LIBRARY_HPP_

#if 0
// see if we can skip this stuff...
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <WinDef.h>

#endif

// I'm sure there's something more appropriate for this. -MH
#ifndef APIENTRY
#define APIENTRY
#endif

#include <gridlabd.h>
#include <string>

#ifdef COMMERCIAL_LIBRARY_EXPORTS
#define COMMERCIALAPI __declspec(dllexport)
#else
#define COMMERCIALAPI __declspec(dllimport)
#endif

#define HVACMODE_SIMPLE 0
#define HVACMODE_ADVANCED 1

#define SEASON_SPRING 0
#define SEASON_SUMMER 1
#define SEASON_FALL 2
#define SEASON_WINTER 3

#define COMMERCIAL_SUCCESS 0
#define COMMERCIAL_COEFFICIENTS_ACCESS_DENIED 1
#define COMMERCIAL_COEFFICIENTS_FILE_NOT_SPECIFIED 2
#define COMMERCIAL_INVALID_BUILDING_TYPE 3
#define COMMERCIAL_INVALID_CLIMATE_ZONE 4
#define COMMERCIAL_UNKNOWN_NAICS_CODE 5
#define COMMERCIAL_COEFFICIENTS_NOT_FOUND 6

class ICommercialObject {
public:
	virtual double calculate(double outdoorTemp, int dayofweek, int hour, int season) = 0;
	virtual void release() = 0;

	virtual void setHvacMode(int mode) = 0;
	virtual void setAvgUsage(double usage) = 0;
};

//extern "C" COMMERCIALAPI ICommercialObject* APIENTRY getCommercialObject(int climateZone, int buildingType, int naicsCode, double usage);
//extern "C" COMMERCIALAPI void APIENTRY setCoefficientPath(char* filepath);
EXPORT ICommercialObject* APIENTRY getCommercialObject(int climateZone, int buildingType, int naicsCode, double usage);
EXPORT int APIENTRY setCoefficientPath(char* filepath);

#endif