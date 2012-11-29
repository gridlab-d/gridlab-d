function [x, success] = LPeqslvr(x, om, Ybus, Yf, Yt, Afeq, bfeq, Af, bf, mpopt, il)
%------------------------------  deprecated  ------------------------------
%   OPF solvers based on LPCONSTR to be removed in a future version.
%--------------------------------------------------------------------------
%LPEQSLVR
%   [X, SUCCESS] = LPEQSLVR(X, OM, YBUS, YF, YT, AFEQ, BFEQ, AF, BF, MPOPT, IL)
%
%   See also LPOPF_SOLVER.

%   MATPOWER
%   $Id: LPeqslvr.m,v 1.22 2010/04/26 19:45:25 ray Exp $
%   by Deqiang (David) Gan, PSERC Cornell & Zhejiang University
%   and Ray Zimmerman, PSERC Cornell
%   Copyright (c) 1996-2010 by Power System Engineering Research Center (PSERC)
%
%   This file is part of MATPOWER.
%   See http://www.pserc.cornell.edu/matpower/ for more info.
%
%   MATPOWER is free software: you can redistribute it and/or modify
%   it under the terms of the GNU General Public License as published
%   by the Free Software Foundation, either version 3 of the License,
%   or (at your option) any later version.
%
%   MATPOWER is distributed in the hope that it will be useful,
%   but WITHOUT ANY WARRANTY; without even the implied warranty of
%   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
%   GNU General Public License for more details.
%
%   You should have received a copy of the GNU General Public License
%   along with MATPOWER. If not, see <http://www.gnu.org/licenses/>.
%
%   Additional permission under GNU GPL version 3 section 7
%
%   If you modify MATPOWER, or any covered work, to interface with
%   other modules (such as MATLAB code and MEX-files) available in a
%   MATLAB(R) or comparable environment containing parts covered
%   under other licensing terms, the licensors of MATPOWER grant
%   you additional permission to convey the resulting work.


%% define named indices into data matrices
[PQ, PV, REF, NONE, BUS_I, BUS_TYPE, PD, QD, GS, BS, BUS_AREA, VM, ...
    VA, BASE_KV, ZONE, VMAX, VMIN, LAM_P, LAM_Q, MU_VMAX, MU_VMIN] = idx_bus;
[GEN_BUS, PG, QG, QMAX, QMIN, VG, MBASE, GEN_STATUS, PMAX, PMIN, ...
    MU_PMAX, MU_PMIN, MU_QMAX, MU_QMIN, PC1, PC2, QC1MIN, QC1MAX, ...
    QC2MIN, QC2MAX, RAMP_AGC, RAMP_10, RAMP_30, RAMP_Q, APF] = idx_gen;
[PW_LINEAR, POLYNOMIAL, MODEL, STARTUP, SHUTDOWN, NCOST, COST] = idx_cost;

%% options
verbose = mpopt(31);            %% verbose

%% default args
if nargin < 11
    il = [];
end

%% unpack data
mpc = get_mpc(om);
[baseMVA, bus, gen, branch, gencost] = ...
    deal(mpc.baseMVA, mpc.bus, mpc.gen, mpc.branch, mpc.gencost);
vv = get_idx(om);

%% problem dimensions
ny = getN(om, 'var', 'y');  %% number of piece-wise linear costs

%% set default constrained lines
if isempty(il)
    nl = size(branch, 1);   %% number of branches
    il = (1:nl);            %% all lines have limits by default
end

%% parse x, update bus, gen
bus(:, VA) = x(vv.i1.Va:vv.iN.Va) * 180/pi;
bus(:, VM) = x(vv.i1.Vm:vv.iN.Vm);
gen(:, PG) = x(vv.i1.Pg:vv.iN.Pg) * baseMVA;
gen(:, QG) = x(vv.i1.Qg:vv.iN.Qg) * baseMVA;

%% turn down verbosity one level for call to power flow
if verbose
    mpopt = mpoption(mpopt, 'VERBOSE', verbose-1);
end

%% get bus index lists of each type of bus
[ref, pv, pq] = bustypes(bus, gen);

%% run the power flow
V = bus(:, VM) .* exp(1j * bus(:, VA) * pi/180);
Sbus = makeSbus(baseMVA, bus, gen);
[V, success, iterations] = newtonpf(Ybus, Sbus, V, ref, pv, pq, mpopt);   %% do NR iteration
[bus, gen, branch] = pfsoln(baseMVA, bus, gen, branch(il,:), Ybus, Yf, Yt, V, ref, pv, pq);   %% post-processing
% printpf(baseMVA, bus, gen, branch, [], success, 0, 1, mpopt);


%% update x
x(vv.i1.Va:vv.iN.Va) = bus(:, VA) * pi/180;
x(vv.i1.Vm:vv.iN.Vm) = bus(:, VM);
x(vv.i1.Pg:vv.iN.Pg) = gen(:, PG) / baseMVA;
x(vv.i1.Qg:vv.iN.Qg) = gen(:, QG) / baseMVA;
if ny > 0
    PgQg = [gen(:, PG); gen(:, QG)];
    ipwl = find(gencost(:, MODEL) == PW_LINEAR);  %% piece-wise linear costs
    x(vv.i1.y:vv.iN.y) = totcost(gencost(ipwl, :), PgQg(ipwl));
end
