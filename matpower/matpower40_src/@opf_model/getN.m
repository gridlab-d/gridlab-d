function N = getN(om, selector, name)
%GETN  Returns the number of variables, constraints or cost rows.
%   N = GETN(OM, SELECTOR)
%   N = GETN(OM, SELECTOR, NAME)
%
%   Returns either the total number of variables/constraints/cost rows
%   or the number corresponding to a specified named block.
%
%   Examples:
%       N = getN(om, 'var')         : total number of variables
%       N = getN(om, 'lin')         : total number of linear constraints
%       N = getN(om, 'nln')         : total number of nonlinear constraints
%       N = getN(om, 'cost')        : total number of cost rows (in N)
%       N = getN(om, 'var', name)   : number of variables in named set
%       N = getN(om, 'lin', name)   : number of linear constraints in named set
%       N = getN(om, 'nln', name)   : number of nonlinear cons. in named set
%       N = getN(om, 'cost', name)  : number of cost rows (in N) in named set
%
%   See also OPF_MODEL.

%   MATPOWER
%   $Id: getN.m,v 1.5 2010/06/09 14:56:58 ray Exp $
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

if nargin < 3
    N = om.(selector).N;
else
    if isfield(om.(selector).idx.N, name)
        N = om.(selector).idx.N.(name);
    else
        N = 0;
    end
end
