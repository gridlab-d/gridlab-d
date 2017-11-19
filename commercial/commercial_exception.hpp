#ifndef COMMERCIAL_EXCEPTION_HPP_
#define COMMERCIAL_EXCEPTION_HPP_

#include <exception>
#include <stdexcept>
#include <string>

class CommercialException: public std::runtime_error {
	int errorCode;
public:
	CommercialException(int errorCode, std::string message): std::runtime_error(message) {
		this->errorCode = errorCode;
	}

	int getErrorCode() {
		return errorCode;
	}
};

#endif