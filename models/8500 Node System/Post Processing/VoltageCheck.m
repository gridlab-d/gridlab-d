clear;
clc;

dir = 'C:\Users\d3x289\Documents\LAAR\8500NodeTest\8500NodeTest\';

load([dir 'voltage_distance_data.mat']);

% do we want to evaluate all of the files?
files_to_evalute = 195:(195+150);

% voltage ranges to check
low_v_range_a = 114 / 120;
high_v_range_a = 126 / 120;

low_v_range_b = 110 / 120;
high_v_range_b = 126.5 / 120;

% initial conditions
secs_over_violation_a_range_a = 0;
secs_over_violation_b_range_a = 0;
secs_over_violation_c_range_a = 0;

secs_under_violation_a_range_a = 0;
secs_under_violation_b_range_a = 0;
secs_under_violation_c_range_a = 0;

secs_over_violation_a_range_b = 0;
secs_over_violation_b_range_b = 0;
secs_over_violation_c_range_b = 0;

secs_under_violation_a_range_b = 0;
secs_under_violation_b_range_b = 0;
secs_under_violation_c_range_b = 0;

secs_over_violation_a_range_a_trip = 0;
secs_under_violation_a_range_a_trip = 0;
secs_over_violation_a_range_b_trip = 0;
secs_under_violation_a_range_b_trip = 0;

first_time = 1;

for kind=files_to_evalute
        % Extract the pu values...but separate out the zero values
    volt.pu_a = sqrt(data.voltage{kind}(:,1).^2 + data.voltage{kind}(:,2).^2) ./ data.volt_pu(:);
        mag_ind_a = find(volt.pu_a ~= 0);
    volt.pu_b = sqrt(data.voltage{kind}(:,3).^2 + data.voltage{kind}(:,4).^2) ./ data.volt_pu(:);
        mag_ind_b = find(volt.pu_b ~= 0);
    volt.pu_c = sqrt(data.voltage{kind}(:,5).^2 + data.voltage{kind}(:,6).^2) ./ data.volt_pu(:);
        mag_ind_c = find(volt.pu_c ~= 0);
    
    over_volt_ind_a_range_a = find(volt.pu_a(mag_ind_a) > high_v_range_a);     
    over_volt_ind_b_range_a = find(volt.pu_b(mag_ind_b) > high_v_range_a); 
    over_volt_ind_c_range_a = find(volt.pu_c(mag_ind_c) > high_v_range_a); 
    
    under_volt_ind_a_range_a = find(volt.pu_a(mag_ind_a) < low_v_range_a);     
    under_volt_ind_b_range_a = find(volt.pu_b(mag_ind_b) < low_v_range_a); 
    under_volt_ind_c_range_a = find(volt.pu_c(mag_ind_c) < low_v_range_a); 
    
    secs_over_violation_a_range_a =  length(over_volt_ind_a_range_a) * 4 + secs_over_violation_a_range_a;
    secs_over_violation_b_range_a =  length(over_volt_ind_b_range_a) * 4 + secs_over_violation_b_range_a;
    secs_over_violation_c_range_a =  length(over_volt_ind_c_range_a) * 4 + secs_over_violation_c_range_a;
    
    secs_under_violation_a_range_a =  length(under_volt_ind_a_range_a) * 4 + secs_under_violation_a_range_a;
    secs_under_violation_b_range_a =  length(under_volt_ind_b_range_a) * 4 + secs_under_violation_b_range_a;
    secs_under_violation_c_range_a =  length(under_volt_ind_c_range_a) * 4 + secs_under_violation_c_range_a;
    
    over_volt_ind_a_range_b = find(volt.pu_a(mag_ind_a) > high_v_range_b);     
    over_volt_ind_b_range_b = find(volt.pu_b(mag_ind_b) > high_v_range_b); 
    over_volt_ind_c_range_b = find(volt.pu_c(mag_ind_c) > high_v_range_b); 
    
    under_volt_ind_a_range_b = find(volt.pu_a(mag_ind_a) < low_v_range_b);     
    under_volt_ind_b_range_b = find(volt.pu_b(mag_ind_b) < low_v_range_b); 
    under_volt_ind_c_range_b = find(volt.pu_c(mag_ind_c) < low_v_range_b); 
    
    secs_over_violation_a_range_b =  length(over_volt_ind_a_range_b) * 4 + secs_over_violation_a_range_b;
    secs_over_violation_b_range_b =  length(over_volt_ind_b_range_b) * 4 + secs_over_violation_b_range_b;
    secs_over_violation_c_range_b =  length(over_volt_ind_c_range_b) * 4 + secs_over_violation_c_range_b;
    
    secs_under_violation_a_range_b =  length(under_volt_ind_a_range_b) * 4 + secs_under_violation_a_range_b;
    secs_under_violation_b_range_b =  length(under_volt_ind_b_range_b) * 4 + secs_under_violation_b_range_b;
    secs_under_violation_c_range_b =  length(under_volt_ind_c_range_b) * 4 + secs_under_violation_c_range_b;
    
    
    % Now do it for the triplex nodes only - assume V1 & V2 are equivalent
    if (first_time == 1)
        triplex_find = strncmp(data.voltage_names,'SX',2);
        triplex_ind = find(triplex_find == 1);
        first_time = 0;
    end
    
    over_volt_ind_a_range_a_trip = find(volt.pu_a(triplex_ind) > high_v_range_a);      
    
    under_volt_ind_a_range_a_trip = find(volt.pu_a(triplex_ind) < low_v_range_a);     
    
    secs_over_violation_a_range_a_trip =  length(over_volt_ind_a_range_a_trip) * 4 + secs_over_violation_a_range_a_trip;
    
    secs_under_violation_a_range_a_trip =  length(under_volt_ind_a_range_a_trip) * 4 + secs_under_violation_a_range_a_trip;
    
    over_volt_ind_a_range_b_trip = find(volt.pu_a(triplex_ind) > high_v_range_b);      
    
    under_volt_ind_a_range_b_trip = find(volt.pu_a(triplex_ind) < low_v_range_b);     
    
    secs_over_violation_a_range_b_trip =  length(over_volt_ind_a_range_b_trip) * 4 + secs_over_violation_a_range_b_trip;
    
    secs_under_violation_a_range_b_trip =  length(under_volt_ind_a_range_b_trip) * 4 + secs_under_violation_a_range_b_trip;

