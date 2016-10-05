%----- This file reads the data files for the LAAR project and saves them as a .mat file for ease of use 
clear
clc

%----------------------------------------------------------- 4 second results -------------------------------------------------------------------
%------------------------------------------------------------------------------------------------------------------------------------------------
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_14_voltagelock_0_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_14_voltagelock_1_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_14_voltagelock_0_voltagesort_1\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_14_voltagelock_1_voltagesort_1\';

%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_10_voltagelock_0_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_10_voltagelock_1_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_10_voltagelock_0_voltagesort_1\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_paloverde_droop_10_voltagelock_1_voltagesort_1\';

%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_valley_droop_2_5_voltagelock_0_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_valley_droop_2_5_voltagelock_1_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_valley_droop_2_5_voltagelock_0_voltagesort_1\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\4_second_results\freq_valley_droop_2_5_voltagelock_1_voltagesort_1\';

%----------------------------------------------------------- 1 second results -------------------------------------------------------------------
%------------------------------------------------------------------------------------------------------------------------------------------------
gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_paloverde_droop_10_voltagelock_0_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_paloverde_droop_10_voltagelock_1_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_paloverde_droop_10_voltagelock_0_voltagesort_1\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_paloverde_droop_10_voltagelock_0_voltagesort_1B\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_paloverde_droop_10_voltagelock_1_voltagesort_1\';

%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_valley_droop_2_5_voltagelock_0_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_valley_droop_2_5_voltagelock_1_voltagesort_0\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_valley_droop_2_5_voltagelock_0_voltagesort_1\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_valley_droop_2_5_voltagelock_0_voltagesort_1B\';
%gld_path = 'C:\Users\hans464\Desktop\LAAR\FY16\8500_node_system\results\1_second_results\freq_valley_droop_2_5_voltagelock_1_voltagesort_1\';


read_overloads = 1;
start_time = 730730.583333333; % 14:00
end_time = 730730.666666667; % 16:00
%start_time = 734017.1875; % 04:30
%end_time = 734017.2708333334; % 06:30

% Transformers can sustain a significant overload for a long time; 
% many utilities run them up to 200% over-rating for hours at a time as
% long as they have long enough to cool afterwards; lines, however, are
% thermally limited after just a few minutes - they start to droop.  
% I'm recc'ing 200% for transformers and 110% for lines.
perc_trans_overload = 200; % percent overload that equates to a transformer violation
perc_lines_overload = 110; % percent overload that equates to a line violation


%Look into the data folder and get the file structure
DataFilesInFolder_DR0=dir([gld_path '*_DR0.csv']);
DataFilesInFolder_DR2=dir([gld_path '*_DR2.csv']);
DataFilesInFolder_DR3=dir([gld_path '*_DR3.csv']);

%Get a count
DataFileCount(1)=length(DataFilesInFolder_DR0);
DataFileCount(3)=length(DataFilesInFolder_DR2);
DataFileCount(4)=length(DataFilesInFolder_DR3);

