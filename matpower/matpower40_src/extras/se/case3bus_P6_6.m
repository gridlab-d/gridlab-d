function [baseMVA, bus, gen, branch, areas, gencost] = case3bus_P6_6
%CASE3BUS_P6_6  Case of 3 bus system.
%   From Problem 6.6 in book 'Computational
%   Methods for Electric Power Systems' by Mariesa Crow
%   created by Rui Bo on 2007/11/12

%   MATPOWER
%   $Id: case3bus_P6_6.m,v 1.3 2010/04/19 14:40:12 ray Exp $

%%-----  Power Flow Data  -----%%
%% system MVA base
baseMVA = 1000;

%% bus data
%	bus_i	type	Pd	Qd	Gs	Bs	area	Vm	Va	baseKV	zone	Vmax	Vmin
bus = [
	1   3   350   	100    0	0	1	1	0	230	1	1.00	1.00;
	2   2   400   	250    0	0	1	1	0	230	1	1.02	1.02;
	3   2   250     100    0	0	1	1	0	230	1	1.02	1.02;
];

%% generator data
% Note: 
% 1)It's better of gen to be in number order, otherwise gen and genbid
% should be sorted to make the lp solution output clearly(in number order as well)
% 2)set Pmax to nonzero. set to 999 if no limit 
% 3)If change the order of gen, then must change the order in genbid
% accordingly
%	bus	Pg	Qg	Qmax	Qmin	Vg	mBase	status	Pmax	Pmin
gen = [
	1	182.18   0	999	-999	1.00       100	1	600	0;
	2	272.77 	 0	999	-999	1.02       100	1	400	0;
	3	545.05   0	999	-999	1.02       100	1	100	0;
];
%gen(:, 9) = 999; % inactive the Pmax constraints

%% branch data
%	fbus	tbus	r	x	b	rateA	rateB	rateC	ratio	angle	status
branch = [
	1	2	0.01     0.1    0.050	999	100	100	0	0	1;
	1	3	0.05     0.1    0.025	999	100	100	0	0	1;
	2	3	0.05     0.1    0.025  	999	100	100	0	0	1;
];

%%-----  OPF Data  -----%%
%% area data
areas = [
	1	1;
];

%% generator cost data
%	2	startup	shutdown	n	c(n-1)	...	c0
gencost = [
	2	0	0	3	1.5 	1	0;
	2	0	0	3	1   	2	0;
	2	0	0	3	0.5  	2.5	0;
];

return;
