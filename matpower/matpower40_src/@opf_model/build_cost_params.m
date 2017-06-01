function om = build_cost_params(om)
%BUILD_COST_PARAMS  Builds and saves the full generalized cost parameters.
%   OM = BUILD_COST_PARAMS(OM)
%   OM = BUILD_COST_PARAMS(OM, 'force')
%
%   Builds the full set of cost parameters from the individual named
%   sub-sets added via ADD_COST. Skips the building process if it has
%   already been done, unless a second input argument is present.
%
%   These cost parameters can be retrieved by calling GET_COST_PARAMS
%   and the user-defined costs evaluated by calling COMPUTE_COST.
%
%   See also OPF_MODEL, ADD_COST, GET_COST_PARAMS, COMPUTE_COST.

%   MATPOWER
%   $Id: build_cost_params.m 4738 2014-07-03 00:55:39Z dchassin $
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

if nargin > 1 || ~isfield(om.cost.params, 'N')
    %% initialize parameters
    nw = om.cost.N;
    nnzN = 0;
    nnzH = 0;
    for k = 1:om.cost.NS
        name = om.cost.order{k};
        nnzN = nnzN + nnz(om.cost.data.N.(name));
        if isfield(om.cost.data.H, name)
            nnzH = nnzH + nnz(om.cost.data.H.(name));
        end
    end
    N = sparse([], [], [], nw, om.var.N, nnzN);
    Cw = zeros(nw, 1);
    H = sparse([], [], [], nw, nw, nnzH);   %% default => no quadratic term
    dd = ones(nw, 1);                       %% default => linear
    rh = Cw;                                %% default => no shift
    kk = Cw;                                %% default => no dead zone
    mm = dd;                                %% default => no scaling
    
    %% fill in each piece
    for k = 1:om.cost.NS
        name = om.cost.order{k};
        Nk = om.cost.data.N.(name);          %% N for kth cost set
        i1 = om.cost.idx.i1.(name);          %% starting row index
        iN = om.cost.idx.iN.(name);          %% ending row index
        if om.cost.idx.N.(name)              %% non-zero number of rows to add
            vsl = om.cost.data.vs.(name);       %% var set list
            kN = 0;                             %% initialize last col of Nk used
            for v = 1:length(vsl)
                j1 = om.var.idx.i1.(vsl{v});    %% starting column in N
                jN = om.var.idx.iN.(vsl{v});    %% ending column in N
                k1 = kN + 1;                    %% starting column in Nk
                kN = kN + om.var.idx.N.(vsl{v});%% ending column in Nk
                N(i1:iN, j1:jN) = Nk(:, k1:kN);
            end
            Cw(i1:iN) = om.cost.data.Cw.(name);
            if isfield(om.cost.data.H, name)
                H(i1:iN, i1:iN) = om.cost.data.H.(name);
            end
            if isfield(om.cost.data.dd, name)
                dd(i1:iN) = om.cost.data.dd.(name);
            end
            if isfield(om.cost.data.rh, name)
                rh(i1:iN) = om.cost.data.rh.(name);
            end
            if isfield(om.cost.data.kk, name)
                kk(i1:iN) = om.cost.data.kk.(name);
            end
            if isfield(om.cost.data.mm, name)
                mm(i1:iN) = om.cost.data.mm.(name);
            end
        end
    end

    %% save in object   
    om.cost.params = struct( ...
        'N', N, 'Cw', Cw, 'H', H, 'dd', dd, 'rh', rh, 'kk', kk, 'mm', mm );
end
