function [x2, duals_rlx, idx_workc, idx_bindc] = LPrelax(a, f, b, nequs, vlb, vub, idx_workc, mpopt)
%------------------------------  deprecated  ------------------------------
%   OPF solvers based on LPCONSTR to be removed in a future version.
%--------------------------------------------------------------------------
%LPRELAX
%   [X2, DUALS_RLX, IDX_WORKC, IDX_BINDC] = ...
%       LPRELAX(A, F, B, NEQUS, VLB, VUB, IDX_WORKC, MPOPT)
%
%   See also LPOPF_SOLVER, MP_LP.

%   MATPOWER
%   $Id: LPrelax.m,v 1.17 2010/11/23 14:34:40 cvs Exp $
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
alg     = mpopt(11);

if alg == 320       %% dense LP
    idx_workc = find(b < 0.001);
end


converged = 0;
while converged == 0

    atemp = a(idx_workc, :);
    btemp = b(idx_workc);

    %% solve via BPMPD_MEX
    [x2, duals] = mp_lp(f, atemp, btemp, vlb, vub, [], nequs, -1, 100);

    diffs = b - a * x2;                 % diffs should be normalized by what means? under development
    idx_bindc = find(diffs < 1.0e-8);
    
    if isempty(find(diffs < -1.0e-8))
        converged = 1;
    else
        flag = zeros(length(b), 1);         % set up flag from scratch
        flag(idx_workc) = ones(length(idx_workc), 1);   % enforce historical working constraints


        idx_add = find(diffs < 0.001);
        flag(idx_add) = ones(length(idx_add), 1);   % enforce violating constraints



        flag(1:nequs) = ones(nequs, 1);         % enforce original equality constraints
        idx_workc_new = find(flag);

        if length(idx_workc) == length(idx_workc_new)   % safeguard step
            if isempty(find(idx_workc - idx_workc_new))
                converged = 1;
            end
        end
        idx_workc = idx_workc_new;

    end

end

duals_rlx = zeros(length(b), 1);
duals_rlx(idx_workc) = duals(1:length(btemp));

