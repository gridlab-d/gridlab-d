function [x, duals, idx_workc, idx_bindc] = LPsetup(a, f, b, nequs, vlb, vub, idx_workc, mpopt)
%------------------------------  deprecated  ------------------------------
%   OPF solvers based on LPCONSTR to be removed in a future version.
%--------------------------------------------------------------------------
%LPSETUP  Solves a LP problem using a callable LP routine.
%   [X, DUALS, IDX_WORKC, IDX_BINDC] = ...
%       LPSETUP(A, F, B, NEQUS, VLB, VUB, IDX_WORKC, MPOPT)
%
%   The LP problem is defined as follows:
%
%   min     f' * x
%   S.T.    a * x =< b
%           vlb =< x =< vub
%
%   All of the equality constraints must appear before inequality constraints.
%   NEQUS specifies how many of the constraints are equality constraints.
%
%   The algorithm (set in MPOPT) can be set to the following options:
%
%   320 - solve LP using ICS (equality constraints are eliminated)
%   340 - solve LP using Iterative Constraint Search (ICS)
%         (equality constraints are preserved, typically superior to 320 & 360)
%   360 - solve LP with full set of constraints
%
%   See also LPOPF_SOLVER.

%   MATPOWER
%   $Id: LPsetup.m 4738 2014-07-03 00:55:39Z dchassin $
%   by Deqiang (David) Gan, PSERC Cornell & Zhejiang University
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

%% options
alg = mpopt(11);

% ----- solve LP directly -----

if alg == 360                   %% sparse LP with full constraints
    [x, duals] = mp_lp(f, a, b, vlb, vub, [], nequs, -1);
    duals = duals(1:length(b));                   % MATLAB built-in LP solver has more elements in duals than we want
    idx_workc = []; idx_bindc = [];
    return;
end

% ----- solve LP using constraint relaxation (equality constraints are preserved) ------

if alg == 340                   %% sparse LP with relaxed constraints
    if isempty(idx_workc) == 1
        idx_workc = find(b < 1.0e-5);
    end
    [x, duals, idx_workc, idx_bindc] = LPrelax(a, f, b, nequs, vlb, vub, idx_workc, mpopt);
    return;
end

% ----- solve LP using constraint relaxation (equality constraints are eliminated) ------

% so alg == 320                 %% dense LP

% set up the indicies of variables and constraints

idx_x1 = 2:nequs; idx_x2 = [1 nequs:length(f)];
idx_c1 = 1:nequs-1; idx_c2 = nequs:length(b);

% eliminate equality constraints

b1 = b(idx_c1);
b2 = b(idx_c2);

a11 = a(idx_c1, idx_x1); a12 = a(idx_c1, idx_x2);
a21 = a(idx_c2, idx_x1); a22 = a(idx_c2, idx_x2);

a11b1 = a11 \ b1;
a11a12 = a11 \ a12;

% set up the reduced LP

fred = -((f(idx_x1))' * a11a12)' + f(idx_x2);
ared =  [-a21 * a11a12 + a22
     -a11a12
      a11a12];
bred =  [ b2 - a21 * a11b1
     vub(idx_x1) - a11b1
     a11b1 - vlb(idx_x1)];
vubred = vub(idx_x2);
vlbred = vlb(idx_x2);
nequsred = nequs - length(idx_x1);

% solve the reduced LP problem using constraint relaxation

if isempty(idx_workc) == 1
    idx_workc = find(b2< 1.0e-5);
end
[x2, dualsred, idx_workc, idx_bindc] = LPrelax(ared, fred, bred, nequsred, vlbred, vubred, idx_workc, mpopt);

% parse the solution of the reduced LP to get the solution of the original LP

x(idx_x1) = a11b1 - a11a12 * x2;  x(idx_x2) = x2; x = x';

dualsc2 = dualsred(1:length(idx_c2));

temp = find(dualsc2);
dualsc1 =  a11' \ ( -f(idx_x1) - (a21(temp, :))' * dualsc2(temp) );

duals(idx_c1) = dualsc1;
duals(idx_c2) = dualsc2;
duals = duals';
