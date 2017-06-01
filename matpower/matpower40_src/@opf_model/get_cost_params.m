function cp = get_cost_params(om, name)
%GET_COST_PARAMS  Returns the cost parameter struct for user-defined costs.
%   CP = GET_COST_PARAMS(OM)
%   CP = GET_COST_PARAMS(OM, NAME)
%
%   Requires calling BUILD_COST_PARAMS first to build the full set of
%   parameters. Returns the full cost parameter struct for all user-defined
%   costs that incorporates all of the named cost sets added via ADD_COSTS,
%   or, if a name is provided it returns the cost struct corresponding to
%   the named set of cost rows (N still has full number of columns).
%
%   The cost parameters are returned in a struct with the following fields:
%       N      - nw x nx sparse matrix
%       Cw     - nw x 1 vector
%       H      - nw x nw sparse matrix (optional, all zeros by default)
%       dd, mm - nw x 1 vectors (optional, all ones by default)
%       rh, kk - nw x 1 vectors (optional, all zeros by default)
%
%   See also OPF_MODEL, ADD_COST, BUILD_COST_PARAMS, COMPUTE_COST.

%   MATPOWER
%   $Id: get_cost_params.m 4738 2014-07-03 00:55:39Z dchassin $
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

if ~isfield(om.cost, 'params')
    error('@opf_model/get_cost_params: must call build_cost_params first');
end

cp = om.cost.params;

if nargin > 1
    if getN(om, 'cost', name)
        idx = om.cost.idx.i1.(name):om.cost.idx.iN.(name);
        cp.N  = cp.N(idx,:);
        cp.Cw = cp.Cw(idx);
        cp.H  = cp.H(idx,:);
        cp.dd = cp.dd(idx);
        cp.rh = cp.rh(idx);
        cp.kk = cp.kk(idx);
        cp.mm = cp.mm(idx);
    end
end
