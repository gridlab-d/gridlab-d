function make_plots(objname)

s = warning('off','MATLAB:table:ModifiedVarnames');
data = readtable([objname,'.csv'],'HeaderLines',8);
warning(s);
data.Properties.VariableNames{1} = 'timestamp';
%data.timestamp = (datenum(data.timestamp,'YYYY-MM-DD hh:mm:ss')-datenum(2000,1,1,0,0,0))/3600;
t = (1:length(data.timestamp))*4/3600;
for n = 2:length(data.Properties.VariableNames)
    figure(n); clf; set(gcf,'windowstyle','docked','color','white');
    if prod(isnumeric(data{:,n}))
        x = data{:,n};
    else
        x = nan(length(t),1);
        for m = 1:length(t)
            if iscell(data{m,n})
                x(m) = sscanf(char(data{m,n}),'%g');
            elseif ischar(data{m,n})
                x(m) = sscanf(data{m,n},'%g');
            elseif isnumeric(data{m,n})
                x(m) = data{m,n};
            end
        end
    end
    if max(t)<2/60
        plot(t*3600,x,'+-');
        xlabel('Time [s]');
        set(gca,'xtick',0:4:max(t*3600));
    elseif max(t)<2
        plot(t*60,x,'-');
        xlabel('Time [m]');
    else
        plot(t,x,'-');
        xlabel('Time [h]');
    end
    vn = data.Properties.VariableNames{n};
    fn = regexprep(vn,'_(\w*)_$','');
    vn = [upper(vn(1)),vn(2:end)];
    vn = regexprep(vn,'_(\w*)_$',' [$1]');
    ylabel(vn);
    grid on;
    set(gcf,'name',vn,'numbertitle','off');
    print('-r300','-dpng',[objname,'_',fn,'.png']);
end