display('Reading data, give me a minute!')
for jfile = [0 2 3]
    for ifile =1:DataFileCount(jfile+1)
        do_nothing = 0;
        filename = eval(['DataFilesInFolder_DR',num2str(jfile),'(ifile).name']);
        fieldname = filename(1:end-8);
        disp(['parsing recorder ' filename ' from folder ' gld_path])    
        
        fid = fopen([gld_path,filename]);
        % read in and discard the header and read in the column names (not always same number of lines, depends on platform and GLD settings)
        A = fgetl(fid); headerlines = 0;
        while strfind(A,'#')==1 % while loop to read and discard header
            Ap = A; A = fgetl(fid); headerlines = headerlines+1;
        end % the second-to-last line that is read is the line of column headings
        numcolumns = length(strfind(Ap,','))+1; % there is one more column than there are delimiters
        
        % build up the format string by adding a %s for each column
        format = repmat('%s',1,numcolumns);
        headings = textscan(Ap,format,'Delimiter',',');
        if isempty(headings{numcolumns}), headings = headings(1:end-1); numcolumns = numcolumns-1; format = format(1:end-2); end
        properties = cell(length(headings)-1,1);
        for i = 1:length(headings)-1, properties(i) = headings{i+1}; end % stupid stupid textscan 
        properties = strrep(properties,'.','_');
        properties = strrep(properties,':','_');
        properties = strrep(properties,'(','_');
        properties = strrep(properties,')','_');
        properties = strrep(properties,'-','_');
        
        
        frewind(fid); % go back to the beginning of the file to get ready to read in the data
        
        % build up the format string by adding a %s for each column
        if strncmp(filename,'whc_pfc_state',13) || strncmp(filename,'capacitor_state',15)
            format = ['%s ',repmat('%s',1,numcolumns-1)];
            temp = textscan(fid,format,'Delimiter',',','HeaderLines',headerlines,'CollectOutput',1);
            temp_data = [temp{:}];
            time = datenum(cellfun(@(x) x(1:end-4),temp_data(:,1),'UniformOutput',false),'yyyy-mm-dd HH:MM:SS');
            temp_data = temp_data(:,2:end);
        elseif strncmp(filename,'Violation',9) || strncmp(filename,'wh_override',11) || strncmp(filename,'wh_current_model',16)
            do_nothing = 1;
        else
            format = ['%s',repmat('%f',1,numcolumns-1)];   
            temp = textscan(fid,format,'Delimiter',',','HeaderLines',headerlines,'CollectOutput',1);
            if strncmp(filename,'meter_voltage',13)
                temp_data = abs([temp{2}]);
            else
                temp_data = [temp{2}];
            end
            time = datenum(cellfun(@(x) x(1:end-4),[temp{1}],'UniformOutput',false),'yyyy-mm-dd HH:MM:SS'); % convert to matlab format
        end
       
        if strncmp(filename,'whc_pfc_state',13)
            eval(['data.DR',num2str(jfile),'.FREE = strcmp(temp_data,''FREE'');']);
            eval(['data.DR',num2str(jfile),'.TRIGGERED_OFF = strcmp(temp_data,''TRIGGERED_OFF'');']);
            eval(['data.DR',num2str(jfile),'.TRIGGERED_ON = strcmp(temp_data,''TRIGGERED_ON'');']);
            eval(['data.DR',num2str(jfile),'.FORCED_OFF = strcmp(temp_data,''FORCED_OFF'');']);
            eval(['data.DR',num2str(jfile),'.FORCED_ON = strcmp(temp_data,''FORCED_ON'');']);
            eval(['data.DR',num2str(jfile),'.RELEASED_OFF = strcmp(temp_data,''RELEASED_OFF'');']);
            eval(['data.DR',num2str(jfile),'.RELEASED_ON = strcmp(temp_data,''RELEASED_ON'');']); 
            %eval(['data.DR',num2str(jfile),'.PFC_string = properties;']);
            eval(['data.DR',num2str(jfile),'.([fieldname,''_string'']) = properties;']);
            eval(['data.DR',num2str(jfile),'.([fieldname,''_timestamps'']) = time;']);
        elseif strncmp(filename,'capacitor_state',15)
            eval(['data.DR',num2str(jfile),'.(fieldname) = strcmp(temp_data,''CLOSED'');']); 
            %eval(['data.DR',num2str(jfile),'.CAP_STATE_string = properties;']);
            eval(['data.DR',num2str(jfile),'.([fieldname,''_string'']) = properties;']);
            eval(['data.DR',num2str(jfile),'.([fieldname,''_timestamps'']) = time;']);
        elseif do_nothing == 0
            eval(['data.DR',num2str(jfile),'.(fieldname) = temp_data;']);
            eval(['data.DR',num2str(jfile),'.([fieldname,''_string'']) = properties;']);
            eval(['data.DR',num2str(jfile),'.([fieldname,''_timestamps'']) = time;']);
        else
            %do nothing
        end
         
        % close the file
        fclose(fid);          
    end
end


