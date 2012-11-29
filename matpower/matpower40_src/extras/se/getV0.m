function V0 = getV0(bus, gen, type_initialguess, V0)
%GETV0  Get initial voltage profile for power flow calculation.
%   Note: The pv bus voltage will remain at the given value even for
%   flat start.
%   type_initialguess: 1 - initial guess from case data
%                      2 - flat start
%                      3 - from input

%   MATPOWER
%   $Id: getV0.m,v 1.4 2010/04/26 19:45:26 ray Exp $
%   by Rui Bo
%   Copyright (c) 2009-2010 by Rui Bo
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

%% define named indices into bus, gen, branch matrices
[PQ, PV, REF, NONE, BUS_I, BUS_TYPE, PD, QD, GS, BS, BUS_AREA, VM, ...
    VA, BASE_KV, ZONE, VMAX, VMIN, LAM_P, LAM_Q, MU_VMAX, MU_VMIN] = idx_bus;
[F_BUS, T_BUS, BR_R, BR_X, BR_B, RATE_A, RATE_B, ...
    RATE_C, TAP, SHIFT, BR_STATUS, PF, QF, PT, QT, MU_SF, MU_ST] = idx_brch;
[GEN_BUS, PG, QG, QMAX, QMIN, VG, MBASE, ...
    GEN_STATUS, PMAX, PMIN, MU_PMAX, MU_PMIN, MU_QMAX, MU_QMIN] = idx_gen;

%% generator info
on = find(gen(:, GEN_STATUS) > 0);      %% which generators are on?
gbus = gen(on, GEN_BUS);                %% what buses are they at?
if type_initialguess == 1 % using previous value in case data
    % NOTE: angle is in degree in case data, but in radians in pf solver,
    % so conversion from degree to radians is needed here
    V0  = bus(:, VM) .* exp(sqrt(-1) * pi/180 * bus(:, VA)); 
elseif type_initialguess == 2 % using flat start
    V0 = ones(size(bus, 1), 1);
elseif type_initialguess == 3 % using given initial voltage
    V0 = V0;
else
    fprintf('Error: unknow ''type_initialguess''.\n');
    pause
end
% set the voltages of PV bus and reference bus into the initial guess
V0(gbus) = gen(on, VG) ./ abs(V0(gbus)).* V0(gbus);
