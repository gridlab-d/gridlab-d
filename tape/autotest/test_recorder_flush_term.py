import csv

with open('test_recorder_flush.csv', 'r') as textfile:
	for row in reversed(list(csv.reader(textfile))):
		print(', '.join(row))
		break
