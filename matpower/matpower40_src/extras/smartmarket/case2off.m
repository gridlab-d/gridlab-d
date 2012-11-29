function [q, p] = case2off(gen, gencost)
%CASE2OFF  Creates quantity & price offers from gen & gencost.
%   [Q, P] = CASE2OFF(GEN, GENCOST) creates quantity and price offers
%   from case variables GEN & GENCOST.
%
%   See also OFF2CASE.

%   MATPOWER
%   $Id: case2off.m,v 1.14 2010/04/26 19:45:26 ray Exp $
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

%% define named indices into data matrices
[GEN_BUS, PG, QG, QMAX, QMIN, VG, MBASE, GEN_STATUS, PMAX, PMIN, ...
    MU_PMAX, MU_PMIN, MU_QMAX, MU_QMIN, PC1, PC2, QC1MIN, QC1MAX, ...
    QC2MIN, QC2MAX, RAMP_AGC, RAMP_10, RAMP_30, RAMP_Q, APF] = idx_gen;
[PW_LINEAR, POLYNOMIAL, MODEL, STARTUP, SHUTDOWN, NCOST, COST] = idx_cost;

%% do conversion
oldgencost = gencost;
i_poly = find(gencost(:, MODEL) == POLYNOMIAL);
npts = 6;                   %% 6 points => 5 blocks
%% convert polynomials to piece-wise linear by evaluating at zero and then
%% at evenly spaced points between Pmin and Pmax
if any(i_poly)
    [m, n] = size(gencost(i_poly, :));                              %% size of piece being changed
    gencost(i_poly, MODEL) = PW_LINEAR * ones(m, 1);                %% change cost model
    gencost(i_poly, COST:n) = zeros(size(gencost(i_poly, COST:n))); %% zero out old data
    gencost(i_poly, NCOST) = npts * ones(m, 1);                     %% change number of data points
    
    for i = 1:m
        ig = i_poly(i);     %% index to gen
        Pmin = gen(ig, PMIN);
        Pmax = gen(ig, PMAX);
        if Pmin == 0
            step = (Pmax - Pmin) / (npts - 1);
            xx = (Pmin:step:Pmax);
        else
            step = (Pmax - Pmin) / (npts - 2);
            xx = [0 Pmin:step:Pmax];
        end
        yy = totcost(oldgencost(ig, :), xx);
        gencost(ig,     COST:2:(COST + 2*(npts-1)    )) = xx;
        gencost(ig, (COST+1):2:(COST + 2*(npts-1) + 1)) = yy;
    end
end
n = max(gencost(:, NCOST));
xx = gencost(:,     COST:2:( COST + 2*n - 1 ));
yy = gencost(:, (COST+1):2:( COST + 2*n     ));
i1 = 1:(n-1);
i2 = 2:n;
q = xx(:, i2) - xx(:, i1);
p = ( yy(:, i2) - yy(:, i1) ) ./ q;
