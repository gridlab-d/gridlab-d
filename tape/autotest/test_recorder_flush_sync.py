import csv
import time 
import datetime
#import os


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


	# temp_time_stamp = reversed(list(csv.reader(textfile)))
	# print((list(temp_time_stamp))[0])

# print("berk")

# with open('test_recorder_flush.csv', 'r') as textfile:
# 	#temp_time_stamp = reversed(list(csv.reader(textfile)))
# 	#print(list(temp_time_stamp)[0])
# 	for row in reversed(list(csv.reader(textfile))):
# 		current_date_time = ', '.join(row)
# 		#print(current_date_time)
# 		#print(type(current_time))
# 		split_time = current_date_time.split(',')
# 		current_date = split_time[0]
# 		current_time = split_time[1]
# 		#temp_time_stamp = current_date_time
# 		#if current_time == temp_time_stamp
# 		#	exit(0) 
# 		#print(current_date)

		

# 		#print(current_date_time)

# 		break

