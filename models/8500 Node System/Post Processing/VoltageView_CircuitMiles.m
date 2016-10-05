clear;
clc;

dir = '';%'C:\Users\d3x289\Documents\LAAR\3.1\VS2005\x64\Release\DR Run 01 14 2015\';

load([dir 'voltage_distance_data.mat']);

primary_only = 0; % 1=TRUE, 0=FALSE

%%
% -------------------------------------------------------------
% now pull out only voltages which found a coordinate match and p.u.ize
% -------------------------------------------------------------

% Get rid of ones that don't have an X/Y coordinate
coord_ind = find(data.volt_length ~= -999999);

% Adjust the lengths to miles
volt.length = data.volt_length(coord_ind)/5280;
volt.parent_ind = data.parent_index(coord_ind);

maxX = max(volt.length);
maxY = 126;
minX = min(volt.length);
minY = 114;

%% Plotting time series
no_samples = 435;
year = 2000 * ones(1,no_samples);
month = 9 * ones(1,no_samples);
day = 1 * ones(1,no_samples);
hour = 15 * ones(1,no_samples);
minutes = 46 * ones(1,no_samples);
seconds = 1:4:435*4;

xdate = datenum(year,month,day,hour,minutes,seconds);

%%
for kind=196:196
    % Extract the pu values
    data.pu_a = 120 * (sqrt(data.voltage{kind}(:,1).^2 + data.voltage{kind}(:,2).^2) ./ data.volt_pu(:));
    data.pu_b = 120 * (sqrt(data.voltage{kind}(:,3).^2 + data.voltage{kind}(:,4).^2) ./ data.volt_pu(:));
    data.pu_c = 120 * (sqrt(data.voltage{kind}(:,5).^2 + data.voltage{kind}(:,6).^2) ./ data.volt_pu(:));
    
    % Start plotting
    titlestr = 'Post-load shed:  ';

    hfig = figure(1001);
    clf(1001);
    hold on;
    for jind=1:length(data.pu_a)
        % only the primary for now
        if (data.volt_pu(jind) > 240)
            if (data.pu_a(jind) ~= 0)
                if (data.volt_length(jind) > 0)
                    if (data.parent_index(jind) ~= 0)
                        plot([data.volt_length(jind)/5280; data.volt_length(data.parent_index(jind))/5280],[data.pu_a(jind); data.pu_a(data.parent_index(jind))],'-o','MarkerSize',2);
                    else
                        plot(data.volt_length(jind)/5280,data.pu_a(jind),'-o','MarkerSize',2);
                    end
                end
            end
        elseif (data.phases(jind) == 1 && primary_only == 0)
            if (data.volt_length(jind) > 0)
                if (data.parent_index(jind) ~= 0)
                    plot([data.volt_length(jind)/5280; data.volt_length(data.parent_index(jind))/5280],[data.pu_a(jind); data.pu_a(data.parent_index(jind))],':.','MarkerSize',2);
                else
                    plot(data.volt_length(jind)/5280,data.pu_a(jind),':.','MarkerSize',2);
                end
            end 
        end
    end

    for jind=1:length(data.pu_b)
        if (data.volt_pu(jind) > 240)
            if (data.pu_b(jind) ~= 0)
                if (data.volt_length(jind) > 0)
                    if (data.parent_index(jind) ~= 0)
                        plot([data.volt_length(jind)/5280; data.volt_length(data.parent_index(jind))/5280],[data.pu_b(jind); data.pu_b(data.parent_index(jind))],'-og','MarkerSize',2);
                    else
                        plot(data.volt_length(jind)/5280,data.pu_b(jind),'-og','MarkerSize',2);
                    end
                end
            end
        elseif (data.phases(jind) == 2 && primary_only == 0)
            if (data.volt_length(jind) > 0)
                if (data.parent_index(jind) ~= 0)
                    if (data.phases(data.parent_index(jind)) == 0)
                        plot([data.volt_length(jind)/5280; data.volt_length(data.parent_index(jind))/5280],[data.pu_a(jind); data.pu_b(data.parent_index(jind))],':.g','MarkerSize',2);
                    else
                        plot([data.volt_length(jind)/5280; data.volt_length(data.parent_index(jind))/5280],[data.pu_a(jind); data.pu_a(data.parent_index(jind))],':.g','MarkerSize',2);
                    end
                else
                    plot(data.volt_length(jind)/5280,data.pu_b(jind),':.g','MarkerSize',2);
                end
            end
        end
    end

    for jind=1:length(data.pu_c)
        if (data.volt_pu(jind) > 240)
            if (data.pu_c(jind) ~= 0)
                if (data.volt_length(jind) > 0)
                    if (data.parent_index(jind) ~= 0)
                        plot([data.volt_length(jind)/5280; data.volt_length(data.parent_index(jind))/5280],[data.pu_c(jind); data.pu_c(data.parent_index(jind))],'-ok','MarkerSize',2);
                    else
                        plot(data.volt_length(jind)/5280,data.pu_c(jind),'-ok','MarkerSize',2);
                    end
                end
            end
        elseif (data.phases(jind) == 3 && primary_only == 0)
            if (data.volt_length(jind) > 0)
                if (data.parent_index(jind) ~= 0)
                    if (data.phases(data.parent_index(jind)) == 0)
                        plot([data.volt_length(jind)/5280; data.volt_length(data.parent_index(jind))/5280],[data.pu_a(jind); data.pu_c(data.parent_index(jind))],':.k','MarkerSize',2);
                    else
                        plot([data.volt_length(jind)/5280; data.volt_length(data.parent_index(jind))/5280],[data.pu_a(jind); data.pu_a(data.parent_index(jind))],':.k','MarkerSize',2);
                    end
                else
                    plot(data.volt_length(jind)/5280,data.pu_b(jind),':.k','MarkerSize',2);
                end
            end
        end
    end
    xlim([0 maxX])
    ylim([minY maxY])
    xlabel('Circuit Miles from Substation','fontsize',14);
    ylabel('Voltage Magnitude (120 V_{pu})','fontsize',14);
    title([titlestr 'Voltage Magnitudes'],'fontsize',16);
    box on
    text(0.5,125,datestr(xdate(kind),'HH:MM:SS'),'Color',[0 0 0],'HorizontalAlignment','center','fontsize',16)
    %text(0.5,115,'- Phase A Primary Voltage','Color',[0 0 0],'HorizontalAlignment','center','fontsize',16)
    hold off;
      
    pause(0.001);
end
