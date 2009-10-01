%% analyze olypen model results
% DP Chassin
% Sept 2009
clear all; clc;

%% load & process data
control.energy = csvread('control_energy.csv',9,1);
%control.transient = csvread('control_power.csv',9,1);
control.power = [0; diff(control.energy)];
control.weekly_peak = max(reshape(control.power(1:8736),168,[]))/1000;
control.peak = max(control.weekly_peak);

fixed.energy = csvread('fixed_energy.csv',9,1);
%fixed.transient = csvread('fixed_power.csv',9,1);
fixed.power = [0; diff(fixed.energy)];
fixed.savings = (fixed.energy-control.energy)./control.energy*100;
fixed.weekly_peak = max(reshape(fixed.power(1:8736),168,[]))/1000;
fixed.peak = max(fixed.weekly_peak);

tou.energy = csvread('tou_energy.csv',9,1);
%tou.transient = csvread('tou_power.csv',9,1);
tou.power = [0; diff(tou.energy)];
tou.savings = (tou.energy-control.energy)./control.energy*100;
tou.weekly_peak = max(reshape(tou.power(1:8736),168,[]))/1000;
tou.peak = max(tou.weekly_peak);

rtp.energy = csvread('rtp_energy.csv',9,1);
%rtp.transient = csvread('rtp_power.csv',9,1);
rtp.market = csvread('rtp_price.csv',9,1);
rtp.power = [0; diff(rtp.energy)];
rtp.savings = (rtp.energy-control.energy)./control.energy*100;
rtp.weekly_peak = max(reshape(rtp.power(1:8736),168,[]))/1000;
rtp.peak = max(rtp.weekly_peak);

%% energy savings plot
figure(1);
plot((1:8759)/168,-fixed.savings,(1:8759)/168,-tou.savings,(1:8759)/168,-rtp.savings);
title('Energy savings');
legend(sprintf('Fixed (%.1f%%)',fixed.savings(end)),...
	sprintf('Time-of-Use (%.1f%%)',tou.savings(end)),...
	sprintf('Real-time (%.1f%%)',rtp.savings(end)));
xlabel('Week');
ylabel('Cumulative % savings');
ylim([-10,20]);
xlim([0 52]);

disp(sprintf('RTP Revenue = $%.3fM', sum(rtp.market(:,1) .* rtp.market(:,2)/1e9)));

%% power demand plot
figure(2);
plot(1:52,control.weekly_peak,1:52,fixed.weekly_peak,1:52,tou.weekly_peak,1:52,rtp.weekly_peak);
line([0 1],[fixed.peak fixed.peak]); text(1,fixed.peak,sprintf(' Fixed %.1f kW (%.1f%%)', fixed.peak,(1-fixed.peak/control.peak)*100));
line([0 1],[tou.peak tou.peak]); text(1,tou.peak,sprintf(' TOU %.1f kW (%.1f%%)', tou.peak, (1-tou.peak/control.peak)*100));
line([0 1],[rtp.peak rtp.peak]); text(1,rtp.peak,sprintf(' RTP %.1f kW (%.1f%%)', rtp.peak, (1-rtp.peak/control.peak)*100));
title('Weekly peak demand (kW)');
ylabel('Peak demand (kW)');
xlabel('Week');
legend('Control','Fixed','Time-of-Use','Real-time');
xlim([0 52]);
ylim([0 110]);

%% price plot
figure(3);

subplot(2,1,1)
semilogy((1:length(rtp.market(:,2)))/168/12,rtp.market(:,2));
xlabel('Week');
ylabel('RTP Price ($/MWh)');
xlim([0 52]);
ylim([0 10000]);

subplot(2,1,2);
semilogy((1:8759)/168,rtp.power/1000);
xlabel('Week');
ylabel('RTP Load (kW)');
xlim([0 52]);
ylim([0 100]);
