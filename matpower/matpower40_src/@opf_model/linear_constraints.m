function [A, l, u] = linear_constraints(om)
%LINEAR_CONSTRAINTS  Builds and returns the full set of linear constraints.
%   [A, L, U] = LINEAR_CONSTRAINTS(OM)
%   Builds the full set of linear constraints based on those added by
%   ADD_CONSTRAINTS.
%
%       L <= A * x <= U
%
%   Example:
%       [A, l, u] = linear_constraints(om);
%
%   See also OPF_MODEL, ADD_CONSTRAINTS.

%   MATPOWER
%   $Id: linear_constraints.m,v 1.7 2010/04/26 19:45:25 ray Exp $
%   by Ray Zimmerman, PSERC Cornell
%   Copyright (c) 2008-2010 by Power System Engineering Research Center (PSERC)
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


%% initialize A, l and u
nnzA = 0;
for k = 1:om.lin.NS
    nnzA = nnzA + nnz(om.lin.data.A.(om.lin.order{k}));
end
A = sparse([], [], [], om.lin.N, om.var.N, nnzA);
u = Inf * ones(om.lin.N, 1);
l = -u;

%% fill in each piece
for k = 1:om.lin.NS
    name = om.lin.order{k};
    N = om.lin.idx.N.(name);
    if N                                %% non-zero number of rows to add
        Ak = om.lin.data.A.(name);          %% A for kth linear constrain set
        i1 = om.lin.idx.i1.(name);          %% starting row index
        iN = om.lin.idx.iN.(name);          %% ending row index
        vsl = om.lin.data.vs.(name);        %% var set list
        kN = 0;                             %% initialize last col of Ak used
        Ai = sparse(N, om.var.N);
        for v = 1:length(vsl)
            j1 = om.var.idx.i1.(vsl{v});    %% starting column in A
            jN = om.var.idx.iN.(vsl{v});    %% ending column in A
            k1 = kN + 1;                    %% starting column in Ak
            kN = kN + om.var.idx.N.(vsl{v});%% ending column in Ak
            Ai(:, j1:jN) = Ak(:, k1:kN);
        end

        A(i1:iN, :) = Ai;        
        l(i1:iN) = om.lin.data.l.(name);
        u(i1:iN) = om.lin.data.u.(name);
    end
end
