% Run XML Cleanup first (and have all the neccessary structs in memory
% before using this script
clc;

% For conversion to "p.u." values later
primary_voltage = 7200;
secondary_voltage = 120;


% Initialize the indices
node_ind = 1;
link_ind = 1;

clear nodeList linkList

%% nodes -- all of the things that classify as nodes
%  nodes
[~,max_ind] = size(nodeStruct);

% more than 3 lines indicates at least one object
if (max_ind > 3)
    for ind=1:max_ind
        if strcmp(nodeStruct(ind).Name,'node')
            temp_node = nodeStruct(ind).Children;
            
            % find the right index for each field
            % - Seems overkill to do this for each object, but just in case
            [~, no_fields] = size(temp_node);
            parent_ind = 0;
            for temp_ind = 1:no_fields 
                if ( strcmp(temp_node(temp_ind).Name,'rank'))
                    rank_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'parent'))
                    parent_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'name'))
                    name_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'bustype'))
                    bustype_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'phases'))
                    phase_ind = temp_ind;
                end
            end
            
            nodeList.rank(node_ind,1) = str2double(temp_node(rank_ind).Children.Data);
            % If the swing node, it has no parent
            if (parent_ind ~= 0)
                nodeList.parent{node_ind,:} = temp_node(parent_ind).Children.Data;
            else
                nodeList.parent{node_ind,:} = 'NONE';
            end
            nodeList.name{node_ind,:} = temp_node(name_ind).Children.Data;
            nodeList.bustype{node_ind,:} = temp_node(bustype_ind).Children.Data;
            nodeList.voltage(node_ind,:) = primary_voltage;
            nodeList.phases{node_ind,:} = temp_node(phase_ind).Children.Data;
            
            node_ind = node_ind + 1;
        end
        
        clear temp_node rank_ind parent_ind name_ind bustype_ind
    end
end

%  meters
[~,max_ind] = size(metStruct);

% more than 3 lines indicates at least one object
if (max_ind > 3)
    for ind=1:max_ind
        if strcmp(metStruct(ind).Name,'meter')
            temp_node = metStruct(ind).Children;
            
            % find the right index for each field
            % - Seems overkill to do this for each object, but just in case
            [~, no_fields] = size(temp_node);
            parent_ind = 0;
            for temp_ind = 1:no_fields 
                if ( strcmp(temp_node(temp_ind).Name,'rank'))
                    rank_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'parent'))
                    parent_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'name'))
                    name_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'bustype'))
                    bustype_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'phases'))
                    phase_ind = temp_ind;
                end
            end
            
            nodeList.rank(node_ind,1) = str2double(temp_node(rank_ind).Children.Data);
            % If the swing node, it has no parent
            if (parent_ind ~= 0)
                nodeList.parent{node_ind,:} = temp_node(parent_ind).Children.Data;
            else
                nodeList.parent{node_ind,:} = 'NONE';
            end
            nodeList.name{node_ind,:} = temp_node(name_ind).Children.Data;
            nodeList.bustype{node_ind,:} = temp_node(bustype_ind).Children.Data;
            nodeList.voltage(node_ind,:) = primary_voltage;
            nodeList.phases{node_ind,:} = temp_node(phase_ind).Children.Data;
            
            node_ind = node_ind + 1;
        end
        
        clear temp_node
    end
end

%  triplex_meters
[~,max_ind] = size(tpmStruct);

% more than 3 lines indicates at least one object
if (max_ind > 3)
    for ind=1:max_ind
        if strcmp(tpmStruct(ind).Name,'triplex_meter')
            temp_node = tpmStruct(ind).Children;
            
           % find the right index for each field
            % - Seems overkill to do this for each object, but just in case
            [~, no_fields] = size(temp_node);
            parent_ind = 0;
            for temp_ind = 1:no_fields 
                if ( strcmp(temp_node(temp_ind).Name,'rank'))
                    rank_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'parent'))
                    parent_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'name'))
                    name_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'bustype'))
                    bustype_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'phases'))
                    phase_ind = temp_ind;
                end
            end
            
            nodeList.rank(node_ind,1) = str2double(temp_node(rank_ind).Children.Data);
            % If the swing node, it has no parent
            if (parent_ind ~= 0)
                nodeList.parent{node_ind,:} = temp_node(parent_ind).Children.Data;
            else
                nodeList.parent{node_ind,:} = 'NONE';
            end
            nodeList.name{node_ind,:} = temp_node(name_ind).Children.Data;
            nodeList.bustype{node_ind,:} = temp_node(bustype_ind).Children.Data;
            nodeList.voltage(node_ind,:) = secondary_voltage;
            nodeList.phases{node_ind,:} = temp_node(phase_ind).Children.Data;
            
            node_ind = node_ind + 1;
        end
        
        clear temp_node
    end
end

%  triplex_node
[~,max_ind] = size(tpnStruct);

