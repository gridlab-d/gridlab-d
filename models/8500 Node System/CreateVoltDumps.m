clear;
clc;

fid=fopen('voltdumps.glm','w');

interval = 1; %mins ... integer divisor of 60 (i.e., 4, 15, 30).
number = 29; %total voltdumps; won't work past 59
start_min = 45;
start_hour = 15;

tind = 1;

for jind=1:number
    for kind=1:4:60
        if kind < 10
            sec = ['0' num2str(kind-1)];
        else
            sec = num2str(kind-1);
        end        
        
        fprintf(fid,'object voltdump {\n');
        fprintf(fid,'     filename 8500_volt_%d.csv;\n',tind);
        fprintf(fid,'     name voltdump_%d.csv;\n',tind);

        tind = tind+1;
        
        min = jind + start_min;   

        if (min == 60)
            fprintf(fid,'     runtime ''2000-09-01 %d:0%d:%s'';\n',start_hour+1,0,sec);
        elseif (min > 60)
            min = min-60;
            if (min < 10)
                fprintf(fid,'     runtime ''2000-09-01 %d:0%d:%s'';\n',start_hour+1,min,sec);
            else
                fprintf(fid,'     runtime ''2000-09-01 %d:%d:%s'';\n',start_hour+1,min,sec);
            end
        else
            fprintf(fid,'     runtime ''2000-09-01 %d:%d:%s'';\n',start_hour,min,sec);
        end
        fprintf(fid,'}\n\n');
    end
end

fclose('all');