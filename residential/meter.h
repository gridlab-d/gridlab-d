// meter.h

#ifndef _METER_H
#define _METER_H

class meter {
private:

protected:

public:

	complex v12; // voltage on 1-2 (~110V)
	complex v23; // voltage on 2-3 (~110V)
	complex v13; // voltage on 1-3 (~220V)
	complex i1, i2, i3; // currents on lines 1, 2 and 3

	meter(void);
	~meter(void);

	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0);
};

#endif
