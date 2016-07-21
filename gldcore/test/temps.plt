set terminal png size 640,480 
set title "Temperatures"
set output 'temps.png'
set datafile separator ","
set xdata time
set key below
set timefmt "%Y-%m-%d %H:%M:%S"
set format x "%m/%d"
set xlabel "Date"
set ylabel "Temperature (F)"
plot 'indoor.csv' using 1:2 with lines title 'Average indoor temperature', 'outdoor.csv' using 1:2 with lines title 'Outdoor temperature'
