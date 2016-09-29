/* transaction.cpp
 *	Copyright (C) 2008 Battelle Memorial Institute
 */
#include "transaction.h"

AbsTransaction::~AbsTransaction(){
//nothing to do here
}

void ReplaceComplex::executeTransaction(){

	for(int i=0;i<count;i++){
		  varOld[i]=varNew[i];
	 }
}

void ReplaceComplexAndDelete::executeTransaction(){

	for(int i=0;i<count;i++){
			  varOld[i]=varNew[i];
		 }

}

ReplaceComplexAndDelete::~ReplaceComplexAndDelete(){
	if(this->count >1)
		delete[] varNew;
	else
		delete varNew;
}

