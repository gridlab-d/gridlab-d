function [val, idx] = fairmax(x)
%FAIRMAX    Same as built-in MAX, except breaks ties randomly.
%   [VAL, IDX] = FAIRMAX(X) takes a vector as an argument and returns
%   the same output as the built-in function MAX with two output
%   parameters, except that where the maximum value occurs at more
%   than one position in the  vector, the index is chosen randomly
%   from these positions as opposed to just choosing the first occurance.
%
%   See also MAX.

%   MATPOWER
%   $Id: fairmax.m,v 1.7 2010/04/26 19:45:25 ray Exp $
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

val = max(x);               %% find max value
i   = find(x == val);       %% find all positions where this occurs
n   = length(i);            %% number of occurences
idx = i( fix(n*rand)+1 );   %% select index randomly among occurances
