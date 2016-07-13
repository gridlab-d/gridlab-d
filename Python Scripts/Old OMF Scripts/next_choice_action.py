# If the first choice action (the one determined in chooseAction.py) fails to produce a .glm with a better WSM score, try the second choice, then third, etc. If nothing produces a better WSM score, we'll have to kick out of the loop. 

next_choice_actions = {}
action_desc ={ 	 1: 'raise load',
				-1: 'lower load',
				 2: 'raise winter load',
				-2: 'lower winter load',
				 3: 'raise winter peak',
				-3: 'lower winter peak',
				 4: 'raise winter & summer',
				-4: 'lower winter & summer',
				 5: 'raise summer load',
				-5: 'lower summer load',
				 6: 'raise summer & winter',
				-6: 'lower summer & winter',
				 7: 'raise peaks',
				-7: 'lower peaks',
				 8: 'raise summer & lower winter',
				-8: 'lower summer & raise winter',
				 0: 'stop calibration',
				-0: 'stop calibration' };

# 1 - 1 : load overall 
# 2 - 7 : peaks overall 
# 3 - 2 : winter load
# 4 - 5 : summer peak
# 5 - 3 : winter peak
# 6 - 4 : winter peak & summer
# 7 - 6 : summer peak & winter
# 8 - 8 : peaks opposite

next_choice_actions[1] = { 1 : 7, 7 : 2, 2 : 5, 5 : 3, 3 : 4, 4 : 6, 6 : 8, 8 : 0 }

# 1 - 2 : winter load only
# 2 - 3 : winter peak only
# 3 - 4 : winter peak & summer
# 4 - 1 : load overall
# 5 - 7 : peaks overall

next_choice_actions[2] = { 2 : 3, 3 : 4, 4 : 1 , 1 : 7, 7 : 0 }

# 1 - 3 : winter peak
# 2 - 2 : winter load
# 3 - 4 : winter peak & summer
# 4 - 7 : peaks overall
# 5 - 1 : load overall 

next_choice_actions[3] = { 3 : 2, 2 : 4, 4 : 7, 7 : 1, 1 : 0 }

# 1 - 4 : winter peak & summer
# 2 - 3 : winter peak
# 3 - 7 : peaks overall
# 4 - 8 : winter load
# 5 - 1 : load overall

next_choice_actions[4] = { 4 : 3, 3 : 7, 7 : 8, 8 : 1, 1 : 0 }

# 1 - 5 : summer peak
# 2 - 6 : summer peak & winter
# 3 - 7 : peaks overall
# 4 - 1 : load overall

next_choice_actions[5] = { 5 : 6, 6 : 7, 7 : 1, 1 : 0 }

# 1 - 6 : summer peak & winter
# 2 - 7 : peaks overall
# 3 - 5 : summer peak
# 4 - 6 : winter peak
# 5 - 1 : load overall

next_choice_actions[6] = { 6 : 7, 7 : 5, 5 : 6, 6 : 1, 1 : 0 }

# 1 - 7 : peaks overall
# 2 - 1 : load overall
# 3 - 4 : winter peak & summer
# 4 - 3 : winter peak
# 5 - 2 : winter load
# 6 - 5 : summer peak

next_choice_actions[7] = { 7 : 1, 1 : 4, 4 : 3, 3 : 2, 2 : 5, 5 : 0 }

# 1 - 8 : peaks opposite
# 2 - 3 : winter peak
# 3 - 2 : winter load
# 4 - 5 : summer peak

next_choice_actions[8] = { 8 : 3, 3 : 2, 2 : 5, 5 : 0 }

def main():
	last_action = -3
	action = 1
	action = (action/abs(action)) * (next_choice_actions[abs(action)][abs(last_action)])
	print (action)
		
if __name__ ==  '__main__':
	main();