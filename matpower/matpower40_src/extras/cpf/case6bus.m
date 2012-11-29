function [baseMVA, bus, gen, branch] = case6bus
%CASE6BUS  6-bus system
%   From in problem 3.6 in book 'Computational
%   Methods for Electric Power Systems' by Mariesa Crow
%   created by Rui Bo on 2007/11/12

%   MATPOWER
%   $Id: case6bus.m,v 1.3 2010/04/19 14:40:12 ray Exp $

%%-----  Power Flow Data  -----%%
%% system MVA base
baseMVA = 100;

%% bus data
%	bus_i	type	Pd	Qd	Gs	Bs	area	Vm	Va	baseKV	zone	Vmax	Vmin
bus = [
	1	3	0.25	0.1	0	0	1	1	0	230	1	1.1	0.9;
	2	2	0.15	0.05	0	0	1	1	0	230	1	1.1	0.9;
	3	1	0.275	0.11	0	0	1	1	0	230	1	1.1	0.9;
	4	1	0	0	0	0	1	1	0	230	1	1.1	0.9;
	5	1	0.15	0.09	0	0	1	1	0	230	1	1.1	0.9;
	6	1	0.25	0.15	0	0	1	1	0	230	1	1.1	0.9;
];

bus(:, 3) = bus(:, 3)*baseMVA;
bus(:, 4) = bus(:, 4)*baseMVA;

%% generator data
% Note: 
% 1)It's better of gen to be in number order, otherwise gen and genbid
% should be sorted to make the lp solution output clearly(in number order as well)
% 2)set Pmax to nonzero. set to 999 if no limit 
% 3)If change the order of gen, then must change the order in genbid
% accordingly
%	bus	Pg	Qg	Qmax	Qmin	Vg	mBase	status	Pmax	Pmin
gen = [
	1	0	0	100	-100	1.05	100	1	100	0;
	2	0.5	0	100	-100	1.05	100	1	100	0;
];

gen(:, 2) = gen(:, 2)*baseMVA;

%% branch data
%	fbus	tbus	r	x	b	rateA	rateB	rateC	ratio	angle	status
branch = [
	1	4	0.020	0.185	0.009	999	100	100	0	0	1;
	1	6	0.031	0.259	0.010	999	100	100	0	0	1;
	2	3	0.006	0.025	0	999	100	100	0	0	1;
	2	5	0.071	0.320	0.015	999	100	100	0	0	1;
	4	6	0.024	0.204	0.010	999	100	100	0	0	1;
	3	4	0.075	0.067	0	999	100	100	0	0	1;
	5	6	0.025	0.150	0.017	999	100	100	0	0	1;
];

%branch(:, 3) = branch(:, 3)*1.75;

return;
