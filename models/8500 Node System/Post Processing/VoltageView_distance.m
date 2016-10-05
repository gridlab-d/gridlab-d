clear;
clc;

dir = '';%'C:\Users\d3x289\Documents\LAAR\3.1\VS2005\x64\Release\DR Run 01 14 2015\';

load([dir 'voltage_data.mat']);

%%
% -------------------------------------------------------------
% now pull out only voltages which found a coordinate match and p.u.ize
% -------------------------------------------------------------

% Get rid of ones that don't have an X/Y coordinate
coord_ind = find(data.voltX ~= -999999);

% Adjust the coordinates to fit nicely on the screen
volt.X = data.voltX(coord_ind) - min(data.voltX(coord_ind));
volt.Y = data.voltY(coord_ind) - min(data.voltY(coord_ind));
volt.X = volt.X/5280;
volt.Y = volt.Y/5280;

reg2_X = (1672032.10695868 - min(data.voltX(coord_ind)))/5280;  
reg2_Y = (12283195.7856474 - min(data.voltY(coord_ind)))/5280;
reg3_X = (1680565.27099862 - min(data.voltX(coord_ind)))/5280;
reg3_Y = (12283007.2099032 - min(data.voltY(coord_ind)))/5280;
reg4_X = (1676925.80296323 - min(data.voltX(coord_ind)))/5280;
reg4_Y = (12268910.6415701 - min(data.voltY(coord_ind)))/5280;
cap0_X = (1673069.33294364 - min(data.voltX(coord_ind)))/5280;  
cap0_Y = (12284208.7264483 - min(data.voltY(coord_ind)))/5280;
cap1_X = (1687041.43612465 - min(data.voltX(coord_ind)))/5280;  
cap1_Y = (12275413.3733521 - min(data.voltY(coord_ind)))/5280;
cap2_X = (1693275.03613046 - min(data.voltX(coord_ind)))/5280; 
cap2_Y = (12276265.3741209 - min(data.voltY(coord_ind)))/5280;
cap3_X = (1672458.68078307 - min(data.voltX(coord_ind)))/5280; 
cap3_Y = (12265257.6485427 - min(data.voltY(coord_ind)))/5280;

test_X = (1680133.60994222 - min(data.voltX(coord_ind)))/5280; 
test_Y = (12266654.319008 - min(data.voltY(coord_ind)))/5280;

