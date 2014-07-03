#include "curve.h"

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF BID CURVE
//////////////////////////////////////////////////////////////////////////

curve::curve(void)
{
	len = 0;
	bids = NULL;
	keys = NULL;
	n_bids = 0;
	total = 0;
}

curve::~curve(void)
{
	delete [] bids;
	delete [] keys;
}

void curve::clear(void)
{
	n_bids = 0;
	total = 0;
	total_on = 0;
	total_off = 0;
}

BID *curve::getbid(KEY n)
{
	return bids+keys[n];
}

KEY curve::submit(BID *bid)
{
	if (len==0) // create the bid list
	{
		len = 8;
		bids = new BID[len];
		keys = new KEY[len];
	}
	else if (n_bids==len) // grow the bid list
	{
		BID *newbids = new BID[len*2];
		KEY *newkeys = new KEY[len*2];
		memcpy(newbids,bids,len*sizeof(BID));
		memcpy(newkeys,keys,len*sizeof(KEY));
		delete[] bids;
		delete[] keys;
		bids = newbids;
		keys = newkeys;
		len*=2;
	}
	keys[n_bids] = n_bids;
	BID *next = bids + n_bids;
	*next = *bid;

	/* handle bid state */
	switch (bid->state) {
	case BS_OFF:
		total_off += bid->quantity;
		break;
	case BS_ON:
		total_on += bid->quantity;
		break;
	}
	total += bid->quantity;

	return n_bids++;
}

KEY curve::resubmit(BID *bid, KEY key)
{
	if (key<n_bids)
	{
		/* undo effect of old state */
		BID *old = &(bids[keys[key]]);
		switch (old->state) {
		case BS_OFF:
			total_off -= old->quantity;
			break;
		case BS_ON:
			total_on -= old->quantity;
			break;
		}
		total -= old->quantity;

		/* replace old bid with new bid */
		bids[keys[key]] = *bid;

		/* impose effect of new state */
		switch (bid->state) {
		case BS_OFF:
			total_off += bid->quantity;
			break;
		case BS_ON:
			total_on += bid->quantity;
			break;
		}
		total += bid->quantity;
		return key;
	}
	else
		return -1;
}

void curve::sort(bool reverse)
{
	sort(bids, keys, n_bids, reverse);
}

void curve::sort(BID *list, KEY *key, const int len, const bool reverse)
{
	//merge sort
	if (len>1)
	{
		int split = len/2;
		KEY *a = key, *b = key+split;
		if (split>1) sort(list,a,split,reverse);
		if (len-split>1) sort(list,b,len-split,reverse);
		KEY *res = new KEY[len];
		KEY *p = res;
		do {
			bool altb = list[*a].price < list[*b].price;
			if ((reverse && !altb) || (!reverse && altb))
				*p++ = *a++;
			else
				*p++ = *b++;
		} while (a<key+split && b<key+len);
		while (a<key+split)
			*p++ = *a++;
		while (b<key+len)
			*p++ = *b++;
		memcpy(key,res,sizeof(KEY)*len);
		delete[] res;
	}
}

double curve::get_total_at(double price){
	double sum = 0.0;
	int i = 0;
	if(n_bids > 0){
		for(i = 0; i < n_bids; ++i){
			if(bids[i].price == price){
				sum += bids[i].quantity;
			}
		}
		return sum;
	}
	return 0.0;
}

double curve::get_min(){
	double min;
	int i = 0;
	if(n_bids > 0){
		min = bids[i].price;
		for(i = 1; i < n_bids; ++i){
			if(bids[i].price < min){
				min = bids[i].price;
			}
		}
		return min;
	} else {
		return 0.0;
	}
}

// End of curve.cpp