end

display('Range A Violations (minutes times nodes)');
display(' ');
display(['Over Voltage Violation Phase A   ' num2str(secs_over_violation_a_range_a/60)]);
display(['Over Voltage Violation Phase B   ' num2str(secs_over_violation_b_range_a/60)]);
display(['Over Voltage Violation Phase C   ' num2str(secs_over_violation_c_range_a/60)]);
display(' ');
display(['Under Voltage Violation Phase A  ' num2str(secs_under_violation_a_range_a/60)]);
display(['Under Voltage Violation Phase B  ' num2str(secs_under_violation_b_range_a/60)]);
display(['Under Voltage Violation Phase C  ' num2str(secs_under_violation_c_range_a/60)]);

display(' ');
display(' ');
display('Range B Violations (minutes times nodes)');
display(' ');
display(['Over Voltage Violation Phase A   ' num2str(secs_over_violation_a_range_b/60)]);
display(['Over Voltage Violation Phase B   ' num2str(secs_over_violation_b_range_b/60)]);
display(['Over Voltage Violation Phase C   ' num2str(secs_over_violation_c_range_b/60)]);
display(' ');
display(['Under Voltage Violation Phase A  ' num2str(secs_under_violation_a_range_b/60)]);
display(['Under Voltage Violation Phase B  ' num2str(secs_under_violation_b_range_b/60)]);
display(['Under Voltage Violation Phase C  ' num2str(secs_under_violation_c_range_b/60)]);
display(' ');
display(' ');

display('Range A Violations Triplex (minutes times nodes)');
display(' ');
display(['Over Voltage Violation Phase 1   ' num2str(secs_over_violation_a_range_a_trip/60)]);
display(['Under Voltage Violation Phase 1  ' num2str(secs_under_violation_a_range_a_trip/60)]);

display(' ');
display('Range B Violations Triplex (minutes times nodes)');
display(' ');
display(['Over Voltage Violation Phase 1   ' num2str(secs_over_violation_a_range_b_trip/60)]);
display(['Under Voltage Violation Phase 1  ' num2str(secs_under_violation_a_range_b_trip/60)]);

