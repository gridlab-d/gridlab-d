% Write some comments

clear;
clc;

% Load the distance data
load('C:\Users\d3x289\Documents\LAAR\8500NodeTest\8500NodeTest\nodeList2.mat');

% This is setup for videos, but you can do stills too
my_case = 2; %1=single, 2=30 min

if (my_case == 2)
    my_limit = 435;
elseif (my_case == 1)
    my_limit = 1;
end

% Loop through the different voltdump files and parse the data
for jind=1:my_limit
    
    % Open the right voltdump files
    dir = 'C:\Users\d3x289\Documents\LAAR\3.1\VS2005\x64\Release\Base Run 04 20 2015\';
    if (my_case == 2)
        my_file = [dir '8500_volt_' num2str(jind) '.csv'];
    else
        my_file = [dir '8500_schedule_volt_' num2str(jind) '.csv'];
    end
    
    fid = fopen(my_file,'r');
    Header1 = textscan(fid,'%s',8,'Delimiter',',');
    Header2 = textscan(fid,'%s %s %s %s %s %s %s',8,'Delimiter',',');
   
    if (jind == 1)
        temp = textscan(fid,'%s %f %f %f %f %f %f','Delimiter',',');
        data.voltage_names = temp{1};
        data.voltage{jind} = [temp{1,2:7}];
    else
        temp = textscan(fid,'%*s %f %f %f %f %f %f','Delimiter',',');
        data.voltage{jind} = [temp{1,1:6}];
    end
     
    fclose('all'); 
end

clear Header1 Header2 temp

%% Find the nodes in the coordinate system (if they exist)
maxk = length(data.voltage_names);
maxc = length(nodeList.name);  

for kind = 1:maxk
    %TODO: May want to make this a "strfind" - assign coordinates of
    %secondary system to the nearest neighbor
    find_val = strcmpi(nodeList.name,data.voltage_names(kind));
    find_ind = find(find_val == 1);  

    if ( length(find_ind) == 1)
        data.volt_length(kind,1) = nodeList.distance(find_ind);     
                        
        if ( strfind( char(nodeList.phases(find_ind)),'S') ) 
            if ( strfind( char(nodeList.phases(find_ind)),'A') ) 
                data.phases(kind,1) = 1;
            elseif ( strfind( char(nodeList.phases(find_ind)),'B') ) 
                data.phases(kind,1) = 2;    
            elseif ( strfind( char(nodeList.phases(find_ind)),'C') ) 
                data.phases(kind,1) = 3;
            else
                disp('oops.  should not have gotten here');
            end
        else
            data.phases(kind,1) = 0;
        end
        
        parent_ind = nodeList.parent_index(find_ind);
        
        if (parent_ind > 0)
            parent_name = nodeList.name(parent_ind);
            find_val2 = strcmpi(data.voltage_names,parent_name);
            find_ind2 = find(find_val2 == 1);

            if ( length(find_ind2) == 1)
                data.parent_index(kind,1) = find_ind2;
            else
                disp('Well, that didn''t work.  Unless of course I''m the swing node.  So, I guess I should get this message once.');
            end
        else
            data.parent_index(kind,1) = 0;
        end
    else
        data.volt_length(kind,1) = -999999;
    end
    
    % turn to mag and angle
    vm_a = sqrt(data.voltage{1}(kind,1)^2 + data.voltage{1}(kind,2)^2);
    vm_b = sqrt(data.voltage{1}(kind,3)^2 + data.voltage{1}(kind,4)^2);
    vm_c = sqrt(data.voltage{1}(kind,5)^2 + data.voltage{1}(kind,6)^2);

        ta = complex(data.voltage{1}(kind,1),data.voltage{1}(kind,2));
    va_a = 180 / pi * angle(ta);
        tb = complex(data.voltage{1}(kind,3),data.voltage{1}(kind,4));
    va_b = 180 / pi * angle(tb);
        tc = complex(data.voltage{1}(kind,5),data.voltage{1}(kind,6));
    va_c = 180 / pi * angle(tc);

    % figure out the per unit value
    if ( vm_a ~= 0 )
        if ( vm_a < 600 )
            data.volt_pu(kind,1) = 120;
        elseif ( vm_a < 10000 )
            data.volt_pu(kind,1) = 7200;
        elseif ( vm_a < 90000 )
            data.volt_pu(kind,1) = 115000/sqrt(3);
        else
            data.volt_pu(kind,1) = 115000;
        end
    elseif ( vm_b ~= 0 )
        if ( vm_b < 600 )
            data.volt_pu(kind,1) = 120;
        elseif ( vm_b < 10000 )
            data.volt_pu(kind,1) = 7200;
        elseif ( vm_b < 90000 )
            data.volt_pu(kind,1) = 115000/sqrt(3);
        else
            data.volt_pu(kind,1) = 115000;
        end
    elseif ( vm_c ~= 0 )
        if ( vm_c < 600 )
            data.volt_pu(kind,1) = 120;
        elseif ( vm_c < 10000 )
            data.volt_pu(kind,1) = 7200;
        elseif ( vm_c < 90000 )
            data.volt_pu(kind,1) = 115000/sqrt(3);
        else
            data.volt_pu(kind,1) = 115000;
        end
    else
        disp(['error: all voltages ~= 0 for node ' data.voltage_names(kind)]);
    end

    clear find_ind find_val
end

%% Save stuff
disp('done saving');
save('voltage_distance_data.mat','data');