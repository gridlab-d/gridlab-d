function [QUANTITY, PRICE, FCOST, VCOST, SCOST, PENALTY] = idx_disp
%IDX_DISP   Defines constants for named column indices to dispatch matrix.
%   Example:
%
%   [QUANTITY, PRICE, FCOST, VCOST, SCOST, PENALTY] = idx_disp;
%
%   The index, name and meaning of each column of the dispatch matrix is given
%   below:
% 
%   columns 1-6
%    1  QUANTITY    quantity produced by generator in MW
%    2  PRICE       market price for power produced by generator in $/MWh
%    3  FCOST       fixed cost in $/MWh
%    4  VCOST       variable cost in $/MWh
%    5  SCOST       startup cost in $
%    6  PENALTY     penalty cost in $ (not used)

%   MATPOWER
%   $Id: idx_disp.m,v 1.8 2010/04/26 19:45:26 ray Exp $
%   by Ray Zimmerman, PSERC Cornell
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

%% define the indices
QUANTITY        = 1;    %% quantity produced by generator in MW
PRICE           = 2;    %% market price for power produced by generator in $/MWh
FCOST           = 3;    %% fixed cost in $/MWh
VCOST           = 4;    %% variable cost in $/MWh
SCOST           = 5;    %% startup cost in $
PENALTY         = 6;    %% penalty cost in $ (not used)
