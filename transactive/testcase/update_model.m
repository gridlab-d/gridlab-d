% update matlab model from WECC model data

clear all; clc;

regen = false; % 'true' reloads all source files and reprocesses all data

%% Load bus/branch data
fprintf('Loading source data...');
if ~exist('bus.mat','file') || regen
    bus = readtable('bus.xlsx');
    save('bus.mat','bus');
else
    load('bus.mat');
end
if ~exist('branch.mat','file')  || regen
    branch = readtable('branch.xlsx');
    save('branch.mat','branch');
else
    load('branch.mat');
end
if ~exist('generator.mat','file') || regen
    generator = readtable('generator.xlsx');
    save('generator.mat','generator');
else
    load('generator.mat');
end
if ~exist('loadarea.mat','file') || regen
    loadarea = readtable('load.xlsx');
    save('loadarea.mat','loadarea');
else
    load('loadarea.mat');
end
if ~exist('region.mat','file') || regen
    region = readtable('region.xlsx');
    save('region.mat','region');
else
    load('region.mat');
end
fprintf(' done.\n');

%% Construct network model
if ~exist('controlarea.mat','file') || regen
    fprintf('Generating controlarea data... ');
    
    % find the buses strictly within each load area
    loadarea_id = unique(bus.LoadAreaID);
    for n = 1:length(loadarea_id)
        AreaId(n,1) = loadarea_id(n);
%        BusList{n,1} = bus.BusID(bus.LoadAreaID==loadarea_id(n));
%        Generators{n,1} = find(generator.LoadAreaID==loadarea_id(n));
        loadarea_index = find(loadarea.LoadAreaID==loadarea_id(n));
        AreaName{n,1} = loadarea.LoadAreaName(loadarea_index);
        BaseLoad(n,1) = loadarea.BaseLoadMW(loadarea_index);
        Losses(n,1) = loadarea.LossPercentage(loadarea_index);
        generator_index = find(generator.LoadAreaID==loadarea_id(n));
        RatedCapacity(n,1) = sum(generator.PSSEMaxCap(generator_index));
    end
    controlarea = table(AreaId,AreaName,RatedCapacity,BaseLoad,Losses);
    save('controlarea.mat','controlarea');
    fprintf(' done.\n');
else
    load('controlarea.mat');
end

if ~exist('intertie.mat','file') || regen
    fprintf('Generating intertie data... ');
    
    % build loadarea index for busses
    loadarea_index = nan(1,max(bus.BusID));
    loadarea_index(bus.BusID) = bus.LoadAreaID;

    % find branches between load areas
    intertie_index = find(loadarea_index(branch.FromBus)~=loadarea_index(branch.ToBus));
    intertie_id = branch.BranchID(intertie_index);

    % combine branches into interties
    for n = 1:length(intertie_index) % n is index to branch rows
        from = loadarea_index(branch.FromBus(intertie_index(n)));
        to = loadarea_index(branch.ToBus(intertie_index(n)));
        intertie_table{from,to} = find((loadarea_index(branch.FromBus)==from & loadarea_index(branch.ToBus)==to) | (loadarea_index(branch.FromBus)==to & loadarea_index(branch.ToBus)==from));
    end
    c = 0;
    for n = 1:size(intertie_table,1)
        for m = n:size(intertie_table,2)
            if length(intertie_table{m,n})>0 || length(intertie_table{n,m})>0
                p = unique([intertie_table{m,n},intertie_table{n,m}]);
                intertie_table{m,n} = p;
                intertie_table{n,m} = p;
                c = c+1;
                IntertieId(c,1) = c;
                FromAreaId(c,1) = m;
                ToAreaId(c,1) = n;
                pl = p(branch.Length(p)>0);
                zl = p(branch.Length(p)==0);
                rx = 1/(sum(1./(branch.R(zl)+j*branch.X(zl))) + sum(1./(branch.Length(pl).*(branch.R(pl)+j*branch.X(pl)))));
                R(c,1) = real(rx);
                X(c,1) = imag(rx);
                B(c,1) = sum(branch.B(p));
                RateA(c,1) = sum(branch.RateA(p));
                RateB(c,1) = sum(branch.RateB(p));
                RateC(c,1) = sum(branch.RateC(p));
            end
        end
    end
    intertie = table(IntertieId,FromAreaId,ToAreaId,R,X,B,RateA,RateB,RateC);
    save('intertie.mat','intertie');
    fprintf(' done.\n');
else
    load('intertie.mat');
end