% more than 3 lines indicates at least one object
if (max_ind > 3)
    for ind=1:max_ind
        if strcmp(tpnStruct(ind).Name,'triplex_node')
            temp_node = tpnStruct(ind).Children;
            
            % find the right index for each field
            % - Seems overkill to do this for each object, but just in case
            [~, no_fields] = size(temp_node);
            parent_ind = 0;
            for temp_ind = 1:no_fields 
                if ( strcmp(temp_node(temp_ind).Name,'rank'))
                    rank_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'parent'))
                    parent_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'name'))
                    name_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'bustype'))
                    bustype_ind = temp_ind;
                elseif ( strcmp(temp_node(temp_ind).Name,'phases'))
                    phase_ind = temp_ind;
                end
            end
            
            nodeList.rank(node_ind,1) = str2double(temp_node(rank_ind).Children.Data);
            % If the swing node, it has no parent
            if (parent_ind ~= 0)
                nodeList.parent{node_ind,:} = temp_node(parent_ind).Children.Data;
            else
                nodeList.parent{node_ind,:} = 'NONE';
            end
            nodeList.name{node_ind,:} = temp_node(name_ind).Children.Data;
            nodeList.bustype{node_ind,:} = temp_node(bustype_ind).Children.Data;
            nodeList.voltage(node_ind,:) = secondary_voltage;
            nodeList.phases{node_ind,:} = temp_node(phase_ind).Children.Data;
            
            node_ind = node_ind + 1;
        end
        
        clear temp_node
    end
end

save('nodeList.mat','nodeList');

%% links -- all of the things that classify as links
%  Overhead lines
[~,max_ind] = size(ohlStruct);

% more than 3 lines indicates at least one object
if (max_ind > 3)
    for ind=1:max_ind
        if strcmp(ohlStruct(ind).Name,'overhead_line')
            temp_link = ohlStruct(ind).Children;
            
            % find the right index for each field
            % - Seems overkill to do this for each object, but just in case
            [~, no_fields] = size(temp_link);
            parent_ind = 0;
            for temp_ind = 1:no_fields 
                if ( strcmp(temp_link(temp_ind).Name,'rank'))
                    rank_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'parent'))
                    parent_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'name'))
                    name_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'length'))
                    length_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'from'))
                    from_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'to'))
                    to_ind = temp_ind;
                end
            end
            % This might be too hard-coded; might need to find these
            % indices rather than assuming them
            linkList.rank(link_ind,1) = str2double(temp_link(rank_ind).Children.Data);
            linkList.parent{link_ind,:} = temp_link(parent_ind).Children.Data;
            linkList.name{link_ind,:} = temp_link(name_ind).Children.Data;
            
                % Assume they are all in ft, for now
                [temp_length,~] = strtok(temp_link(length_ind).Children.Data,' ');            
            linkList.length(link_ind,1) = str2double(temp_length);
            linkList.from{link_ind,:} = temp_link(from_ind).Children.Data;
            linkList.to{link_ind,:} = temp_link(to_ind).Children.Data;
            
            link_ind = link_ind + 1;
        end
        
        clear temp_link temp1 temp_length
    end
end

%  Underground lines
[~,max_ind] = size(uglStruct);

% more than 3 lines indicates at least one object
if (max_ind > 3)
    for ind=1:max_ind
        if strcmp(uglStruct(ind).Name,'underground_line')
            temp_link = uglStruct(ind).Children;
            
            % find the right index for each field
            % - Seems overkill to do this for each object, but just in case
            [~, no_fields] = size(temp_link);
            parent_ind = 0;
            for temp_ind = 1:no_fields 
                if ( strcmp(temp_link(temp_ind).Name,'rank'))
                    rank_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'parent'))
                    parent_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'name'))
                    name_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'length'))
                    length_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'from'))
                    from_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'to'))
                    to_ind = temp_ind;
                end
            end
            % This might be too hard-coded; might need to find these
            % indices rather than assuming them
            linkList.rank(link_ind,1) = str2double(temp_link(rank_ind).Children.Data);
            linkList.parent{link_ind,:} = temp_link(parent_ind).Children.Data;
            linkList.name{link_ind,:} = temp_link(name_ind).Children.Data;
            
                % Assume they are all in ft, for now
                [temp_length,~] = strtok(temp_link(length_ind).Children.Data,' ');            
            linkList.length(link_ind,1) = str2double(temp_length);
            linkList.from{link_ind,:} = temp_link(from_ind).Children.Data;
            linkList.to{link_ind,:} = temp_link(to_ind).Children.Data;
            
            link_ind = link_ind + 1;
        end
        
        clear temp_link temp1 temp_length
    end
end

%  Transformers
[~,max_ind] = size(tfmrStruct);

