'''Use the main metrics to determine the best course of action.

The main metrics are the percent differences between SCADA and simulated for:
- summer peak value
- summer total energy
- winter peak value
- winter total energy
Evaluating whether each of these values is less than, greater than or equal to 0 points to a particular "action", which in turn will point to which calibration parameters to alter.

This function returns an action ID and a brief description. 

'''
# Returns which of eight actions to take

# Test Metric Values (% differences for summer peak, summer energy, winter peak, winter energy)
Ps = 5;
Es = 5;
Pw =  -9;
Ew =  -9;
mets = [Ps, Es, Pw, Ew]; 

actionDict = { 	 1:[ 'raise load',[1,1,1,1],[2,1,1,1],[2,1,2,1]],
				-1: ['lower load',[3,3,3,3],[2,3,3,3],[2,3,2,3]],
				 2: ['raise winter load',[3,3,1,1],[3,2,1,1],[2,3,1,1],[2,2,1,1],[2,3,2,1],[2,2,2,1]],
				-2: ['lower winter load',[1,1,3,3],[1,2,3,3],[2,1,3,3],[2,2,3,3],[2,1,2,3],[2,2,2,3]],
				 3: ['raise winter peak',[3,3,1,3],[2,3,1,3],[2,2,1,3],[2,2,1,2],[2,3,1,2],[3,3,1,2]],
				-3: ['lower winter peak',[1,1,3,1],[2,1,3,1],[2,2,3,1],[2,2,3,2],[2,1,3,2],[1,1,3,2]],
				 4: ['raise winter & summer',[1,1,1,3],[2,1,1,3],[2,1,1,2],[1,1,1,2]],
				-4: ['lower winter & summer',[3,3,3,1],[2,3,3,1],[2,3,3,2],[3,3,3,2]],
				 5: ['raise summer load',[1,3,3,3],[1,3,2,3],[1,3,2,2],[1,2,2,3],[1,2,2,2],[1,1,2,2],[1,1,2,3],[2,1,2,2]],
				-5: ['lower summer load',[3,1,1,1],[3,1,2,1],[3,1,2,2],[3,2,2,1],[3,2,2,2],[3,3,2,2],[3,3,2,1],[2,3,2,2]],
				 6: ['raise summer & winter',[1,3,1,1],[1,3,2,1],[1,1,2,1],[1,2,2,1]],
				-6: ['lower summer & winter',[3,1,3,3],[3,1,2,3],[3,3,2,3],[3,1,1,3]],
				 7: ['raise peaks',[1,3,1,3],[1,3,1,2],[1,2,1,2],[1,2,1,3],[1,2,1,1]],
				-7: ['lower peaks',[3,1,3,1],[3,1,3,2],[3,2,3,2],[3,2,3,1],[3,2,3,3]],
				 8: ['raise summer & lower winter',[1,3,3,1],[1,2,3,1],[1,2,3,2],[1,3,3,2]],
				-8: ['lower summer & raise winter',[3,1,1,3],[3,1,1,2],[3,2,1,3],[3,2,1,2]] };

def convert(d):
	'''Takes list of 4 numbers and returns coded list based on higher, equal, less than 0.'''
	[ps,es,pw,ew] = d;
	v = [0,0,0,0];
	if ps < 0:
		v[0]=1;
	elif ps == 0:
		v[0]=2;
	elif ps > 0:
		v[0]=3;
	if es < 0:
		v[1]=1;
	elif es == 0:
		v[1]=2;
	elif es > 0:
		v[1]=3;
	if pw < 0:
		v[2]=1;
	elif pw == 0:
		v[2]=2;
	elif pw > 0:
		v[2]=3;
	if ew < 0:
		v[3]=1;
	elif ew == 0:
		v[3]=2;
	elif ew > 0:
		v[3]=3;
	return v;
	
def chooseAction(difs):
	'''Using 4 main metrics, decide which action to take.'''
	checkVal = convert(difs);
	if  0 in checkVal:
		print ("Ooops! Something is the matter.");
		return;
	else:
		for i in actionDict.keys():
			if checkVal in actionDict[i]:
				return i, actionDict[i][0];

def main():
	action, desc = chooseAction(mets);
	print (__doc__)
	print ("From the test metrics, we've chosen to " +desc);

if __name__ ==  '__main__':
	main();