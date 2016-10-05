clear;
clc;

dir = 'C:\Users\d3x289\Documents\LAAR\3.1\VS2005\x64\Release\Base2 02 04 2015\';
xml_file = 'test_8500_XML.xml';

filename = [dir xml_file];

% PARSEXML Convert XML file to a MATLAB structure.
disp('Reading XML file...');
tree = xmlread(filename);
disp('Finished reading XML file...');

% Recurse over child nodes. This could run into problems 
% with very deeply nested trees.
disp('Parsing XML/DOM into Matlab structure...');
theStruct = parseChildNodes(tree);
disp('Finished parsing XML/DOM into Matlab structure.');

save('theStruct.mat','theStruct');
