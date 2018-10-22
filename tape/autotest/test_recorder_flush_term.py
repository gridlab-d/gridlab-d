import csv
import time 
import datetime

with open('test_recorder_flush.csv', 'r') as textfile:
	
	temp_row = 0
	first_row_flag = True
	delta = 0

	for row in reversed(list(csv.reader(textfile))):
		try:
			time_col = datetime.datetime.strptime(row[0][:-3].rstrip(), "%Y-%m-%d %H:%M:%S")
		except:
			continue

		if first_row_flag:
			temp_row = time_col
			first_row_flag = False
			continue

		delta = temp_row-time_col		
		

		if delta == datetime.timedelta(minutes=1):
			#print("success", delta)
			exit(0)
		else: 
			#print("fail", delta)
			exit(1)
		break


