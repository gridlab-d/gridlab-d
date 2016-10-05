clear;
clc;
%close all;

%% Read data
%----------------------------------------------------------- 4 second results -------------------------------------------------------------------
%------------------------------------------------------------------------------------------------------------------------------------------------
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_14_voltagelock_0_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_14_voltagelock_1_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_14_voltagelock_0_voltagesort_1\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_14_voltagelock_1_voltagesort_1\';

%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_10_voltagelock_0_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_10_voltagelock_1_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_10_voltagelock_0_voltagesort_1\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_10_voltagelock_1_voltagesort_1\';

%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_valley_droop_2_5_voltagelock_0_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_valley_droop_2_5_voltagelock_1_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_valley_droop_2_5_voltagelock_0_voltagesort_1\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_valley_droop_2_5_voltagelock_1_voltagesort_1\';

%----------------------------------------------------------- 1 second results -------------------------------------------------------------------
%------------------------------------------------------------------------------------------------------------------------------------------------
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_paloverde_droop_10_voltagelock_0_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_paloverde_droop_10_voltagelock_1_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_paloverde_droop_10_voltagelock_0_voltagesort_1\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_paloverde_droop_10_voltagelock_0_voltagesort_1B\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_paloverde_droop_10_voltagelock_1_voltagesort_1\';

gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_valley_droop_2_5_voltagelock_0_voltagesort_0\';
gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_valley_droop_2_5_voltagelock_1_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_valley_droop_2_5_voltagelock_0_voltagesort_1\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_valley_droop_2_5_voltagelock_0_voltagesort_1B\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_valley_droop_2_5_voltagelock_1_voltagesort_1\';

load([gld_path,'timeseries_data.mat'])

%% water heater state plot
wh_state_plot = 0;

%% Feeder load plot along with tap changes of regulators
loadAndTapPlot = 0;
phase = 'A';
compare = 0; %0 = DR0, 1 = DR0 and DR2, 2 = DR0 and DR3

%% water heater voltage lockout plot
wh_voltage_lock_plot = 0;

%% Frequency plot
frequency_plot = 0;

%% FRL control state plot    
FRL_state_plot = 0;

%% Plot the PF curve
PF_curve_at_clear = 0;
PF_curve_between_clear = 0;
start = (0*60)/15; 
stop = (2*60)/15;
market = 50;
delta_time = 4; % in seconds
supervisor_time = 900;
supervisor_droop = 0.035;

%% Plot of power response in MW
MWHz_response_plot = 0;
    
%% Violations
violations_data = 0;
delta_time = 1; % (secs) time step of data
% Violation 2 - ANSI bands, instantaneous
high_voltage2 = 240*1.1;
low_voltage2 = 240*0.9;
% Violation 3 - ANSI bands, 5-min
high_voltage3 = 240*1.05;
low_voltage3 = 240*0.95;
% Violation 6 - flicker
% 4 secs == 0.013; 20 secs == 0.016; 60 secs == 0.02; 300 secs == 0.027
time_between = 60; % (secs) time interval to look for flicker
flicker_mag = 0.02; % (perc) threshold of voltage change between time steps

%% Overloads
overload_data = 0;

%% Regulator and Capacitor taps
regulatorCapacitorTaps = 1;

%% Voltage Map Plot
voltageMapPlot = 0;
voltage_point = 1860;%465; % time of frequency drop 450

%% The plots

if wh_state_plot == 1
    % water heater state plot
    figure();
    total_num_wh = size(data.DR0.wh_state,2);
    plot(data.DR0.wh_state_timestamps,sum(data.DR0.wh_state,2),'b')
    hold on
    plot(data.DR2.wh_state_timestamps,sum(data.DR2.wh_state,2),'r')
    plot(data.DR3.wh_state_timestamps,sum(data.DR3.wh_state,2),'g')
    plot([data.DR0.wh_state_timestamps(1) data.DR0.wh_state_timestamps(end)],[total_num_wh/100*5 total_num_wh/100*5],'y--');
    plot([data.DR0.wh_state_timestamps(1) data.DR0.wh_state_timestamps(end)],[total_num_wh/100*10 total_num_wh/100*10],'c--');
    plot([data.DR0.wh_state_timestamps(1) data.DR0.wh_state_timestamps(end)],[total_num_wh/100*15 total_num_wh/100*15],'m--');
    plot([data.DR0.wh_state_timestamps(1) data.DR0.wh_state_timestamps(end)],[total_num_wh/100*20 total_num_wh/100*20],'k--');
    legend('base','autonomous','supervisor','5%','10%','15%','20%')
    xlabel('time')
    ylabel('water heaters on (#)')
    title('water heaters on')
    dynamicDateTicks();
    grid
end

if loadAndTapPlot == 1
    % load and regulator tap plot
    figure();
    subplot(2,1,1)
    plot(data.DR0.main_regulator_timestamps,sum(data.DR0.main_regulator(:,[1:2:end]),2)/1e6,'b');
    hold on
    plot(data.DR2.main_regulator_timestamps,sum(data.DR2.main_regulator(:,[1:2:end]),2)/1e6,'r');
    plot(data.DR3.main_regulator_timestamps,sum(data.DR3.main_regulator(:,[1:2:end]),2)/1e6,'g');
    legend('base','autonomous','supervisor')
    xlabel('time')
    ylabel('power (MW)')
    title('Load')
    dynamicDateTicks();
    grid
    
    if phase == 'A'
        startIndex = 1;
    elseif phase == 'B'
        startIndex = 2;
    elseif phase == 'C'
        startIndex = 3;
    else
        error('check phase flag');
    end 
    subplot(2,1,2)
    plot(data.DR0.main_regulator_timestamps,data.DR0.regulator_taps(:,startIndex),'b');
    hold on
    plot(data.DR0.main_regulator_timestamps,data.DR0.regulator_taps(:,startIndex+3),'g');
    plot(data.DR0.main_regulator_timestamps,data.DR0.regulator_taps(:,startIndex+6),'r');
    plot(data.DR0.main_regulator_timestamps,data.DR0.regulator_taps(:,startIndex+9),'c');
    if compare == 0
        legend('DR0 reg1','DR0 reg2','DR0 reg3','DR0 reg4')
    elseif compare == 1    
        plot(data.DR2.main_regulator_timestamps,data.DR2.regulator_taps(:,startIndex),'b--');
        plot(data.DR2.main_regulator_timestamps,data.DR2.regulator_taps(:,startIndex+3),'g--');
        plot(data.DR2.main_regulator_timestamps,data.DR2.regulator_taps(:,startIndex+6),'r--');
        plot(data.DR2.main_regulator_timestamps,data.DR2.regulator_taps(:,startIndex+9),'c--');
        legend('DR0 reg1','DR0 reg2','DR0 reg3','DR0 reg4','DR2 reg1','DR2 reg2','DR2 reg3','DR2 reg4')
    elseif compare == 2
        plot(data.DR3.main_regulator_timestamps,data.DR3.regulator_taps(:,startIndex),'b--');
        plot(data.DR3.main_regulator_timestamps,data.DR3.regulator_taps(:,startIndex+3),'g--');
        plot(data.DR3.main_regulator_timestamps,data.DR3.regulator_taps(:,startIndex+6),'r--');
        plot(data.DR3.main_regulator_timestamps,data.DR3.regulator_taps(:,startIndex+9),'c--');
        legend('DR0 reg1','DR0 reg2','DR0 reg3','DR0 reg4','DR3 reg1','DR3 reg2','DR3 reg3','DR3 reg4')        
    else 
        error('check compare flag');
    end
    title(['Regulator taps for phase ',phase])
    xlabel('time')
    ylabel('tap (#)')
    dynamicDateTicks();
    grid
