clear;
clc;

%% Read data
dir = 'C:\Users\d3x289\Documents\LAAR\3.1\VS2005\x64\Release\Base2 02 04 2015\';

data.substation = csvread([dir 'main_regulator.csv'],9,1);
data.waterheater = csvread([dir 'waterheater_load.csv'],9,1);
data.ZIPload = csvread([dir 'ZIPload_load.csv'],9,1);
data.regulator_taps = csvread([dir 'regulator_taps.csv'],9,1);
data.regulator_tap_count = csvread([dir 'regulator_tap_count.csv'],9,1);
data.capacitor_switch_count = csvread([dir 'capacitor_switch_count.csv'],9,1);
data.house = csvread([dir 'example_house.csv'],9,1);

fid = fopen([dir 'capacitor_state.csv']);
junk = textscan(fid,'%s',9,'Delimiter','\n');
temp_data = textscan(fid,'%*s %s %s %s %s %s %s %s %s %s','Delimiter',',');

for jind=1:9
    cap_state(:,jind) = strncmp(temp_data{1,jind},'CLOS',4);
end

data.capacitor_state = cap_state;

fclose('all');
clear temp_data cap_state;

%% Plotting time series
no_hours = 5;
n = 60 * no_hours;
year = 2000 * ones(1,n);
month = 9 * ones(1,n);
day = 1 * ones(1,n);
hour = [12 * ones(1,n/no_hours), 13 * ones(1,n/no_hours), 14 * ones(1,n/no_hours), 15 * ones(1,n/no_hours), 16 * ones(1,n/no_hours)];
for jind=1:no_hours
    minutes((jind-1)*60+1:jind*60) = 0:59;
end

xdate = datenum(year,month,day,hour,minutes,zeros(1,n));

%% Quick plot data
figure(1);
clf(1);
hold on;

my_max = round( 1.1 * max([max(data.substation(:,1)),max(data.substation(:,3)),max(data.substation(:,5))]) * .000001) / .000001;

plot(xdate,data.substation(:,1)/1000,'r');
plot(xdate,data.substation(:,3)/1000,'b');
plot(xdate,data.substation(:,5)/1000,'g');

ylim([0,my_max / 1000])

ylabel('Power by Phase (kW)');
legend('Phase A','Phase B','Phase C');
datetick('x','HH:MM');
hold off;



figure(10);
clf(10);
hold on;

plot(xdate,data.substation(:,1)/1000+data.substation(:,3)/1000+data.substation(:,5)/1000,'k');
plot(xdate,data.waterheater(:,1),'b');
plot(xdate,data.waterheater(:,1)+data.ZIPload(:,1),'r');

ylabel('Power (kW)');
legend('Total Load','Water heater','Water heater + ZIP','Location','Northwest');
datetick('x','HH:MM');
hold off;



figure(20);
clf(20);

subplot(7,1,1);
hold on;

plot(xdate,data.regulator_taps(:,1:3),'k');
plot(xdate,data.regulator_taps(:,4:6),'g');
plot(xdate,data.regulator_taps(:,7:9),'b');
plot(xdate,data.regulator_taps(:,10:12),'r');

legend({'Feeder Reg A','B','C','Reg 2 A','B','C','Reg 3 A','B','C','Reg 4 A','B','C'},'Location','West');
ylabel('Tap Position');
datetick('x','HH:MM');
ylim([-16 16]);
hold off;

subplot(5,1,1:2);
hold on;
plot(xdate,data.regulator_taps(:,1:3),'k');
plot(xdate,data.regulator_taps(:,4:6),'g');
plot(xdate,data.regulator_taps(:,7:9),'b');
plot(xdate,data.regulator_taps(:,10:12),'r');
legend({'Feeder Reg A','B','C','Reg 2 A','B','C','Reg 3 A','B','C','Reg 4 A','B','C'},'Location','West');
ylabel('Tap Position');
ylim([-16 16]);
datetick('x','HH:MM');
hold off;

subplot(5,1,3);
hold on;
plot(xdate,data.capacitor_state(:,1),'k');
plot(xdate,data.capacitor_state(:,2),'k-.');
plot(xdate,data.capacitor_state(:,3),'k:');
legend({'Cap0 A','B','C'},'Location','West');
ylabel('Switch Position');
ylim([-0.1 1.1]);
datetick('x','HH:MM');
hold off;

subplot(5,1,4);
hold on;
plot(xdate,data.capacitor_state(:,4),'g');
plot(xdate,data.capacitor_state(:,5),'g-.');
plot(xdate,data.capacitor_state(:,6),'g:');
legend({'Cap1 A','B','C'},'Location','West');
ylabel('Switch Position');
ylim([-0.1 1.1]);
datetick('x','HH:MM');
hold off;

subplot(5,1,5);
hold on;
plot(xdate,data.capacitor_state(:,7),'b');
plot(xdate,data.capacitor_state(:,8),'b-.');
plot(xdate,data.capacitor_state(:,9),'b:');
legend({'Cap2 A','B','C'},'Location','West');
ylabel('Switch Position');
ylim([-0.1 1.1]);
datetick('x','HH:MM');
hold off;




figure(30);
clf(30);

hold on;
my_max = round( 1.1 * max([max(data.substation(:,1)),max(data.substation(:,3)),max(data.substation(:,5))]) * .000001) / .000001;

plot(xdate,data.substation(:,1)./sqrt(data.substation(:,1).^2 + data.substation(:,2).^2),'r');
plot(xdate,data.substation(:,3)./sqrt(data.substation(:,3).^2 + data.substation(:,4).^2),'b');
plot(xdate,data.substation(:,5)./sqrt(data.substation(:,5).^2 + data.substation(:,6).^2),'g');

ylim([0.9 1])

ylabel('Power Factor');
legend('Phase A','Phase B','Phase C');
datetick('x','HH:MM');
hold off;
%%

save('timeseries_data.mat','data');