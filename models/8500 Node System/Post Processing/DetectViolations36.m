clear;
clc;

load('voltage_data.mat');

time_step = 4; % (secs) time step of data

% Violation 2 - ANSI bands, instantaneous
high_voltage2 = 240*1.10;
low_voltage2 = 240*0.9;

% Violation 3 - ANSI bands, 5-min
high_voltage3 = 240*1.05;
low_voltage3 = 240*0.95;

% Violation 6 - flicker
% 4 secs == 0.013; 20 secs == 0.016; 60 secs == 0.02; 300 secs == 0.027
time_between = 4; % (secs) time interval to look for flicker
flicker_mag = 0.013; % (perc) threshold of voltage change between time steps


%% Violation 2 
[r,c] = size(data);
high_v_count2 = 0;
low_v_count2 = 0;

for jind=1:c
    high_v2 = find(data(:,jind) > high_voltage2);
    low_v2 = find(data(:,jind) < low_voltage2);

    if (sum(high_v2) > 0)
        high_v_count2 = high_v_count2 + 1;
    end
    
    if (sum(low_v2) > 0)
        low_v_count2 = low_v_count2 + 1;
    end
end

m1 = max(max(data));
m2 = min(min(data));

disp(' ');
disp('ANSI Instantaneous Voltage Violations at Customer Meter');
disp(['total meters = ' num2str(c)]);
disp('-------------------------------------------------------');
disp('             High Voltage   |   Low Voltage  ');
disp(['Meter Count          ' num2str(high_v_count2) '    |      ' num2str(low_v_count2)]);
disp(['Min/Max V        ' num2str(m1) '    |      ' num2str(m2)]);
disp(' ');


%% Violation 3
high_v_count3 = 0;
low_v_count3 = 0;

for iind=1:c
    high_v3 = find(data(:,iind) > high_voltage3);
    low_v3 = find(data(:,iind) < low_voltage3);

    if (sum(high_v3) > 0)       
        for mind=1:(r-300/time_step)
            test1 = find(data(mind:(mind+300/time_step - 1),iind) > high_voltage3);
            if (length(test1) == 300/time_step)         
                high_v_count3 = high_v_count3 + 1;
                break;
            end
        end
    end
    
    if (sum(low_v3) > 0)
        for mind=1:(r-300/time_step)
            test1 = find(data(mind:(mind+300/time_step - 1),iind) < low_voltage3);
            if (length(test1) == 300/time_step)         
                low_v_count3 = low_v_count3 + 1;
                break;
            end
        end
    end
end

disp(' ');
disp('ANSI Continuous Voltage Violations at Customer Meter');
disp(['total meters = ' num2str(c)]);
disp('-------------------------------------------------------');
disp('             High Voltage   |   Low Voltage  ');
disp(['Meter Count          ' num2str(high_v_count3) '    |      ' num2str(low_v_count3)]);
disp(' ');

%% Violation 6
flicker_count = 0;

for iind=1:c
    for mind=1:(r-time_between/time_step)
        test1 = find(data(mind:(mind+time_between/time_step - 1),iind) > data(mind,iind)*(1+flicker_mag));
        test2 = find(data(mind:(mind+time_between/time_step - 1),iind) < data(mind,iind)*(1-flicker_mag));
        
        if ( ~isempty(test1) )        
            flicker_count = flicker_count + 1;
            break;
        end
        
        if ( ~isempty(test2) )        
            flicker_count = flicker_count + 1;
            break;
        end
        
        
    end
end

disp(' ');
disp('Flicker Voltage Violations at Customer Meter');
disp(['total meters = ' num2str(c)]);
disp('-------------------------------------------------------');
disp('             Flicker  ');
disp(['Meter Count          ' num2str(flicker_count)]);
disp(' ');

fclose('all');