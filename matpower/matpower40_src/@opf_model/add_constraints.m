function om = add_constraints(om, name, AorN, l, u, varsets)
%ADD_CONSTRAINTS  Adds a set of constraints to the model.
%   OM = ADD_CONSTRAINTS(OM, NAME, A, L, U);
%   OM = ADD_CONSTRAINTS(OM, NAME, A, L, U, VARSETS);
%   OM = ADD_CONSTRAINTS(OM, NAME, N, 'NON-LINEAR');
%
%   Linear constraints are of the form L <= A * x <= U, where
%   x is a vector made of of the vars specified in VARSETS (in
%   the order given). This allows the A matrix to be defined only
%   in terms of the relevant variables without the need to manually
%   create a lot of zero columns. If VARSETS is empty, x is taken
%   to be the full vector of all optimization variables. If L or 
%   U are empty, they are assumed to be appropriately sized vectors
%   of -Inf and Inf, respectively.
%
%   For nonlinear constraints, the 3rd argument, N, is the number
%   of constraints in the set. Currently, this is used internally
%   by MATPOWER, but there is no way for the user to specify
%   additional nonlinear constraints.
%
%   Examples:
%     om = add_constraints(om, 'vl', Avl, lvl, uvl, {'Pg', 'Qg'});
%     om = add_constraints(om, 'Pmis', nb, 'nonlinear');
%
%   See also OPF_MODEL, LINEAR_CONSTRAINTS.

%   MATPOWER
%   $Id: add_constraints.m,v 1.7 2010/06/09 14:56:58 ray Exp $
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

if nargin < 5       %% nonlinear
    %% prevent duplicate named constraint sets
    if isfield(om.nln.idx.N, name)
        error('@opf_model/add_constraints: nonlinear constraint set named ''%s'' already exists', name);
    end

    %% add info about this nonlinear constraint set
    om.nln.idx.i1.(name) = om.nln.N + 1;    %% starting index
    om.nln.idx.iN.(name) = om.nln.N + AorN; %% ending index
    om.nln.idx.N.(name)  = AorN;            %% number of constraints
    
    %% update number of nonlinear constraints and constraint sets
    om.nln.N  = om.nln.idx.iN.(name);
    om.nln.NS = om.nln.NS + 1;

    %% put name in ordered list of constraint sets
    om.nln.order{om.nln.NS} = name;
else                %% linear
    %% prevent duplicate named constraint sets
    if isfield(om.lin.idx.N, name)
        error('@opf_model/add_constraints: linear constraint set named ''%s'' already exists', name);
    end

    if nargin < 6
        varsets = {};
    end
    [N, M] = size(AorN);
    if isempty(l)                   %% default l is -Inf
        l = -Inf * ones(N, 1);
    end
    if isempty(u)                   %% default u is Inf
        u = Inf * ones(N, 1);
    end
    if isempty(varsets)
        varsets = om.var.order;
    end

    %% check sizes
    if size(l, 1) ~= N || size(u, 1) ~= N
        error('@opf_model/add_constraints: sizes of A, l and u must match');
    end
    nv = 0;
    for k = 1:length(varsets)
        nv = nv + om.var.idx.N.(varsets{k});
    end
    if M ~= nv
        error('@opf_model/add_constraints: number of columns of A does not match\nnumber of variables, A is %d x %d, nv = %d\n', N, M, nv);
    end

    %% add info about this linear constraint set
    om.lin.idx.i1.(name)  = om.lin.N + 1;   %% starting index
    om.lin.idx.iN.(name)  = om.lin.N + N;   %% ending index
    om.lin.idx.N.(name)   = N;              %% number of constraints
    om.lin.data.A.(name)  = AorN;
    om.lin.data.l.(name)  = l;
    om.lin.data.u.(name)  = u;
    om.lin.data.vs.(name) = varsets;
    
    %% update number of vars and var sets
    om.lin.N  = om.lin.idx.iN.(name);
    om.lin.NS = om.lin.NS + 1;
    
    %% put name in ordered list of var sets
    om.lin.order{om.lin.NS} = name;
end
