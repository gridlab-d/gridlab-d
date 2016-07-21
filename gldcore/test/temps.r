## remove all objects in case a previous workspace is loaded 
rm(list=ls()) 

## read data
indoor.data <- read.table("indoor.csv", sep=",", header=FALSE, row.names=NULL, as.is=TRUE)
outdoor.data <- read.table("outdoor.csv", sep=",", header=FALSE, row.names=NULL, as.is=TRUE)

## convert date strings to date objects:
date.format <- "%Y-%m-%d %H:%M:%S"
indoor.dates <- strptime(indoor.data[,1], date.format)
outdoor.dates <- strptime(outdoor.data[,1], date.format)

plot.date.format <- "%m/%d"

y.limits <- range(indoor.data[,2], outdoor.data[,2], na.rm=TRUE)

## use plot(...) to setup first plot, then lines(...) or points(...) to add more data
png(filename="temps.png", width=640, height=480)

plot(outdoor.dates, outdoor.data[,2], type="l", col="green", format=plot.date.format,
    xlab="Date", ylab="Temperature (F)", main="Temperatures", ylim=y.limits)

lines(indoor.dates, indoor.data[,2], type="l", col="red")

legend("bottomleft", c("Outdoor temperature", "Average indoor temperature"), 
    col=c("green", "red"), lwd=1, cex=0.7, inset=0.02)

dev.off() ##closes png for writing

q("no") ##quit without saving workspace