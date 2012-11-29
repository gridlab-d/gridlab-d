function om = add_vars(om, name, N, v0, vl, vu)
%ADD_VARS  Adds a set of variables to the model.
%   OM = ADD_VARS(OM, NAME, N, V0, VL, VU)
%   OM = ADD_VARS(OM, NAME, N, V0, VL)
%   OM = ADD_VARS(OM, NAME, N, V0)
%   OM = ADD_VARS(OM, NAME, N)
%   
%   Adds a set of variables to the model, where N is the number of
%   variables in the set, V0 is the initial value of those variables,
%   and VL and VU are the lower and upper bounds on the variables.
%   The defaults for the last three arguments, which are optional,
%   are for all values to be initialized to zero (V0 = 0) and unbounded
%   (VL = -Inf, VU = Inf).
%
%   See also OPF_MODEL, GETV.

%   MATPOWER
%   $Id: add_vars.m,v 1.7 2010/04/26 19:45:25 ray Exp $
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

%% prevent duplicate named var sets
if isfield(om.var.idx.N, name)
    error('@opf_model/add_vars: variable set named ''%s'' already exists', name);
end

%% initialize args and assign defaults
if nargin < 6
    vu = [];
    if nargin < 5
        vl = [];
        if nargin < 4
            v0 = [];
        end
    end
end
if isempty(v0)
    v0 = zeros(N, 1);           %% init to zero by default
end
if isempty(vl)
    vl = -Inf * ones(N, 1);     %% unbounded below by default
end
if isempty(vu)
    vu = Inf * ones(N, 1);      %% unbounded above by default
end

%% add info about this var set
om.var.idx.i1.(name)  = om.var.N + 1;   %% starting index
om.var.idx.iN.(name)  = om.var.N + N;   %% ending index
om.var.idx.N.(name)   = N;              %% number of vars
om.var.data.v0.(name) = v0;             %% initial value
om.var.data.vl.(name) = vl;             %% lower bound
om.var.data.vu.(name) = vu;             %% upper bound

%% update number of vars and var sets
om.var.N  = om.var.idx.iN.(name);
om.var.NS = om.var.NS + 1;

%% put name in ordered list of var sets
om.var.order{om.var.NS} = name;
