'''Calculate the weights of each metric used in Weighted Sum Model.

These calculations are based on pairwise comparisons between each metric.
These pairwise comparisons are defined within this module, and this is where they should be changed.
There are 15 metrics total going into the WSM score for each .glm being tested.
Theses are peak value, total energy, peak time, minimum value, minimum time for each season -- summer, winter, shoulder (spring).

Pairwise Comparisons represent the importance of A with respect to B according to the following scale.
1: Equal 		2: weak/slight 		3: moderate 
4: moderate plus 	5: strong 		6: strong plus
7: very strong 		8: very very strong 	9: extreme 

Example: 
A is 'extremely' more important than B implies A vs. B = 9, and therfore B vs. A = 1/9.

'''
from __future__ import division
 
# Pairwise Comparisons
PvPt = 3;
PvTe = 5;
PvMv = 5;
PvMt = 7;
PtTe = 2;
PtMv = 4;
PtMt = 6;
TeMv = 2;
TeMt = 4;
MvMt = 2;
SW = 4;
Ss = 9;
Ws = 8;

def calcPriorities ():
	'''Calculate weights for each "difference metric" based on pairwise comparisons defined earlier.
	
	Return wieghts for peak value, peak time, total energy, minimum value, minimum time.
	Return wieghts for summer, winter, and shoulder.
	
	'''
	pv_r = 1 		+ PvPt 		+ PvTe 		+ PvMv 		+ PvMt;
	pt_r = 1/PvPt 	+ 1 		+ PtTe 		+ PtMv 		+ PtMt;
	te_r = 1/PvTe 	+ 1/PtTe 	+ 1 		+ TeMv 		+ TeMt;
	mv_r = 1/PvMv 	+ 1/PtMv 	+ 1/TeMv 	+ 1 		+ MvMt;
	mt_r = 1/PvMt 	+ 1/PtMt 	+ 1/TeMt 	+ 1/MvMt 	+ 1;
	total = pv_r + pt_r + te_r + mv_r + mt_r;
	pv = pv_r/total;
	pt = pt_r/total;
	te = te_r/total;
	mv = mv_r/total;
	mt = mt_r/total;	
	S_r = 1 	+ SW 	+ Ss;
	W_r = 1/SW 	+ 1 	+ Ws;
	s_r = 1/Ss 	+ 1/Ws 	+ 1;
	tota1 = S_r + W_r + s_r;
	su = S_r/tota1;
	wi = W_r/tota1;
	sh = s_r/tota1;
	return pv, pt, te, mv, mt, su, wi, sh;


def calcGlobalPriorities (p):
	'''Further weight the calculated priorities of each "difference metric" by the season priorities. '''
	summer = [];
	winter = [];
	shoulder = [];
	for i in range (len(p[0:5])):
		summer.append(p[0:5][i] * p[5]);
		winter.append(p[0:5][i] * p[6]);
		shoulder.append(p[0:5][i] * p[7]);
	return summer, winter, shoulder;
	
def main():
	weights = calcGlobalPriorities(calcPriorities());
	
	return weights;
	
if __name__ ==  '__main__':
	w = main();
	print (__doc__)
	print ("Weights for each of the fifteen metrics: ");
	j = [];
	l = [];
	m = [];
	for i in w[0]:
		j.append(round(i,4));
	for i in w[1]:
		l.append(round(i,4));
	for i in w[2]:
		m.append(round(i,4));
	print ("summer \t\t" + str(j));
	print ("winter \t\t" + str(l));
	print ("shoulder \t" + str(m));