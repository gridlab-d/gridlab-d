function [AREA_I, PRICE_REF_BUS] = idx_area
%------------------------------  deprecated  ------------------------------
%   AREA matrix is not used by MATPOWER.
%--------------------------------------------------------------------------
%IDX_AREA   Defines constants for named column indices to areas matrix.
%   Example:
%
%   [AREA_I, PRICE_REF_BUS] = idx_area;
%
%   The index, name and meaning of each column of the areas matrix is given
%   below:
% 
%   columns 1-2
%    1  AREA_I          area number
%    2  PRICE_REF_BUS   price reference bus for this area

%   MATPOWER
%   $Id: idx_area.m,v 1.11 2010/04/26 19:45:26 ray Exp $
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
AREA_I          = 1;    %% area number
PRICE_REF_BUS   = 2;    %% price reference bus for this area