end

if wh_voltage_lock_plot == 1
    % water heater voltage lockout plot
    figure();
    total_num_wh = size(data.DR2.wh_state,2);
    plot(data.DR2.wh_state_timestamps,sum(data.DR2.wh_state.*data.DR2.whc_voltage_lockout,2),'r')
    hold on
    plot(data.DR3.wh_state_timestamps,sum(data.DR3.wh_state.*data.DR3.whc_voltage_lockout,2),'g')
    plot([data.DR2.wh_state_timestamps(1) data.DR2.wh_state_timestamps(end)],[total_num_wh/100*5 total_num_wh/100*5],'y--');
    plot([data.DR2.wh_state_timestamps(1) data.DR2.wh_state_timestamps(end)],[total_num_wh/100*10 total_num_wh/100*10],'c--');
    plot([data.DR2.wh_state_timestamps(1) data.DR2.wh_state_timestamps(end)],[total_num_wh/100*15 total_num_wh/100*15],'m--');
    plot([data.DR2.wh_state_timestamps(1) data.DR2.wh_state_timestamps(end)],[total_num_wh/100*20 total_num_wh/100*20],'k--');    
    legend('autonomous','supervisor','5%','10%','15%','20%')
    xlabel('time')
    ylabel('water heaters in lockout (#)')
    title('water heaters on in voltage lockout')
    dynamicDateTicks();
    grid
end
    
if frequency_plot == 1
    % Frequency plot
    figure();
    plot(data.DR2.whc_frequency_timestamps,data.DR2.whc_frequency(:,1),'b')
    hold on
    plot(data.DR3.whc_frequency_timestamps,data.DR3.whc_frequency(:,1),'r')
    legend('autonomous','supervisor')
    xlabel('time')
    ylabel('frequency (Hz)')
    title('frequency')
    grid
    dynamicDateTicks();
end
    
