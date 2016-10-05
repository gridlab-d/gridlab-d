clear;
clc;

filename = 'DR0_output.txt';
fid = fopen(['C:\Users\d3x289\Documents\GridLABD\ticket\937\VS2005\x64\Release\' filename]);

temp_data = textscan(fid,'%s','Delimiter','\n');

start_time = 730730.583333333; % 14:00
end_time = 730730.666666667; % 16:00

% Transformers can sustain a significant overload for a long time; 
% many utilities run them up to 200% over-rating for hours at a time as
% long as they have long enough to cool afterwards; lines, however, are
% thermally limited after just a few minutes - they start to droop.  
% I'm recc'ing 200% for transformers and 110% for lines.
perc_trans_overload = 200; % percent overload that equates to a transformer violation
perc_lines_overload = 110; % percent overload that equates to a line violation

%%
[c,d]=size(temp_data{1,1});
my_ind_trans = 1;
my_ind_lines = 1;

for jind=1:c
    temp1 = temp_data{1,1}(jind);
    temp2 = strfind(temp1,'transformer:');
    temp3 = strfind(temp1,'Line:');

    % transformer
    if (~isempty(temp2{1,1}))
        temp4 = strfind(temp1,'rated');

        if (~isempty(temp4{1,1}))
            % check to see if we are in our time window
            [~,rem] = strtok(temp1,'[');
            [tok0,~] = strtok(rem,'[');
            [tok,~] = strtok(tok0,']');
            
            time = datenum(tok,'yyyy-mm-dd HH:MM:SS');
            
            % if we are, then parse out stuff
            if (time >= start_time && time < end_time)
                % parse out the violation and count if >200%
                [tok2,~] = strtok(temp1,'%');
                
                % grab the percentage
                % this seems a little dangerous, but we'll run with it for now
                tok3 = char(tok2);
                tok4 = tok3(end-7:end);
                [~,tok5]=strtok(tok4,' ');
                
                perc = str2double(tok5);
                
                if (perc > perc_trans_overload)
                    [~,rem2] = strtok(temp1,']');
                    [~,rem3] = strtok(rem2,':');
                    [~,rem4] = strtok(rem3,':');
                    [tname,~] = strtok(rem4,' ');
                    tname2 = char(tname);
                    name = tname2(2:end);
                    
                    % check if it is unique or not
                    if (my_ind_trans > 1)
                        found_ind = 0;
                        for kind=1:length(overloads{1,1})
                            if (strcmp(char(overloads{1,1}(kind)),name)==1) 
                                found_ind = kind;
                            end
                        end 
                    else
                        found_ind = 0;
                    end
                       
                    % now count them up
                    if (found_ind == 0)
                        overloads{1,1}(my_ind_trans) = cellstr(name);
                        overloads{1,2}(my_ind_trans) = 1;
                        overloads{1,3}(my_ind_trans) = time;
                        my_ind_trans = my_ind_trans + 1;
                    else
                        if (time == overloads{1,3}(found_ind))
                            % don't count it if we've already seen it at
                            % this time
                        else
                            overloads{1,2}(found_ind) = overloads{1,2}(found_ind) + 1;  
                            overloads{1,3}(found_ind) = time;
                        end
                    end
                end
            end
            
        end
    % line
    elseif (~isempty(temp3{1,1})) 
        temp4 = strfind(temp1,'rating');

        if (~isempty(temp4{1,1}))
            % check to see if we are in our time window
            [~,rem] = strtok(temp1,'[');
            [tok0,~] = strtok(rem,'[');
            [tok,~] = strtok(tok0,']');
            
            time = datenum(tok,'yyyy-mm-dd HH:MM:SS');
            
            % if we are, then parse out stuff
            if (time >= start_time && time < end_time)
                % parse out the violation and count if >100%
                [tok2,~] = strtok(temp1,'%');
                
                % grab the percentage
                % this seems a little dangerous, but we'll run with it for now
                tok3 = char(tok2);
                tok4 = tok3(end-7:end);
                [~,tok5]=strtok(tok4,' ');
                
                perc = str2double(tok5);
                
                if (perc > perc_lines_overload)
                    [~,rem2] = strtok(temp1,']');
                    [~,rem3] = strtok(rem2,':');
                    [~,rem4] = strtok(rem3,':');
                    [lname,~] = strtok(rem4,' ');
                    lname2 = char(lname);
                    name = lname2(2:end);
                    
                    % check if it is unique or not
                    if (my_ind_lines > 1)
                        found_ind = 0;
                        for kind=1:length(overloads{2,1})
                            if (strcmp(char(overloads{2,1}(kind)),name)==1) 
                                found_ind = kind;
                            end
                        end 
                    else
                        found_ind = 0;
                    end
                       
                    % now count them up
                    if (found_ind == 0)
                        overloads{2,1}(my_ind_lines) = cellstr(name);
                        overloads{2,2}(my_ind_lines) = 1;
                        overloads{2,3}(my_ind_lines) = time;
                        my_ind_lines = my_ind_lines + 1;
                    else
                        if (time == overloads{2,3}(found_ind))
                            % don't count it if we've already seen it at
                            % this time
                        else
                            overloads{2,2}(found_ind) = overloads{2,2}(found_ind) + 1;  
                            overloads{2,3}(found_ind) = time;
                        end
                    end
                end
            end
            
        end
    end

end

save('overloads.mat','overloads');

if (my_ind_trans > 1)
    disp('Transformer Overloads (200% overloaded)');
    disp(' Number of time steps with overloads)');
    disp('---------------------------------------');
    disp('Name            | Number of Overloads  ');
    for mind=1:length(overloads{1,1})
        disp([char(overloads{1,1}(mind)) '     | ' num2str(overloads{1,2}(mind)) '                    ']);
    end
else
    disp('No transformer overloads detected.');
end

disp(' ');

if (my_ind_lines > 1) 
    disp('Line Overloads (100% overloaded)');
    disp(' Number of time steps with overloads)');
    disp('---------------------------------------');
    disp('Name            | Number of Overloads  ');
    for mind=1:length(overloads{2,1})
        disp([char(overloads{2,1}(mind)) '     | ' num2str(overloads{2,2}(mind)) '                    ']);
    end
else
    disp('No line overloads detected.');
end