maxX = max(volt.X);
maxY = max(volt.Y);
minX = min(volt.X);
minY = min(volt.Y);

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
for kind=1:435
    % Extract the pu values...but separate out the zero values
    volt.pu_a = sqrt(data.voltage{kind}(coord_ind,1).^2 + data.voltage{kind}(coord_ind,2).^2) ./ data.volt_pu(coord_ind);
        mag_ind_a = find(volt.pu_a ~= 0);
    volt.pu_b = sqrt(data.voltage{kind}(coord_ind,3).^2 + data.voltage{kind}(coord_ind,4).^2) ./ data.volt_pu(coord_ind);
        mag_ind_b = find(volt.pu_b ~= 0);
    volt.pu_c = sqrt(data.voltage{kind}(coord_ind,5).^2 + data.voltage{kind}(coord_ind,6).^2) ./ data.volt_pu(coord_ind);
        mag_ind_c = find(volt.pu_c ~= 0);

    for jind=1:length(volt.pu_a)
        %TODO: make this more complex - maybe furthest deviation from 1.00?
        volt.pu(jind,1) = max([volt.pu_a(jind),volt.pu_b(jind),volt.pu_c(jind)]);    
    end
        mag_ind = find(volt.pu ~= 0);

    minV = min([volt.pu_a(mag_ind_a);volt.pu_b(mag_ind_b);volt.pu_c(mag_ind_c);0.9]);
    maxV = max([volt.pu_a(mag_ind_a);volt.pu_b(mag_ind_b);volt.pu_c(mag_ind_c);1.1]);


    % Start plotting
    titlestr = 'Base Case: ';

    % hfig = figure(100);
    % clf(100);
    % set(hfig,'Position',[300 100 900 800]);
    % set(hfig,'color','white')
    % % subplot(1,3,1)
    % hold on;
    % colormap(jet);
    % hcbar = colorbar('location','eastoutside');
    % set(get(hcbar,'ylabel'),'string','Voltage in per unit','fontsize',12)
    % scatter([volt.X(mag_ind); minX; maxX],[volt.Y(mag_ind); minY; maxY],6,[volt.pu(mag_ind); minV; maxV]); %,[volt1_pu(timestep,:) minV maxV]
    % xlim([0 maxX])
    % ylim([0 maxY])
    % title([titlestr 'Maximum Voltage Magnitude'],'fontsize',16);
    % box on
    % set(gca,'XTick',[])
    % set(gca,'YTick',[])
    % plot([0.5 1.5],[0.5 0.5],'k+-')
    % text(1.0,0.4,'1 mile','HorizontalAlignment','center','fontsize',12)
    % hold off;
    %text(inv1.X,inv1.Y,'x','Color',[0 1 0],'HorizontalAlignment','center')

    hfig = figure(1001);
    clf(1001);
    set(hfig,'Position',[300 100 900 800]);
    set(hfig,'color','white')
    % subplot(1,3,1)
    hold on;
    colormap(jet);
    hcbar = colorbar('location','eastoutside');
    set(get(hcbar,'ylabel'),'string','Voltage in per unit','fontsize',12)
    scatter([volt.X(mag_ind_a); minX; maxX],[volt.Y(mag_ind_a); minY; maxY],6,[volt.pu_a(mag_ind_a); minV; maxV]);
    scatter([volt.X(mag_ind_b); minX; maxX],[volt.Y(mag_ind_b); minY; maxY],10,[volt.pu_b(mag_ind_b); minV; maxV],'+'); 
    scatter([volt.X(mag_ind_c); minX; maxX],[volt.Y(mag_ind_c); minY; maxY],8,[volt.pu_c(mag_ind_c); minV; maxV],'x'); 
    scatter([reg2_X reg3_X reg4_X],[reg2_Y reg3_Y reg4_Y],12,'s','MarkerEdgeColor','k','MarkerFaceColor','k');
    scatter([cap0_X cap1_X cap2_X cap3_X],[cap0_Y cap1_Y cap2_Y cap3_Y],18,'d','MarkerEdgeColor','k','MarkerFaceColor','k');
    scatter(test_X,test_Y,24,'d','MarkerEdgeColor','k','MarkerFaceColor','r')
    xlim([0 maxX])
    ylim([0 maxY])
    title([titlestr 'All Voltage Magnitudes'],'fontsize',16);
    box on
    set(gca,'XTick',[])
    set(gca,'YTick',[])
    plot([0.5 1.5],[0.5 0.5],'k+-')
    text(1.0,0.4,'1 mile','HorizontalAlignment','center','fontsize',12)
    text(6.0,0.4,datestr(xdate(kind),'HH:MM:SS'),'Color',[0 0 0],'HorizontalAlignment','center','fontsize',16)
    hold off;
      
    
    pause(0.001);
end


% clear movieframes
% figure(100);
% hfig = gcf;
% set(hfig,'Position',[100 400 1200 400]);
% set(hfig,'color','white')
% 
%     subplot(1,3,1)
%     set(gca,'NextPlot','replaceChildren');
%     colormap(cool);
%     colorbar('location','southoutside');
% 
%     subplot(1,3,2)
%     set(gca,'NextPlot','replaceChildren');
%     colormap(cool);
%     colorbar('location','southoutside');
% 
%     subplot(1,3,3)
%     set(gca,'NextPlot','replaceChildren');
%     colormap(cool);
%     colorbar('location','southoutside');
% 
% for fframe = 1:20;
%     timemultiplier = 5;
%     
%     subplot(1,3,1)
% %     set(gca,'NextPlot','replaceChildren');
%     hold on;
%     scatter([volt.X(mag1_ind),minX,maxX],[volt.Y(mag1_ind),minY,maxY],3,[volt1_pu(fframe,:),minV,maxV]);
%     title({'Phase A |V| Map', datestr(timestamps(fframe*timemultiplier),'HH:MM:SS')});
%     hold off;
% 
%     subplot(1,3,2)
% %     set(gca,'NextPlot','replaceChildren');
%     hold on;
%     scatter([volt.X(mag2_ind),minX,maxX],[volt.Y(mag2_ind),minY,maxY],3,[volt2_pu(fframe,:),minV,maxV]);
%     title({'Phase B |V| Map', datestr(timestamps(fframe*timemultiplier),'HH:MM:SS')});
%     hold off;
% 
%     subplot(1,3,3)
% %     set(gca,'NextPlot','replaceChildren');
%     hold on;
%     scatter([volt.X(mag3_ind),minX,maxX],[volt.Y(mag3_ind),minY,maxY],3,[volt3_pu(fframe,:),minV,maxV]);
%     title({'Phase C |V| Map', datestr(timestamps(fframe*timemultiplier),'HH:MM:SS')});
%     hold off;
% 
%     movieframes(fframe)=getframe(hfig);
%     
% end
% 
% 
% movie(hfig,movieframes,10,3,[0 0 0 0])