if read_overloads == 1
    filename = {'DR0_output.txt';'DR0_output.txt';'DR0_output.txt'};
    p=1;
    for ioverload = [0 2 3]
        disp(['parsing overloads for DR',num2str(ioverload)])
        fid = fopen([gld_path,char(filename(p,:))]);
        p=p+1;
        temp_data = textscan(fid,'%s','Delimiter','\n');

        [c,d]=size(temp_data{1,1});
        temp_data2 = temp_data{:};

        index = cellfun(@(x) ~isempty(x), strfind(temp_data2,'% of its rated power value'), 'UniformOutput', 1);
        transformer_data = temp_data2(index);
        transformer_perc = cellfun(@(x) logical(str2double(x(end-31:end-26)) > perc_trans_overload), transformer_data, 'UniformOutput', 1);
        transformer_data = transformer_data(transformer_perc);
        transformer_times = cellfun(@(x) x(11:29), transformer_data, 'UniformOutput',false);
        transformer_times = datenum(transformer_times,'yyyy-mm-dd HH:MM:SS');
        index = logical(transformer_times >= start_time & transformer_times < end_time);
        transformer_data = transformer_data(index);
        transformer_times = transformer_times(index);
        transformer_names = cellfun(@(x) x(strfind(x,'transformer:')+12:strfind(x,' is at')-1), transformer_data, 'UniformOutput', false);

        index = cellfun(@(x) ~isempty(x), strfind(temp_data2,'% of its emergency rating on phase'), 'UniformOutput', 1);
        line_data = temp_data2(index);
        line_perc = cellfun(@(x) logical(str2double(x(end-42:end-37)) > perc_lines_overload), line_data, 'UniformOutput', 1);
        line_data = line_data(line_perc);
        line_times = cellfun(@(x) x(11:29), line_data, 'UniformOutput',false);
        line_times = datenum(line_times,'yyyy-mm-dd HH:MM:SS');
        index = logical(line_times >= start_time & line_times < end_time);
        line_data = line_data(index);
        line_times = line_times(index);
        line_names = cellfun(@(x) x(strfind(x,'Line:')+5:strfind(x,' is at')-1), line_data, 'UniformOutput', false);

        my_ind_trans = 1;
        my_ind_lines = 1;
        overloads = {};
        for i = 1 : length(transformer_names)
            if (my_ind_trans > 1)
                found_ind = 0;
                for kind=1:length(overloads{1,1})
                    if (strcmp(char(overloads{1,1}(kind)),transformer_names(i))==1) 
                        found_ind = kind;
                    end
                end 
            else
                found_ind = 0;
            end

            % now count them up
            if (found_ind == 0)
                overloads{1,1}(my_ind_trans) = cellstr(transformer_names(i));
                overloads{1,2}(my_ind_trans) = 1;
                overloads{1,3}(my_ind_trans) = transformer_times(i);
                my_ind_trans = my_ind_trans + 1;
            else
                if (transformer_times(i) == overloads{1,3}(found_ind))
                    % don't count it if we've already seen it at
                    % this time
                else
                    overloads{1,2}(found_ind) = overloads{1,2}(found_ind) + 1;  
                    overloads{1,3}(found_ind) = transformer_times(i);
                end
            end
        end
        for i = 1 : length(line_names)
            if (my_ind_lines > 1)
                found_ind = 0;
                for kind=1:length(overloads{2,1})
                    if (strcmp(char(overloads{2,1}(kind)),line_names(i))==1) 
                        found_ind = kind;
                    end
                end 
            else
                found_ind = 0;
            end

            % now count them up
            if (found_ind == 0)
                overloads{2,1}(my_ind_lines) = cellstr(line_names(i));
                overloads{2,2}(my_ind_lines) = 1;
                overloads{2,3}(my_ind_lines) = line_times(i);
                my_ind_lines = my_ind_lines + 1;
            else
                if (line_times(i) == overloads{2,3}(found_ind))
                    % don't count it if we've already seen it at
                    % this time
                else
                    overloads{2,2}(found_ind) = overloads{2,2}(found_ind) + 1;  
                    overloads{2,3}(found_ind) = line_times(i);
                end
            end
        end
        eval(['data.DR',num2str(ioverload),'.overloads = overloads;']);   
        fclose(fid);
    end
end
    
display('Saving data, give me a minute!')
save([gld_path,'timeseries_data.mat'],'data','-v7.3'); 