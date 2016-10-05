% Either run XMLReader.m first, or load the structure that comes out of it
clear;
clc;

load('theStruct.mat')

%%
[~,max_ind] = size(theStruct.Children);

for ind=1:max_ind
    if strcmp(theStruct.Children(1,ind).Name,'node_list')
        node_list_ind = ind;
    elseif strcmp(theStruct.Children(1,ind).Name,'overhead_line_list')
        ohl_list_ind = ind;
    elseif strcmp(theStruct.Children(1,ind).Name,'underground_line_list')
        ugl_list_ind = ind;
    elseif strcmp(theStruct.Children(1,ind).Name,'transformer_list')
        tfmr_list_ind = ind;
    elseif strcmp(theStruct.Children(1,ind).Name,'regulator_list')
        reg_list_ind = ind;
    elseif strcmp(theStruct.Children(1,ind).Name,'triplex_node_list')
        tpn_list_ind = ind;
    elseif strcmp(theStruct.Children(1,ind).Name,'triplex_meter_list')
        tpm_list_ind = ind;
    elseif strcmp(theStruct.Children(1,ind).Name,'switch_list')
        sw_list_ind = ind;
    elseif strcmp(theStruct.Children(1,ind).Name,'triplex_line_list')
        tpl_list_ind = ind;
    elseif strcmp(theStruct.Children(1,ind).Name,'meter_list')
        met_list_ind = ind;
    end
end

nodeStruct = theStruct.Children(node_list_ind).Children;
ohlStruct = theStruct.Children(ohl_list_ind).Children;
uglStruct = theStruct.Children(ugl_list_ind).Children;
tfmrStruct = theStruct.Children(tfmr_list_ind).Children;
regStruct = theStruct.Children(reg_list_ind).Children;
tpnStruct = theStruct.Children(tpn_list_ind).Children;
tpmStruct = theStruct.Children(tpm_list_ind).Children;
swStruct = theStruct.Children(sw_list_ind).Children;
tplStruct = theStruct.Children(tpl_list_ind).Children;
metStruct = theStruct.Children(met_list_ind).Children;

clear theStruct