% more than 3 lines indicates at least one object
if (max_ind > 3)
    for ind=1:max_ind
        if strcmp(tfmrStruct(ind).Name,'transformer')
            temp_link = tfmrStruct(ind).Children;
            
           % find the right index for each field
            % - Seems overkill to do this for each object, but just in case
            [~, no_fields] = size(temp_link);
            parent_ind = 0;
            for temp_ind = 1:no_fields 
                if ( strcmp(temp_link(temp_ind).Name,'rank'))
                    rank_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'parent'))
                    parent_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'name'))
                    name_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'from'))
                    from_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'to'))
                    to_ind = temp_ind;
                end
            end
            % This might be too hard-coded; might need to find these
            % indices rather than assuming them
            linkList.rank(link_ind,1) = str2double(temp_link(rank_ind).Children.Data);
            linkList.parent{link_ind,:} = temp_link(parent_ind).Children.Data;
            linkList.name{link_ind,:} = temp_link(name_ind).Children.Data;          
            linkList.length(link_ind,1) = 0;
            linkList.from{link_ind,:} = temp_link(from_ind).Children.Data;
            linkList.to{link_ind,:} = temp_link(to_ind).Children.Data;
            
            link_ind = link_ind + 1;
        end
        
        clear temp_link temp1 temp_length
    end
end

%  Triplex Lines
[~,max_ind] = size(tplStruct);

% more than 3 lines indicates at least one object
if (max_ind > 3)
    for ind=1:max_ind
        if strcmp(tplStruct(ind).Name,'triplex_line')
            temp_link = tplStruct(ind).Children;
            
           % find the right index for each field
            % - Seems overkill to do this for each object, but just in case
            [~, no_fields] = size(temp_link);
            parent_ind = 0;
            for temp_ind = 1:no_fields 
                if ( strcmp(temp_link(temp_ind).Name,'rank'))
                    rank_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'parent'))
                    parent_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'name'))
                    name_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'length'))
                    length_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'from'))
                    from_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'to'))
                    to_ind = temp_ind;
                end
            end
            % This might be too hard-coded; might need to find these
            % indices rather than assuming them
            linkList.rank(link_ind,1) = str2double(temp_link(rank_ind).Children.Data);
            linkList.parent{link_ind,:} = temp_link(parent_ind).Children.Data;
            linkList.name{link_ind,:} = temp_link(name_ind).Children.Data;
            
                % Assume they are all in ft, for now
                [temp_length,~] = strtok(temp_link(length_ind).Children.Data,' ');            
            linkList.length(link_ind,1) = str2double(temp_length);
            linkList.from{link_ind,:} = temp_link(from_ind).Children.Data;
            linkList.to{link_ind,:} = temp_link(to_ind).Children.Data;
            
            link_ind = link_ind + 1;
        end
        
        clear temp_link temp1 temp_length
    end
end

%  Switches
[~,max_ind] = size(swStruct);

% more than 3 lines indicates at least one object
if (max_ind > 3)
    for ind=1:max_ind
        if strcmp(swStruct(ind).Name,'switch')
            temp_link = swStruct(ind).Children;
            
            % find the right index for each field
            % - Seems overkill to do this for each object, but just in case
            [~, no_fields] = size(temp_link);
            parent_ind = 0;
            for temp_ind = 1:no_fields 
                if ( strcmp(temp_link(temp_ind).Name,'rank'))
                    rank_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'parent'))
                    parent_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'name'))
                    name_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'from'))
                    from_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'to'))
                    to_ind = temp_ind;
                end
            end
            % This might be too hard-coded; might need to find these
            % indices rather than assuming them
            linkList.rank(link_ind,1) = str2double(temp_link(rank_ind).Children.Data);
            linkList.parent{link_ind,:} = temp_link(parent_ind).Children.Data;
            linkList.name{link_ind,:} = temp_link(name_ind).Children.Data;           
            linkList.length(link_ind,1) = 0;
            linkList.from{link_ind,:} = temp_link(from_ind).Children.Data;
            linkList.to{link_ind,:} = temp_link(to_ind).Children.Data;
            
            link_ind = link_ind + 1;
        end
        
        clear temp_link temp1 temp_length
    end
end

%  Regulators
[~,max_ind] = size(regStruct);

% more than 3 lines indicates at least one object
if (max_ind > 3)
    for ind=1:max_ind
        if strcmp(regStruct(ind).Name,'regulator')
            temp_link = regStruct(ind).Children;
            
            % find the right index for each field
            % - Seems overkill to do this for each object, but just in case
            [~, no_fields] = size(temp_link);
            parent_ind = 0;
            for temp_ind = 1:no_fields 
                if ( strcmp(temp_link(temp_ind).Name,'rank'))
                    rank_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'parent'))
                    parent_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'name'))
                    name_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'from'))
                    from_ind = temp_ind;
                elseif ( strcmp(temp_link(temp_ind).Name,'to'))
                    to_ind = temp_ind;
                end
            end
            % This might be too hard-coded; might need to find these
            % indices rather than assuming them
            linkList.rank(link_ind,1) = str2double(temp_link(rank_ind).Children.Data);
            linkList.parent{link_ind,:} = temp_link(parent_ind).Children.Data;
            linkList.name{link_ind,:} = temp_link(name_ind).Children.Data;           
            linkList.length(link_ind,1) = 0;
            linkList.from{link_ind,:} = temp_link(from_ind).Children.Data;
            linkList.to{link_ind,:} = temp_link(to_ind).Children.Data;
            
            link_ind = link_ind + 1;
        end
        
        clear temp_link temp1 temp_length
    end
end

save('linkList.mat','linkList');