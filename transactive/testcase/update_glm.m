% update GLM file from matlab model

clear all; clc;
load('controlarea.mat');
load('intertie.mat');
map = readtable('control_area_map.csv');

glm = fopen('test_wecc2024_case.glm','w');

fprintf(glm,'// generated automatically by %s on %s\n\n',mfilename,datestr(now));
fprintf(glm,'#include "options.glm"\n\n');
fprintf(glm,'script on_term start "test_wecc2024_case.kml";\n\n');

fprintf(glm,['clock {\n\t'...
    'timezone PST+8PDT;\n\t'...
    'starttime ''2000-01-01 00:00:00 PST'';\n\t'...
    'stoptime ''2000-01-01 01:00:00 PST'';\n'...
    '}\n\n']);

% define interconnection
model.class = 'interconnection';
model.name = 'WECC2024';
model.initialize = 'STEADY';
model.initial_balancing_unit = 'WALC_G';
model.latitude = '40.7714N';
model.longitude = '111.9025W';
model.damping = '1.0';
model.frequency = '60';
glm_write(glm,model);

% define controlarea
for m = 1:size(controlarea,1)
    loadloss = controlarea(m,:).Losses;
    clear area;
    area.class = sprintf('controlarea:%d',controlarea(m,:).AreaId);
    area.name = char(controlarea(m,:).AreaName{1});
    area.parent = 'WECC2024';
    area.internal_losses = sprintf('%g %%',loadloss*100);
    
    n = find(map.LoadAreaID==controlarea(m,:).AreaId);
    if ~isnan(map.XCoordinate(n)) && ~isnan(map.YCoordinate(n))
        area.latitude = sprintf('%.0fN%.0f:%.0f',map.YCoordinate(n),mod(map.YCoordinate(n),1)*60,mod(map.YCoordinate(n)*60,1)*60);
        area.longitude = sprintf('%.0fW%.0f:%.0f',-map.XCoordinate(n),mod(-map.XCoordinate(n),1)*60,mod(-map.XCoordinate(n)*60,1)*60);
    end
    
    %% define load
    baseload = controlarea(m,:).BaseLoad;
    clear areaload;
    areaload.class = 'load';
    areaload.name = [area.name,'_L'];
    areaload.capacity = sprintf('%g MW',baseload);
    areaload.actual = sprintf('%g MW',baseload * 0.8); % TODO fix this
    areaload.inertia = sprintf('%g MJ',baseload * 8 ); % TODO fix this
    areaload.ufls = '" 59.2 0.10 ; 58.8 0.15 ; 58.0 0.20 "'; % see Kundur pg. 626
    areaload.control_flags = 'UFLS';
    areaload.X = '" 0 ; 0 "';
    areaload.A = '"0.7 0.6 ; 0.3 0.4"';
    areaload.B = '"0.003 ; 0.007"';
    areaload.C = '"1 0"';
    areaload.D = '"0"';
    areaload.U = '"parent.ace"';
    
    % load recorder
    areaload.child{1}.class = 'recorder';
    areaload.child{1}.property = 'capacity[GW],schedule[GW],actual[GW],inertia[GJ],A,X,B,Y,Y,D';
    areaload.child{1}.file = [areaload.name,'.csv'];
    areaload.child{1}.interval = '4';
    area.child{1} = areaload;
    
    %% define generator
    actualgen = baseload * 0.8 * (1+loadloss);
%    if m==1; actualgen = actualgen*0.95; end; % add a small power discrepancy in one area
% four corners loss
    if strcmp(area.name,'WALC') actualgen = actualgen - 1000; end; % single generator loss
%    actualgen = actualgen * (randn*0.03+1); % deviate generation randomly

    clear areagen;
    areagen.class = 'generator';
    areagen.name = [area.name,'_G'];
    areagen.capacity = sprintf('%g MW',baseload*(1+loadloss)); %controlarea(m,:).GenerationCapacity);
    areagen.actual = sprintf('%g MW',actualgen); % TODO fix this
    areagen.inertia =  sprintf('%g MJ',actualgen * 10); % TODO fix this
    areagen.schedule = areagen.actual;
    areagen.control_gain = '0.5';
    areagen.frequency_deadband = sprintf('%g Hz',0.036/m); % CA 1 has 0.036, 2 has 0.018, etc.
    areagen.control_flags = 'DROOP|AGC';
    
    % generation recorder
    areagen.child{1}.class = 'recorder';
    areagen.child{1}.property = 'capacity[GW],schedule[GW],actual[GW],inertia[GJ]';
    areagen.child{1}.file = [areagen.name,'.csv'];
    areagen.child{1}.interval = '4';
    area.child{2} = areagen;
    
    %% finalize controlarea 
%    area.bias = sprintf('%g',0.2*baseload);
    area.schedule = '0';
    area.child{3}.class = 'recorder';
    area.child{3}.property = 'actual[GW],schedule[GW],capacity[GW],supply[GW],demand[GW],losses[MW],inertia[GJ],bias[MW/Hz],imbalance[MW],ace[MW]';
    area.child{3}.file = [area.name,'.csv'];
    area.child{3}.interval = '4';
    
    % attach controlarea to interconnection
    glm_write(glm,area);
end

recorder.class = 'recorder';
recorder.parent = 'WECC2024';
recorder.property = 'capacity[GW],supply[GW],demand[GW],losses[MW],imbalance[GW],inertia[GJ],frequency[Hz]';
recorder.file = 'WECC2024.csv';
recorder.interval = '4';
glm_write(glm,recorder);

for m = 1:size(intertie,1)
    clear line;
    line.class = 'intertie';
    line.parent = 'WECC2024';
    line.from = sprintf('controlarea:%d',intertie(m,:).FromAreaId);
    line.to = sprintf('controlarea:%d',intertie(m,:).ToAreaId);
    line.capacity = num2str(intertie(m,:).RateA);
    line.loss = '0.0'; % TODO fix me
    glm_write(glm,line);
end

fclose(glm);