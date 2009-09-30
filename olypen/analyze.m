%% analyze olypen results

%% load data
control.energy = csvread('control_energy.csv',9,1);
%control.transient = csvread('control_power.csv',9,1);
control.power = [0; diff(control.energy)];

fixed.energy = csvread('fixed_energy.csv',9,1);
%fixed.transient = csvread('fixed_power.csv',9,1);
fixed.power = [0; diff(fixed.energy)];
fixed.savings = (fixed.energy-control.energy)./control.energy*100;

tou.energy = csvread('tou_energy.csv',9,1);
%tou.transient = csvread('tou_power.csv',9,1);
tou.power = [0; diff(tou.energy)];
tou.savings = (tou.energy-control.energy)./control.energy*100;

rtp.energy = csvread('rtp_energy.csv',9,1);
%rtp.transient = csvread('rtp_power.csv',9,1);
rtp.power = [0; diff(rtp.energy)];
rtp.savings = (rtp.energy-control.energy)./control.energy*100;

plot(1:8759,-fixed.savings,1:8759,-tou.savings,1:8759,-rtp.savings);
title('Energy savings');
legend(sprintf('Fixed (%.1f%%)',fixed.savings(end)),...
	sprintf('Time-of-Use (%.1f%%)',tou.savings(end)),...
	sprintf('Real-time (%.1f%%)',rtp.savings(end)));
xlabel('Hour of year');
ylabel('Cumulative % savings');
ylim([-5,20]);