if FRL_state_plot == 1
    %Plot primary frequency control mode plot
    figure();
    subplot(2,1,1)
    clear temp
    temp(1,:) = sum(data.DR2.FREE,2);
    temp(2,:) = sum(data.DR2.TRIGGERED_OFF,2);
    temp(3,:) = sum(data.DR2.TRIGGERED_ON,2);
    temp(4,:) = sum(data.DR2.FORCED_OFF,2);
    temp(5,:) = sum(data.DR2.FORCED_ON,2);
    temp(6,:) = sum(data.DR2.RELEASED_OFF,2);
    temp(7,:) = sum(data.DR2.RELEASED_ON,2);
    area(data.DR2.whc_pfc_state_timestamps,temp')
    legend('FREE','TRIGGERED OFF','TRIGGERED ON','FORCED OFF','FORCED ON','RELEASED OFF','RELEASED ON')
    xlabel('time')
    ylabel('state (#)')
    grid
    title('FRL state response for autonomous operation')
    dynamicDateTicks();
    
    subplot(2,1,2)
    clear temp
    temp(1,:) = sum(data.DR3.FREE,2);
    temp(2,:) = sum(data.DR3.TRIGGERED_OFF,2);
    temp(3,:) = sum(data.DR3.TRIGGERED_ON,2);
    temp(4,:) = sum(data.DR3.FORCED_OFF,2);
    temp(5,:) = sum(data.DR3.FORCED_ON,2);
    temp(6,:) = sum(data.DR3.RELEASED_OFF,2);
    temp(7,:) = sum(data.DR3.RELEASED_ON,2);
    area(data.DR3.whc_pfc_state_timestamps,temp')
    legend('FREE','TRIGGERED OFF','TRIGGERED ON','FORCED OFF','FORCED ON','RELEASED OFF','RELEASED ON')
    xlabel('time')
    ylabel('state (#)')
    grid
    title('FRL state response for supervisory operation')
    dynamicDateTicks();
end


if PF_curve_at_clear == 1  
    % PF curves at clearing for autonomous approach
    if stop ~= 0
        ending = stop;
    else
        ending = floor(size(powerOverSorted,1)/(supervisor_time/delta_time))-1;
    end
    
    state = data.DR2.wh_state;
    power_over = data.DR2.wh_rated_load;
    power_under = data.DR2.wh_actual_load;
    freq_under = data.DR2.whc_trig_freq_under;
    freq_over = data.DR2.whc_trig_freq_over;
    power_over = power_over.*(~state);

    [freqUnderSorted, idxU] = sort(freq_under,2,'descend');
    [freqOverSorted, idxO] = sort(freq_over,2,'ascend');
    m = size(freqUnderSorted,1);
    powerUnderSorted = power_under(bsxfun(@plus,(idxU-1)*m,(1:m)'));  
    powerOverSorted = power_over(bsxfun(@plus,(idxO-1)*m,(1:m)'));
    
    figure();
    subplot(2,1,1)
    hold on
    legend_str = 'legend(''droop curve'',';    
    for i = start : ending
        temp = find(powerUnderSorted(i*((supervisor_time/delta_time))+1,:)~=0);
        powTemp = powerUnderSorted(i*((supervisor_time/delta_time))+1,temp);
        freqTemp = freqUnderSorted(i*((supervisor_time/delta_time))+1,temp);
        if sum(temp) == 0
            continue;
        end

        if i == start
            total_power = max(sum(powerUnderSorted,2))+500;
            plot([60 60-((total_power)/1000)*supervisor_droop],[0 total_power]);
        end

        cumPowTemp = cumsum(powTemp);
        
        %figure();
        plot(freqTemp,cumPowTemp)
        legend_str = [legend_str,'''Market time ',num2str((i)*(supervisor_time/60)),''','];
        %legend(['Market time ',num2str((i-1)*5)]) 
    end
    eval([legend_str(1:end-1),')']);
    xlabel('frequency (Hz)')
    ylabel('power (kW)')
    grid
    title('Autonomous under frequency PF at clearing')
    
    subplot(2,1,2)
    hold on
    legend_str = 'legend(''droop curve'',';
    for i = start : ending
        temp = find(powerOverSorted(i*((supervisor_time/delta_time))+1,:)~=0);
        powTemp = powerOverSorted(i*((supervisor_time/delta_time))+1,temp);
        freqTemp = freqOverSorted(i*((supervisor_time/delta_time))+1,temp);
        if sum(temp) == 0
            continue;
        end

        if i == start
            total_power = max(sum(powerOverSorted,2))+500;
            plot([60 60+((total_power)/1000)*supervisor_droop],[0 total_power]);
        end

        cumPowTemp = cumsum(powTemp);

        %figure();
        plot(freqTemp,cumPowTemp)
        legend_str = [legend_str,'''Market time ',num2str((i)*(supervisor_time/60)),''','];
        %legend(['Market time ',num2str((i-1)*5)]) 
    end
    eval([legend_str(1:end-1),')']);
    xlabel('frequency (Hz)')
    ylabel('power (kW)')
    grid
    title('Autonomous over frequency PF at clearing')  
    
    % PF curves at clearing for supervisory approach
    state = data.DR3.wh_state;
    power_over = data.DR3.wh_rated_load;
    power_under = data.DR3.wh_actual_load;
    freq_under = data.DR3.whc_trig_freq_under;
    freq_over = data.DR3.whc_trig_freq_over;
    power_over = power_over.*(~state);

    [freqUnderSorted, idxU] = sort(freq_under,2,'descend');
    [freqOverSorted, idxO] = sort(freq_over,2,'ascend');
    m = size(freqUnderSorted,1);
    powerUnderSorted = power_under(bsxfun(@plus,(idxU-1)*m,(1:m)'));  
    powerOverSorted = power_over(bsxfun(@plus,(idxO-1)*m,(1:m)'));      

    figure();
    subplot(2,1,1)
    hold on
    legend_str = 'legend(''droop curve'',';
    for i = start : ending
        temp = find(powerUnderSorted(i*((supervisor_time/delta_time))+1,:)~=0);
        powTemp = powerUnderSorted(i*((supervisor_time/delta_time))+1,temp);
        freqTemp = freqUnderSorted(i*((supervisor_time/delta_time))+1,temp);
        if sum(temp) == 0
            continue;
        end

        if i == start
            total_power = max(sum(powerUnderSorted,2))+500;
            plot([60 60-((total_power)/1000)*supervisor_droop],[0 total_power]);
        end

        cumPowTemp = cumsum(powTemp);

        %figure();
        plot(freqTemp,cumPowTemp)
        legend_str = [legend_str,'''Market time ',num2str((i)*(supervisor_time/60)),''','];
        %legend(['Market time ',num2str((i-1)*5)]) 
    end
    eval([legend_str(1:end-1),')']);
    xlabel('frequency (Hz)')
    ylabel('power (kW)')
    grid
    title('Supervisory under frequency PF at clearing')
    
    subplot(2,1,2)
    hold on
    legend_str = 'legend(''droop curve'',';
    for i = start : ending
        temp = find(powerOverSorted(i*((supervisor_time/delta_time))+1,:)~=0);
        powTemp = powerOverSorted(i*((supervisor_time/delta_time))+1,temp);
        freqTemp = freqOverSorted(i*((supervisor_time/delta_time))+1,temp);
        if sum(temp) == 0
            continue;
        end

        if i == start
            total_power = max(sum(powerOverSorted,2))+500;
            plot([60 60+((total_power)/1000)*supervisor_droop],[0 total_power]);
        end

        cumPowTemp = cumsum(powTemp);

        %figure();
        plot(freqTemp,cumPowTemp)
        legend_str = [legend_str,'''Market time ',num2str((i)*(supervisor_time/60)),''','];
        %legend(['Market time ',num2str((i-1)*5)]) 
    end
    eval([legend_str(1:end-1),')']);
    xlabel('frequency (Hz)')
    ylabel('power (kW)')
    grid
    title('Supervisory over frequency PF at clearing')  
end


if PF_curve_between_clear == 1
    % PF curves between clearing for autonomous approach
    state = data.DR2.wh_state;
    power_over = data.DR2.wh_rated_load;
    power_under = data.DR2.wh_actual_load;
    freq_under = data.DR2.whc_trig_freq_under;
    freq_over = data.DR2.whc_trig_freq_over;
    power_over = power_over.*(~state);

    [freqUnderSorted, idxU] = sort(freq_under,2,'descend');
    [freqOverSorted, idxO] = sort(freq_over,2,'ascend');
    m = size(freqUnderSorted,1);
    powerUnderSorted = power_under(bsxfun(@plus,(idxU-1)*m,(1:m)'));  
    powerOverSorted = power_over(bsxfun(@plus,(idxO-1)*m,(1:m)'));     
    
    figure();
    subplot(2,1,1)
    hold on
    t = 0;
    legend_str = 'legend(''droop curve'',';
    for i = market*(supervisor_time/delta_time)+1 : 60/delta_time : (market+1)*(supervisor_time/delta_time)+1
        temp = find(powerUnderSorted(i,:)~=0);
        powTemp = powerUnderSorted(i,temp);
        freqTemp = freqUnderSorted(i,temp);
        if sum(temp) == 0
            continue;
        end
    
        if i == market*(supervisor_time/delta_time)+1
            total_power = max(sum(powerUnderSorted,2))+500;
            plot([60 60-((total_power)/1000)*supervisor_droop],[0 total_power]);
        end
    
        cumPowTemp = cumsum(powTemp);
        
        %figure();
        plot(freqTemp,cumPowTemp)
        legend_str = [legend_str,'''Market time ',num2str((t)+(market*supervisor_time/60)),''','];
        t=t+1;
        %legend(['Market time ',num2str((i-1)*5)]) 
    end
    eval([legend_str(1:end-1),')']);
    xlabel('frequency (Hz)')
    ylabel('power (kW)')
    grid
    title('Autonomous under frequency PF between clearing')
    
    subplot(2,1,2)
    hold on
    t = 0;
    legend_str = 'legend(''droop curve'',';
    for i = market*(supervisor_time/delta_time)+1 : 60/delta_time : (market+1)*(supervisor_time/delta_time)
        temp = find(powerOverSorted(i,:)~=0);
        powTemp = powerOverSorted(i,temp);
        freqTemp = freqOverSorted(i,temp);
        if sum(temp) == 0
            continue;
        end
    
        if i == market*(supervisor_time/delta_time)+1
            total_power = max(sum(powerOverSorted,2))+500;
            plot([60 60+((total_power)/1000)*supervisor_droop],[0 total_power]);
        end
    
        cumPowTemp = cumsum(powTemp);
    
        %figure();
        plot(freqTemp,cumPowTemp)
        legend_str = [legend_str,'''Market time ',num2str((t)+(market*supervisor_time/60)),''','];
        %legend(['Market time ',num2str((i-1)*5)])
        t=t+1;
    end
    eval([legend_str(1:end-1),')']);
    xlabel('frequency (Hz)')
    ylabel('power (kW)')
    grid
    title('Autonomous over frequency PF between clearing')
    
    % PF curves between clearing for supervisry approach
    state = data.DR3.wh_state;
    power_over = data.DR3.wh_rated_load;
    power_under = data.DR3.wh_actual_load;
    freq_under = data.DR3.whc_trig_freq_under;
    freq_over = data.DR3.whc_trig_freq_over;
    power_over = power_over.*(~state);
    
    [freqUnderSorted, idxU] = sort(freq_under,2,'descend');
    [freqOverSorted, idxO] = sort(freq_over,2,'ascend');
    m = size(freqUnderSorted,1);
    powerUnderSorted = power_under(bsxfun(@plus,(idxU-1)*m,(1:m)'));  
    powerOverSorted = power_over(bsxfun(@plus,(idxO-1)*m,(1:m)'));     
    
    figure();
    subplot(2,1,1)
    hold on
    t = 0;
    legend_str = 'legend(''droop curve'',';
    for i = market*(supervisor_time/delta_time)+1 : 60/delta_time : (market+1)*(supervisor_time/delta_time)
        temp = find(powerUnderSorted(i,:)~=0);
        powTemp = powerUnderSorted(i,temp);
        freqTemp = freqUnderSorted(i,temp);
        if sum(temp) == 0
            continue;
        end
    
        if i == market*(supervisor_time/delta_time)+1
            total_power = max(sum(powerUnderSorted,2))+500;
            plot([60 60-((total_power)/1000)*supervisor_droop],[0 total_power]);
        end
    
        cumPowTemp = cumsum(powTemp);
    
        %figure();
        plot(freqTemp,cumPowTemp)
        legend_str = [legend_str,'''Market time ',num2str((t)+(market*supervisor_time/60)),''','];
        t=t+1;
        %legend(['Market time ',num2str((i-1)*5)]) 
    end
    eval([legend_str(1:end-1),')']);
    xlabel('frequency (Hz)')
    ylabel('power (kW)')
    grid
    title('Supervisory under frequency PF between clearing')
    
    subplot(2,1,2)
    hold on
    t = 0;
    legend_str = 'legend(''droop curve'',';
    for i = market*(supervisor_time/delta_time)+1 : 60/delta_time : (market+1)*(supervisor_time/delta_time)
        temp = find(powerOverSorted(i,:)~=0);
        powTemp = powerOverSorted(i,temp);
        freqTemp = freqOverSorted(i,temp);
        if sum(temp) == 0
            continue;
        end
    
        if i == market*(supervisor_time/delta_time)+1
            total_power = max(sum(powerOverSorted,2))+500;
            plot([60 60+((total_power)/1000)*supervisor_droop],[0 total_power]);
        end
    
        cumPowTemp = cumsum(powTemp);
    
        %figure();
        plot(freqTemp,cumPowTemp)
        legend_str = [legend_str,'''Market time ',num2str((t)+(market*supervisor_time/60)),''','];
        %legend(['Market time ',num2str((i-1)*5)])
        t=t+1;
    end
    eval([legend_str(1:end-1),')']);
    xlabel('frequency (Hz)')
    ylabel('power (kW)')
    grid
    title('Supervisory over frequency PF between clearing')
end 
     

if MWHz_response_plot == 1
    % Plot of power response in MW/Hz    
    power_under_auto = -sum((data.DR0.wh_state .* (data.DR2.FORCED_OFF | data.DR2.RELEASED_OFF)).*data.DR0.wh_actual_load,2);
    power_under_auto_volt = -sum((data.DR0.wh_state .* ((data.DR2.FORCED_OFF | data.DR2.RELEASED_OFF) & ~data.DR2.whc_voltage_lockout)).*data.DR0.wh_actual_load,2);
    
    power_under_super = -sum((data.DR0.wh_state .* (data.DR3.FORCED_OFF | data.DR3.RELEASED_OFF)).*data.DR0.wh_actual_load,2);
    power_under_super_volt = -sum((data.DR0.wh_state .* ((data.DR3.FORCED_OFF | data.DR3.RELEASED_OFF) & ~data.DR3.whc_voltage_lockout)).*data.DR0.wh_actual_load,2);
    
    power_over_auto = sum((~data.DR0.wh_state .* (data.DR2.FORCED_ON | data.DR2.RELEASED_ON)).*data.DR0.wh_rated_load,2);
    power_over_auto_volt = sum((~data.DR0.wh_state .* ((data.DR2.FORCED_ON | data.DR2.RELEASED_ON) & ~data.DR2.whc_voltage_lockout)).*data.DR0.wh_rated_load,2);
    
    power_over_super = sum((~data.DR0.wh_state .* (data.DR3.FORCED_ON | data.DR3.RELEASED_ON)).*data.DR0.wh_rated_load,2);
    power_over_super_volt = sum((~data.DR0.wh_state .* ((data.DR3.FORCED_ON | data.DR3.RELEASED_ON) & ~data.DR3.whc_voltage_lockout)).*data.DR0.wh_rated_load,2);
    
    figure();
    subplot(2,1,1)
    plot(data.DR0.wh_state_timestamps,power_under_auto)
    hold on
    plot(data.DR0.wh_state_timestamps,power_under_auto_volt)
    plot(data.DR0.wh_state_timestamps,power_under_super)
    plot(data.DR0.wh_state_timestamps,power_under_super_volt)
    legend('autonomous w/o voltage lockout','autonomous w/ voltage lockout','supervisory w/o voltage lockout','supervisory w/ voltage lockout')
    xlabel('time')
    ylabel('power (kW)')
    title('Combined power response for under frequency')   
    dynamicDateTicks();
    grid
    
    subplot(2,1,2)
    plot(data.DR0.wh_state_timestamps,power_over_auto)
    hold on
    plot(data.DR0.wh_state_timestamps,power_over_auto_volt)
    plot(data.DR0.wh_state_timestamps,power_over_super)
    plot(data.DR0.wh_state_timestamps,power_over_super_volt)
    legend('autonomous w/o voltage lockout','autonomous w/ voltage lockout','supervisory w/o voltage lockout','supervisory w/ voltage lockout')
    xlabel('time')
    ylabel('power (kW)')
    title('Combined power response for over frequency')   
    dynamicDateTicks();
    grid
end


if violations_data == 1
    display('processing voltage date, this will take awhile!')
    % Violation 2
    p=1;
    for i = [0 2 3]
        temp = eval(['data.DR',num2str(i),'.meter_voltage_12']);

        [r,c] = size(temp);
        high_v_count2(p) = 0;
        low_v_count2(p) = 0;

        high_v_count2(p) = sum(sum(logical(temp > high_voltage2),1));
        low_v_count2(p) = sum(sum(logical(temp < low_voltage2),1));

%         for jind=1:c
%             high_v2 = find(temp(:,jind) > high_voltage2);
%             low_v2 = find(temp(:,jind) < low_voltage2);
% 
%             if (sum(high_v2) > 0)
%                 high_v_count2(p) = high_v_count2(p) + 1;
%             end
% 
%             if (sum(low_v2) > 0)
%                 low_v_count2(p) = low_v_count2(p) + 1;
%             end
%         end

        m1(p) = max(max(temp));
        m2(p) = min(min(temp));

    
        % Violation 3
        high_v_count3(p) = 0;
        low_v_count3(p) = 0;
        
        %edges = [zeros(1,size(temp,2)) ; diff(logical(temp > high_voltage3)) ; zeros(1,size(temp,2))];
        edges = diff([zeros(1,size(temp,2)) ; logical(temp > high_voltage3) ; zeros(1,size(temp,2))]);
        rising = find(edges==1);     %rising/falling edges
        falling = find(edges==-1);  
        spanWidth = falling - rising;  %width of span of 1's (above threshold)
        high_v_count3(p) = sum(spanWidth >= 300/delta_time);
        
        %edges = [zeros(1,size(temp,2)) ; diff(logical(temp < low_voltage3)) ; zeros(1,size(temp,2))];
        edges = diff([zeros(1,size(temp,2)) ; logical(temp < low_voltage3) ; zeros(1,size(temp,2))]);
        rising = find(edges==1);     %rising/falling edges
        falling = find(edges==-1);  
        spanWidth = falling - rising;  %width of span of 1's (above threshold)
        low_v_count3(p) = sum(spanWidth >= 300/delta_time);
        
%         for iind=1:c
%             high_v3 = find(temp(:,iind) > high_voltage3);
%             low_v3 = find(temp(:,iind) < low_voltage3);
% 
%             if (sum(high_v3) > 0)       
%                 for mind=1:(r-300/delta_time)
%                     test1 = find(temp(mind:(mind+300/delta_time - 1),iind) > high_voltage3);
%                     if (length(test1) == 300/delta_time)         
%                         high_v_count3(p) = high_v_count3(p) + 1;
%                         break;
%                     end
%                 end
%             end
% 
%             if (sum(low_v3) > 0)
%                 for mind=1:(r-300/delta_time)
%                     test1 = find(temp(mind:(mind+300/delta_time - 1),iind) < low_voltage3);
%                     if (length(test1) == 300/delta_time)         
%                         low_v_count3(p) = low_v_count3(p) + 1;
%                         break;
%                     end
%                 end
%             end
%         end

        % Violation 6
        flicker_count(p) = 0;
 
        for iind=2:(r-time_between/delta_time)
            flicker_count(p) = flicker_count(p) + sum(logical(temp(iind+time_between/delta_time,:) > temp(iind,:)*(1+flicker_mag)));
            flicker_count(p) = flicker_count(p) + sum(logical(temp(iind+time_between/delta_time,:) < temp(iind,:)*(1-flicker_mag)));
        end  
        
%         flicker_count(p) = 0;
%         
%         tic
%         for iind=1:c
%             for mind=1:(r-time_between/delta_time)
%                 test1 = find(temp(mind:(mind+time_between/delta_time - 1),iind) > temp(mind,iind)*(1+flicker_mag));
%                 test2 = find(temp(mind:(mind+time_between/delta_time - 1),iind) < temp(mind,iind)*(1-flicker_mag));
% 
%                 if ( ~isempty(test1) )        
%                     flicker_count(p) = flicker_count(p) + 1;
%                     break;
%                 end
% 
%                 if ( ~isempty(test2) )        
%                     flicker_count(p) = flicker_count(p) + 1;
%                     break;
%                 end
% 
% 
%             end
%         end
%         toc
        
        
        p=p+1;
    end  
%%
    disp(' ');
    disp('ANSI Instantaneous Voltage Violations at Customer Meter');
    disp(['total meters = ' num2str(c)]);
    disp('-------------------------------------------------------');
    disp('              High Voltage   |   Low Voltage  ');
    disp('Meter Count');
    disp(['     DR0             ' num2str(high_v_count2(1)) '   |   ' num2str(low_v_count2(1))]);
    disp(['     DR2             ' num2str(high_v_count2(2)) '   |   ' num2str(low_v_count2(2))]);
    disp(['     DR3             ' num2str(high_v_count2(3)) '   |   ' num2str(low_v_count2(3))]);
    disp('Min/Max V');
    disp(['     DR0         ' num2str(m1(1)) '  |   ' num2str(m2(1))]);
    disp(['     DR2         ' num2str(m1(2)) '  |   ' num2str(m2(2))]);
    disp(['     DR3         ' num2str(m1(3)) '  |   ' num2str(m2(3))]);
    disp(' ');
    disp(' ');
    disp('ANSI Continuous Voltage Violations at Customer Meter');
    disp(['total meters = ' num2str(c)]);
    disp('-------------------------------------------------------');
    disp('                  High Voltage   |   Low Voltage  ');
    disp('Meter Count');
    disp(['     DR0                 ' num2str(high_v_count3(1)) '   |   ' num2str(low_v_count3(1))]);
    disp(['     DR2                 ' num2str(high_v_count3(2)) '   |   ' num2str(low_v_count3(2))]);
    disp(['     DR3                 ' num2str(high_v_count3(3)) '   |   ' num2str(low_v_count3(3))]);
    disp(' ');
    disp(' ');
    disp('Flicker Voltage Violations at Customer Meter');
    disp(['total meters = ' num2str(c)]);
    disp('-------------------------------------------------------');
    disp('                  Flicker  ');
    disp('Meter Count');
    disp(['     DR0             ' num2str(flicker_count(1))]);
    disp(['     DR2             ' num2str(flicker_count(2))]);
    disp(['     DR3             ' num2str(flicker_count(3))]);
    disp(' ');  
end

if overload_data == 1;
    disp('Transformer Overloads (200% overloaded)');
    disp(' Number of time steps with overloads)');
    disp('---------------------------------------');
    disp('Name               | Number of Overloads  ');
    disp('DR0');
    if size(data.DR0.overloads,2) == 3
        for mind=1:length(data.DR0.overloads{1,1})
            disp(['    ',char(data.DR0.overloads{1,1}(mind)) '     | ' num2str(data.DR0.overloads{1,2}(mind)) '                    ']);
        end
    else
        disp('    No transformer overloads detected.');
    end
    disp('DR2');
    if size(data.DR2.overloads,2) == 3
        for mind=1:length(data.DR2.overloads{1,1})
            disp(['    ',char(data.DR2.overloads{1,1}(mind)) '     | ' num2str(data.DR2.overloads{1,2}(mind)) '                    ']);
        end
    else
        disp('    No transformer overloads detected.');
    end
    disp('DR3');
    if size(data.DR3.overloads,2) == 3
        for mind=1:length(data.DR3.overloads{1,1})
            disp(['    ',char(data.DR3.overloads{1,1}(mind)) '     | ' num2str(data.DR3.overloads{1,2}(mind)) '                    ']);
        end
    else
        disp('    No transformer overloads detected.');
    end
    disp(' ');
    disp('Line Overloads (100% overloaded)');
    disp(' Number of time steps with overloads)');
    disp('---------------------------------------');
    disp('Name               | Number of Overloads  ');
    disp('DR0');
    if size(data.DR0.overloads,1) == 2
        for mind=1:length(data.DR0.overloads{2,1})
            disp(['    ',char(data.DR0.overloads{2,1}(mind)) '     | ' num2str(data.DR0.overloads{2,2}(mind)) '                    ']);
        end
    else
        disp('    No transformer overloads detected.');
    end
    disp('DR2');
    if size(data.DR2.overloads,1) == 2
        for mind=1:length(data.DR2.overloads{2,1})
            disp(['    ',char(data.DR2.overloads{2,1}(mind)) '     | ' num2str(data.DR2.overloads{2,2}(mind)) '                    ']);
        end
    else
        disp('    No transformer overloads detected.');
    end    
    disp('DR3');
    if size(data.DR3.overloads,1) == 2
        for mind=1:length(data.DR3.overloads{2,1})
            disp(['    ',char(data.DR3.overloads{2,1}(mind)) '     | ' num2str(data.DR3.overloads{2,2}(mind)) '                    ']);
        end
    else
        disp('    No transformer overloads detected.');
    end  
end
    
if regulatorCapacitorTaps == 1
    timestampInterval = round((data.DR0.main_regulator_timestamps(2)-data.DR0.main_regulator_timestamps(1))*(24*60*60));
    tapsDR0 = sum(abs(diff(data.DR0.regulator_taps(7200/timestampInterval:end,:))));
    tapsDR2 = sum(abs(diff(data.DR2.regulator_taps(7200/timestampInterval:end,:))));
    tapsDR3 = sum(abs(diff(data.DR3.regulator_taps(7200/timestampInterval:end,:))));
    
    disp('Tap Changes at Voltage Regulators');
    disp(['total regulators = ' num2str(size(tapsDR0,2)/3)]);
    disp('-------------------------------------------------------');
    disp('FEEDERREG');
    disp('  Phase             A         B         C');
    disp(['     DR0             ' num2str(tapsDR0(1)) '         ' num2str(tapsDR0(2)) '         ' num2str(tapsDR0(3))]);
    disp(['     DR2             ' num2str(tapsDR2(1)) '         ' num2str(tapsDR2(2)) '         ' num2str(tapsDR2(3))]);
    disp(['     DR3             ' num2str(tapsDR3(1)) '         ' num2str(tapsDR3(2)) '         ' num2str(tapsDR3(3))]);
    disp('VREG2');
    disp('  Phase             A         B         C');
    disp(['     DR0             ' num2str(tapsDR0(4)) '         ' num2str(tapsDR0(5)) '         ' num2str(tapsDR0(6))]);
    disp(['     DR2             ' num2str(tapsDR2(4)) '         ' num2str(tapsDR2(5)) '         ' num2str(tapsDR2(6))]);
    disp(['     DR3             ' num2str(tapsDR3(4)) '         ' num2str(tapsDR3(5)) '         ' num2str(tapsDR3(6))]);
    disp('VREG3');
    disp('  Phase             A         B         C');
    disp(['     DR0             ' num2str(tapsDR0(7)) '         ' num2str(tapsDR0(8)) '         ' num2str(tapsDR0(9))]);
    disp(['     DR2             ' num2str(tapsDR2(7)) '         ' num2str(tapsDR2(8)) '         ' num2str(tapsDR2(9))]);
    disp(['     DR3             ' num2str(tapsDR3(7)) '         ' num2str(tapsDR3(8)) '         ' num2str(tapsDR3(9))]);
    disp('VREG4');
    disp('  Phase             A         B         C');
    disp(['     DR0             ' num2str(tapsDR0(10)) '         ' num2str(tapsDR0(11)) '         ' num2str(tapsDR0(12))]);
    disp(['     DR2             ' num2str(tapsDR2(10)) '         ' num2str(tapsDR2(11)) '         ' num2str(tapsDR2(12))]);
    disp(['     DR3             ' num2str(tapsDR3(10)) '         ' num2str(tapsDR3(11)) '         ' num2str(tapsDR3(12))]);
    disp(' ');
    
    
    timestampInterval = round((data.DR0.capacitor_state_timestamps(2)-data.DR0.capacitor_state_timestamps(1))*(24*60*60));
    tapsDR0 = sum(abs(diff(data.DR0.capacitor_state(7200/timestampInterval:end,:))));
    tapsDR2 = sum(abs(diff(data.DR2.capacitor_state(7200/timestampInterval:end,:))));
    tapsDR3 = sum(abs(diff(data.DR3.capacitor_state(7200/timestampInterval:end,:))));
    
    disp('Tap Changes at Capacitors');
    disp(['total capacitors = ' num2str(size(tapsDR0,2)/3)]);
    disp('-------------------------------------------------------');
    disp('CAPBANK0');
    disp('  Phase             A         B         C');
    disp(['     DR0             ' num2str(tapsDR0(1)) '         ' num2str(tapsDR0(2)) '         ' num2str(tapsDR0(3))]);
    disp(['     DR2             ' num2str(tapsDR2(1)) '         ' num2str(tapsDR2(2)) '         ' num2str(tapsDR2(3))]);
    disp(['     DR3             ' num2str(tapsDR3(1)) '         ' num2str(tapsDR3(2)) '         ' num2str(tapsDR3(3))]);
    disp('CAPBANK1');
    disp('  Phase             A         B         C');
    disp(['     DR0             ' num2str(tapsDR0(4)) '         ' num2str(tapsDR0(5)) '         ' num2str(tapsDR0(6))]);
    disp(['     DR2             ' num2str(tapsDR2(4)) '         ' num2str(tapsDR2(5)) '         ' num2str(tapsDR2(6))]);
    disp(['     DR3             ' num2str(tapsDR3(4)) '         ' num2str(tapsDR3(5)) '         ' num2str(tapsDR3(6))]);
    disp('CAPBANK2');
    disp('  Phase             A         B         C');
    disp(['     DR0             ' num2str(tapsDR0(7)) '         ' num2str(tapsDR0(8)) '         ' num2str(tapsDR0(9))]);
    disp(['     DR2             ' num2str(tapsDR2(7)) '         ' num2str(tapsDR2(8)) '         ' num2str(tapsDR0(9))]);
    disp(['     DR3             ' num2str(tapsDR3(7)) '         ' num2str(tapsDR3(8)) '         ' num2str(tapsDR0(9))]);
    disp(' ');     
end
    
if voltageMapPlot == 1
    % -------------------------------------------------------------
    % now assign voltages to coordinates and identify p.u.
    % -------------------------------------------------------------
    load('coordinates.mat');
    nominal_volt = 240;
    
    p=1;
    for i = [0 2 3]
        node_names = eval(['data.DR',num2str(i),'.meter_voltage_12_string']);
        volt_data = eval(['data.DR',num2str(i),'.meter_voltage_12']);
        if i ~= 0 
            state_changes = eval(['data.DR',num2str(i),'.FORCED_ON | data.DR',num2str(i),'.FORCED_OFF']);
        end
        nind = 1;
        maxk = length(node_names);
        maxc = length(coordinates.X); 
        coordinate_names = cellfun(@(x) x(2:end),coordinates.name,'UniformOutput',false);
        node_names = cellfun(@(x) x(isstrprop(x,'digit')),node_names,'UniformOutput',false);
        node_names = cellfun(@(x) x(1:7),node_names,'UniformOutput',false);
        
        
        find_ind = zeros(20,maxk);
        
        for kind = 1:maxk
            find_val = strcmp(coordinate_names,node_names(kind));
            find_ind = find(find_val == 1);
            
            if  length(find_ind) == 1
                eval(['volt_DR',num2str(i),'.X(nind) = coordinates.X(find_ind);']);
                eval(['volt_DR',num2str(i),'.Y(nind) = coordinates.Y(find_ind);']);
                eval(['volt_DR',num2str(i),'.name{nind,:} = char(node_names(kind));']);
               
                % GLD voltages Mag in pu
                eval(['volt_DR',num2str(i),'.mag(:,nind) = volt_data(:,kind) ./ nominal_volt;']);
                if i ~=0
                    eval(['volt_DR',num2str(i),'.state_changes(:,nind) = state_changes(:,kind);']);  
                end
                
                if ( mod(nind,10) == 0 )
                     %disp(['Match ' num2str(nind) ' out of ' num2str(maxk)]);
                end
                nind = nind + 1;
            else
                if i == 0 
                    disp(['Could not find singular match in coordinates for ', char(node_names(kind)),' L = ' num2str(length(find_ind))]); 
                end
            end
            clear find_ind find_val
        end
    end
    
    VREG.names = {'E192860','190-8593','190-8581','190-7361'}; %sub / reg1    
    for iind = 1 : length(VREG.names)
        find_ind = find(strcmp(coordinates.name,VREG.names(iind)) == 1);
        VREG.X(iind) = coordinates.X(find_ind);
        VREG.Y(iind) = coordinates.Y(find_ind);
    end
    
    CAPBANKS.names = {'R20185','R42247','R42246','R18242'}; %sub / reg1 
    for iind = 1 : length(CAPBANKS.names)
        find_ind = find(strcmp(coordinates.name,CAPBANKS.names(iind)) == 1);
        CAPBANKS.X(iind) = coordinates.X(find_ind);
        CAPBANKS.Y(iind) = coordinates.Y(find_ind);
    end
    
    volt.X = volt_DR0.X - min(volt_DR0.X);
    volt.Y = volt_DR0.Y - min(volt_DR0.Y);
    VREG.X = VREG.X - min(volt_DR0.X);
    VREG.Y = VREG.Y - min(volt_DR0.Y);
    CAPBANKS.X = CAPBANKS.X - min(volt_DR0.X);
    CAPBANKS.Y = CAPBANKS.Y - min(volt_DR0.Y);
    volt.X = volt.X/5280;
    volt.Y = volt.Y/5280;
    VREG.X = VREG.X/5280;
    VREG.Y = VREG.Y/5280;
    CAPBANKS.X = CAPBANKS.X/5280;
    CAPBANKS.Y = CAPBANKS.Y/5280;

    maxX = max(volt.X);
    maxY = max(volt.Y);
    minX = min(volt.X);
    minY = min(volt.Y);

    minV_DR0 = min(min(volt_DR0.mag));
    minV_DR2 = min(min(volt_DR2.mag));
    minV_DR3 = min(min(volt_DR3.mag));
    maxV_DR0 = max(max(volt_DR0.mag));
    maxV_DR2 = max(max(volt_DR2.mag));
    maxV_DR3 = max(max(volt_DR3.mag));
    
    minV = min([minV_DR0 minV_DR2 minV_DR3]);
    maxV = max([maxV_DR0 maxV_DR2 maxV_DR3]);
    
    if minV < 0.825 || maxV > 1.145
        minV
        maxV
        error('minimum or maximum voltage pu will not work')
    else
        minV = 0.825;
        maxV = 1.145;       
    end
    
    volt_to_plot_DR0 = volt_DR0.mag(voltage_point-22,:);
    
    volt_to_plot_DR2 = volt_DR2.mag(voltage_point,:);
    state_changes_DR2 = volt_DR2.state_changes(voltage_point,:);
    indexDR2 = find(state_changes_DR2 == 1);
    indexNotDR2 = find(state_changes_DR2 == 0);
    volt_to_plot_DR3 = volt_DR3.mag(voltage_point,:);
    state_changes_DR3 = volt_DR3.state_changes(voltage_point,:);
    indexDR3 = find(state_changes_DR3 == 1);
    indexNotDR3 = find(state_changes_DR3 == 0);
    
    figure();
    subplot(1,3,1)
    scatter([volt.X minX-1 minX-1],[volt.Y minY-1 minY-1],8,[volt_DR0.mag(voltage_point,:) minV maxV],'o');
    hold on
    scatter(VREG.X,VREG.Y,70,'r','o','filled');
    scatter(CAPBANKS.X,CAPBANKS.Y,70,'b','o','filled');
    xlim([0 maxX])
    ylim([0 maxY])
    title('Voltage magnitude for DR 0');
    xlabel('miles')
    ylabel('miles')
    colormap(jet);
    hcbar = colorbar('location','eastoutside');
    set(get(hcbar,'ylabel'),'string','Voltage in per unit','fontsize',12)
    subplot(1,3,2)   
    scatter([volt.X(indexDR2) minX-1 minX-1],[volt.Y(indexDR2) minY-1 minY-1],8,[volt_to_plot_DR2(indexDR2) minV maxV],'+');
    hold on
    scatter([volt.X(indexNotDR2) minX-1 minX-1],[volt.Y(indexNotDR2) minY-1 minY-1],8,[volt_to_plot_DR2(indexNotDR2) minV maxV],'o');
    scatter(VREG.X,VREG.Y,70,'r','o','filled');
    scatter(CAPBANKS.X,CAPBANKS.Y,70,'b','o','filled');
    %scatter([volt.X minX-1 minX-1],[volt.Y minY-1 minY-1],6,[volt_DR2.mag(voltage_point,:) minV maxV]);
    xlim([0 maxX])
    ylim([0 maxY])
    title('Voltage magnitude for DR 2');
    xlabel('miles')
    ylabel('miles')
    colormap(jet);
    hcbar = colorbar('location','eastoutside');
    set(get(hcbar,'ylabel'),'string','Voltage in per unit','fontsize',12)
    subplot(1,3,3)
    scatter([volt.X(indexDR3) minX-1 minX-1],[volt.Y(indexDR3) minY-1 minY-1],8,[volt_to_plot_DR3(indexDR3) minV maxV],'+');
    hold on
    scatter([volt.X(indexNotDR3) minX-1 minX-1],[volt.Y(indexNotDR3) minY-1 minY-1],8,[volt_to_plot_DR3(indexNotDR3) minV maxV],'o');
    scatter(VREG.X,VREG.Y,70,'r','o','filled');
    scatter(CAPBANKS.X,CAPBANKS.Y,70,'b','o','filled');
    %scatter([volt.X minX-1 minX-1],[volt.Y minY-1 minY-1],6,[volt_DR3.mag(voltage_point,:) minV maxV]);
    xlim([0 maxX])
    ylim([0 maxY])
    title('Voltage magnitude for DR 3');
    xlabel('miles')
    ylabel('miles')
    colormap(jet);
    hcbar = colorbar('location','eastoutside');
    set(get(hcbar,'ylabel'),'string','Voltage in per unit','fontsize',12)

    figure();
    subplot(1,3,1)
    %scatter([volt.X minX-1 minX-1],[volt.Y minY-1 minY-1],8,[volt_to_plot_DR0 minV maxV],'o');
    scatter([volt.X minX-1 minX-1],[volt.Y minY-1 minY-1],8,[volt_DR0.mag(voltage_point,:) minV maxV],'o');
    hold on
    scatter(VREG.X,VREG.Y,70,'r','o','filled');
    scatter(CAPBANKS.X,CAPBANKS.Y,70,'b','o','filled');
    xlim([0 maxX])
    ylim([0 maxY])
    title('Voltage magnitude for DR 0');
    xlabel('miles')
    ylabel('miles')
    colormap(jet);
    hcbar = colorbar('location','eastoutside');
    set(get(hcbar,'ylabel'),'string','Voltage in per unit','fontsize',12)
    subplot(1,3,2)   
    scatter([volt.X(indexDR2) minX-1 minX-1],[volt.Y(indexDR2) minY-1 minY-1],8,[volt_to_plot_DR2(indexDR2) minV maxV],'+');
    hold on
    %scatter([volt.X(indexNotDR2) minX-1 minX-1],[volt.Y(indexNotDR2) minY-1 minY-1],8,[volt_to_plot_DR2(indexNotDR2) minV maxV],'o');
    scatter([volt.X(indexNotDR2) minX-1 minX-1],[volt.Y(indexNotDR2) minY-1 minY-1],8,[0.94 0.94 0.94],'o');
    %scatter([volt.X minX-1 minX-1],[volt.Y minY-1 minY-1],6,[volt_DR2.mag(voltage_point,:) minV maxV]);
    scatter(VREG.X,VREG.Y,70,'r','o','filled');
    scatter(CAPBANKS.X,CAPBANKS.Y,70,'b','o','filled');
    xlim([0 maxX])
    ylim([0 maxY])
    title('Voltage magnitude for DR 2');
    xlabel('miles')
    ylabel('miles')
    colormap(jet);
    hcbar = colorbar('location','eastoutside');
    set(get(hcbar,'ylabel'),'string','Voltage in per unit','fontsize',12)
    subplot(1,3,3)
    scatter([volt.X(indexDR3) minX-1 minX-1],[volt.Y(indexDR3) minY-1 minY-1],8,[volt_to_plot_DR3(indexDR3) minV maxV],'+');
    hold on
    %scatter([volt.X(indexNotDR3) minX-1 minX-1],[volt.Y(indexNotDR3) minY-1 minY-1],8,[volt_to_plot_DR3(indexNotDR3) minV maxV],'o');
    scatter([volt.X(indexNotDR3) minX-1 minX-1],[volt.Y(indexNotDR3) minY-1 minY-1],8,[0.94 0.94 0.94],'o');
    %scatter([volt.X minX-1 minX-1],[volt.Y minY-1 minY-1],6,[volt_DR3.mag(voltage_point,:) minV maxV]);
    scatter(VREG.X,VREG.Y,70,'r','o','filled');
    scatter(CAPBANKS.X,CAPBANKS.Y,70,'b','o','filled');
    xlim([0 maxX])
    ylim([0 maxY])
    title('Voltage magnitude for DR 3');
    xlabel('miles')
    ylabel('miles')
    colormap(jet);
    hcbar = colorbar('location','eastoutside');
    set(get(hcbar,'ylabel'),'string','Voltage in per unit','fontsize',12)

end
