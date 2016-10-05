clear;
clc;

load('linkList.mat');
load('nodeList.mat');

%% Walk through the ranking structure to get the 
% Identify the highest rank 
high_rank = max(nodeList.rank);
rank_ind = find(nodeList.rank == high_rank);
nodeList.distance(rank_ind,1) = 0;

% Now loop through all of the ranks and build a tree of distances
for jind=1:high_rank
    % But we want to loop from highest to lowest
    ind = high_rank - jind;
    
    rank_ind = find(linkList.rank == ind);
    
    if (ind == 218)
        disp('pause');
    end
    if (~isempty(rank_ind))
        for kind=1:length(rank_ind)
            my_parent = linkList.parent(rank_ind(kind));
            my_to = linkList.to(rank_ind(kind));
            my_from = linkList.from(rank_ind(kind));
            
            %TODO - will this mess up with node1 vs. node12?
            parent_ind = find(ismember(nodeList.name,my_parent));
            if (isempty(parent_ind))
                disp('Umm, could not find the parent. Now what?');
            end
            if (length(parent_ind) > 1)
                disp('Umm, try again. Found more than one parent.');
            end
            
            % Assume to and from were set up correctly
            if ( strcmp(my_from,my_parent) )
                to_ind = find(ismember(nodeList.name,my_to));
                
                nodeList.distance(to_ind,1) = nodeList.distance(parent_ind) + linkList.length(rank_ind(kind));
                nodeList.parent_index(to_ind,1) = parent_ind;
            % If they were set up backwards, 'cuz NR doesn't care
            elseif ( strcmp(my_to,my_parent) )
                from_ind = find(ismember(nodeList.name,my_from));
                
                nodeList.distance(from_ind,1) = nodeList.distance(parent_ind) + linkList.length(rank_ind(kind));
                nodeList.parent_index(from_ind,1) = parent_ind;
            else
                disp('Not sure how we could have a parent of link be neither the ''to'' or ''from'' node unless we ran it in NR.');
            end
            
            clear parent_ind to_ind from_ind my_parent my_to my_from
        end  
    end
    
    clear rank_ind
end

% Just for completeness, let's loop through the nodes that are '0' which
% should be ones that have another node as a parent
zero_ind = find(nodeList.distance == 0);

for mind=1:length(zero_ind)
    % zero is okay if its the swing node
    if (~strcmp(nodeList.name(zero_ind(mind)),'NONE'))
        my_parent = nodeList.parent(zero_ind(mind));
        parent_ind = find(ismember(nodeList.name,my_parent));
        
        if (isempty(parent_ind))
            % Note, this could just be zero length right after the swing
            % node, e.g., a regulator to another node
            disp(['Umm, could not find the parent in the node list (node ind = ' num2str(zero_ind(mind)) '). Now what?']);
        else
            nodeList.distance(zero_ind(mind),1) = nodeList.distance(parent_ind);
            nodeList.parent_index(zero_ind(mind),1) = parent_ind;
        end
    end
end
    
save('linkList2.mat','linkList');
save('nodeList2.mat','nodeList');
    
    
    