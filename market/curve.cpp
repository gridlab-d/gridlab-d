#include "curve.h"

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF BID CURVE
//////////////////////////////////////////////////////////////////////////

curve::curve(void)
{
	len = 0;
	bids = NULL;
	keys = NULL;
	bid_ids = NULL;
	n_bids = 0;
	total = 0;
}

curve::~curve(void)
{
	delete [] bids;
	delete [] keys;
	delete [] bid_ids;
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
		bid_ids = new KEY[len];
	}
	else if (n_bids==len) // grow the bid list
	{
		BID *newbids = new BID[len*2];
		KEY *newkeys = new KEY[len*2];
		KEY *newbid_ids = new KEY[len*2];
		memcpy(newbids,bids,len*sizeof(BID));
		memcpy(newkeys,keys,len*sizeof(KEY));
		memcpy(newbid_ids,bid_ids,len*sizeof(KEY));
		delete[] bids;
		delete[] keys;
		delete[] bid_ids;
		bids = newbids;
		keys = newkeys;
		bid_ids = newbid_ids;
		len*=2;
	}
	keys[n_bids] = n_bids;
	bid_ids[n_bids] = bid->bid_id;
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

KEY curve::resubmit(BID *bid)
{
	int i = 0;
	int bid_index = 0;
	int bid_hitcount = 0;
	for(i = 0; i < n_bids; i++) {
		if(bid_ids[i] == bid->bid_id) {
			if(bid_hitcount == 1) {
				gl_error("curve::resubmit - There is more than one bid with the same bid id in the bid curve.");
				return -1;
			}
			bid_index = i;
			bid_hitcount++;
		}
	}
	if(bid_hitcount == 0) {
		gl_warning("The bid was flagged as a rebid but there is no bid in the bid curve with the bid id provided. Submitting the bid.");
		if (len==0) // create the bid list
		{
			len = 8;
			bids = new BID[len];
			keys = new KEY[len];
			bid_ids = new KEY[len];
		}
		else if (n_bids==len) // grow the bid list
		{
			BID *newbids = new BID[len*2];
			KEY *newkeys = new KEY[len*2];
			KEY *newbid_ids = new KEY[len*2];
			memcpy(newbids,bids,len*sizeof(BID));
			memcpy(newkeys,keys,len*sizeof(KEY));
			memcpy(newbid_ids,bid_ids,len*sizeof(KEY));
			delete[] bids;
			delete[] keys;
			delete[] bid_ids;
			bids = newbids;
			keys = newkeys;
			bid_ids = newbid_ids;
			len*=2;
		}
		keys[n_bids] = n_bids;
		bid_ids[n_bids] = bid->bid_id;
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
	} else if(bid_index < n_bids) {
		/* undo effect of old state */
		BID *old = &(bids[keys[bid_index]]);
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
		bids[keys[bid_index]] = *bid;

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
		return bid_index;
	} else {
		gl_error("curve::resubmit - the bid failed to be captured in the curve.");
		return -1;
	}
}
//This function is for removing a from a curve if the rebid places the bidder in the opposite curve.(i.e. switching from a seller to a buyer or vice versa)
int curve::remove_bid(KEY bid_id)
{
	int i = 0;
	int bid_index = 0;
	int bid_hitcount = 0;
	for(i = 0; i < n_bids; i++) {
		if(bid_ids[i] == bid_id) {
			if(bid_hitcount == 1) {
				gl_error("curve::resubmit - There is more than one bid with the same bid id in the bid curve.");
				return -1;
			}
			bid_index = i;
			bid_hitcount++;
		}
	}
	if (bid_index < n_bids && bid_hitcount == 1) {
		/* undo effect of old state */
		BID *old = &(bids[keys[bid_index]]);
		switch (old->state) {
		case BS_OFF:
			total_off -= old->quantity;
			break;
		case BS_ON:
			total_on -= old->quantity;
			break;
		}
		total -= old->quantity;
		/* copy all other bids into new curve */
		BID *newbids = new BID[len];
		KEY *newkeys = new KEY[len];
		KEY *newbid_ids = new KEY[len];
		for(i = 0; i <= len; i++){
			if(i < bid_index){
				newkeys[i] = keys[i];
				newbids[i] = bids[i];
				newbid_ids[i] = bid_ids[i];
			} else if(i>bid_index){
				newkeys[i-1] = keys[i];
				newbids[i-1] = bids[i];
				newbid_ids[i-1] = bid_ids[i];
			}
		}
		delete[] bids;
		delete[] keys;
		delete[] bid_ids;
		bids = newbids;
		keys = newkeys;
		bid_ids = newbid_ids;
		n_bids--;
		return n_bids;
	} else {
		return n_bids;
	}
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
