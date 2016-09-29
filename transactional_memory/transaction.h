/* transaction.h
 *	Copyright (C) 2008 Battelle Memorial Institute
 */

#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "complex.h"
#include "property.h"


class AbsTransaction {
		//interface definition for Transactions
	public:
		virtual void executeTransaction() =0;
		virtual void *getOldPtr() =0;
		virtual ~AbsTransaction();
};

template<typename T>
class Transaction: public AbsTransaction {
	protected:
		T* varNew;
		T* varOld;
		uint32_t count;
	public:
		Transaction(T* varOld, T* varNew, uint32 count) {
			this->varOld = varOld;
			this->varNew = varNew;
			this->count = count;
		}
		virtual void *getOldPtr() {
			return reinterpret_cast<void *>(varOld);
		}
};

//TODO: Memcopy might be faster!
template<typename T>
class ReplaceTransaction: public Transaction<T> {
	protected:
		using Transaction<T>::varNew;
		using Transaction<T>::count;
		using Transaction<T>::varOld;
	public:
		ReplaceTransaction(T *varOld, T *varNew, uint32 size = 1) :
				Transaction<T>(varOld, varNew, size) {
		}
		;

		virtual void executeTransaction() {
			//T* oldValPtr=reinterpret_cast<T*>(given);
			for (int i = 0; i < count; i++) {
				varOld[i] = varNew[i];
			}
		}

		virtual ~ReplaceTransaction() {
		}
};

template<typename T>
class ReplaceAndDeleteTransaction: public ReplaceTransaction<T> {
	protected:
		using ReplaceTransaction<T>::varNew;
		using ReplaceTransaction<T>::count;
		using ReplaceTransaction<T>::varOld;
	public:
		ReplaceAndDeleteTransaction(T *varOld, T *varNew, uint32 size = 1) :
				ReplaceTransaction<T>(varOld, varNew, size) {
		}
		;

		virtual ~ReplaceAndDeleteTransaction() {
			if (count > 1)
				delete[] varOld;
			else
				delete varOld;
		}
};

template<typename T>
class AddTransaction: public Transaction<T> {
	protected:
		using Transaction<T>::varNew;
		using Transaction<T>::count;
		using Transaction<T>::varOld;
	public:
		AddTransaction(T *varOld, T *varNew, uint32 size = 1) :
				Transaction<T>(varOld, varNew, size) {
		}
		;
		virtual void executeTransaction() {
			//T* oldValPtr=reinterpret_cast<T*>(given);
			for (int i = 0; i < count; i++) {
				varOld[i] += varNew[i];
			}
		}
		virtual ~AddTransaction() {
		}
};

template<typename T>
class SubtractTransaction: public Transaction<T> {
	protected:
		using Transaction<T>::varNew;
		using Transaction<T>::count;
		using Transaction<T>::varOld;
	public:
		SubtractTransaction(T *varOld, T *varNew, uint32 size = 1) :
				Transaction<T>(varOld, varNew, size) {
		}
		;
		virtual void executeTransaction() {
			//T* oldValPtr=reinterpret_cast<T*>(given);
			for (int i = 0; i < count; i++) {
				varOld[i] -= varNew[i];
			}
		}
		virtual ~SubtractTransaction() {
		}
};

class ComplexTransaction: public Transaction<complex> {
		//place holder for transactions with complex type
	public:
		ComplexTransaction(complex* varOld, complex *varNew, int count) :
				Transaction<complex>(varOld, varNew, count) {
		}
		;
};

//TODO: memcpy might be faster
class ReplaceComplex: public ComplexTransaction {

	public:
		ReplaceComplex(complex* varOld, complex* varNew, uint32 count = 1) :
				ComplexTransaction(varOld, varNew, count) {
		}
		;
		virtual void executeTransaction();
};

class ReplaceComplexAndDelete: public ComplexTransaction {

	public:
		ReplaceComplexAndDelete(complex* varOld, complex* varNew, uint32 count =
				1) :
				ComplexTransaction(varOld, varNew, count) {
		}
		;
		virtual void executeTransaction();
		virtual ~ReplaceComplexAndDelete();
};

#endif // TRANSACTION_H
