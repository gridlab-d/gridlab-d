function [varName] = getVarName(varIndex, pv, pq)
%GETVARNAME  Get variable name by variable index (as in H matrix).
%   [OUTPUT PARAMETERS]
%   varName: comprise both variable type ('Va', 'Vm') and the bus number of
%   the variable. For instance, Va8, Vm10, etc.
%   created by Rui Bo on Jan 9, 2010

%   MATPOWER
%   $Id: getVarName.m,v 1.3 2010/04/26 19:45:26 ray Exp $
%   by Rui Bo
%   Copyright (c) 2009-2010 by Rui Bo
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

%% get non reference buses
nonref = [pv;pq];

if varIndex <= length(nonref)
    varType = 'Va';
    newIdx = varIndex;
else
    varType = 'Vm';
    newIdx = varIndex - length(nonref);
end
varName = sprintf('%s%d', varType, nonref(newIdx));
