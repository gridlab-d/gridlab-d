function [gen, gencost] = off2case(gen, gencost, offers, bids, lim)
%OFF2CASE  Updates case variables gen & gencost from quantity & price offers.
%   [GEN, GENCOST] = OFF2CASE(GEN, GENCOST, OFFERS, BIDS, LIM) updates
%   GEN & GENCOST variables based on the OFFERS and BIDS supplied, where each
%   is a struct (or BIDS can be an empty matrix) with field 'P' (active power
%   offer/bid) and optional field 'Q' (reactive power offer/bid), each of which
%   is another struct with fields 'qty' and 'prc', m x n matrices of quantity
%   and price offers/bids, respectively. There are m offers with n blocks each.
%   For OFFERS, m can be equal to the number of actual generators (not including
%   dispatchable loads) or the total number of rows in the GEN matrix (including
%   dispatchable loads). For BIDS, m can be equal to the number of dispatchable
%   loads or the total number of rows in the GEN matrix. Non-zero offer (bid)
%   quantities for GEN matrix entries where Pmax <= 0 (Pmin >= 0) produce an
%   error. Similarly for Q.
%   
%   E.g.
%       OFFERS.P.qty - m x n, active power quantity offers, m offers, n blocks
%               .prc - m x n, active power price offers
%             .Q.qty - m x n, reactive power quantity offers
%               .prc - m x n, reactive power price offers
%
%   These values are used to update PMIN, PMAX, QMIN, QMAX and GEN_STATUS
%   columns of the GEN matrix and all columns of the GENCOST matrix except
%   STARTUP and SHUTDOWN.
%
%   The last argument, LIM is a struct with the following fields,
%   all of which are optional:
%       LIM.P.min_bid
%            .max_offer
%          .Q.min_bid
%            .max_offer
%   Any price offers (bids) for real power above (below) LIM.P.max_offer
%   (LIM.P.min_bid) will be treated as being withheld. Likewise for Q.
%
%   See also CASE2OFF.

%   MATPOWER
%   $Id: off2case.m,v 1.29 2010/05/24 15:51:50 ray Exp $
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

%% default args and stuff
if nargin < 5
    lim = [];
    if nargin < 4
        bids = [];
    end
end
if isfield(offers, 'Q') || isfield(bids, 'Q')
    haveQ = 1;
else
    haveQ = 0;
end
lim = pricelimits(lim, haveQ);
if isempty(bids)
	np = size(offers.P.qty, 2);
    bids = struct( 'P', struct('qty', zeros(0,np), 'prc', zeros(0,np)));
end
if haveQ
    if ~isfield(bids, 'Q')
        bids.Q = struct('qty', [], 'prc', []);
    elseif ~isfield(offers, 'Q')
        offers.Q = struct('qty', [], 'prc', []);
    end
end

%% indices and sizes
ngc = size(gencost, 2);
G = find( ~isload(gen) );       %% real generators
L = find(  isload(gen) );       %% dispatchable loads
nGL = size(gen, 1);
[idxPo, idxPb, idxQo, idxQb] = idx_vecs(offers, bids, G, L, haveQ);
if haveQ
    if size(gencost, 1) == nGL
        %% set all reactive costs to zero if not provided
        gencost = [ ...
            gencost;
            [PW_LINEAR * ones(nGL, 1) gencost(:,[STARTUP SHUTDOWN]) 2*ones(nGL,1) zeros(nGL,ngc-4) ]
        ];
        gencost(G+nGL, COST+2) =  1;
        gencost(L+nGL, COST)   = -1;
    elseif size(gencost, 1) ~= 2 * nGL
        error('gencost should have either %d or %d rows', nGL, 2*nGL);
    end
end

%% number of points to define piece-wise linear cost
if any(idxPo & idxPb)
    np = size(offers.P.qty, 2) + size(bids.P.qty, 2);
else
    np = max([ size(offers.P.qty, 2) size(bids.P.qty, 2) ]);
end
if haveQ
    if any(idxQo & idxQb)
        np = max([ np size(offers.Q.qty, 2) + size(bids.Q.qty, 2) ]);
    else
        np = max([ np size(offers.Q.qty, 2) size(bids.Q.qty, 2) ]);
    end
end
np = np + 1;
if any(idxPo + idxPb == 0)  %% some gens have no offer or bid, use original cost
    np = max([ np ceil(ngc-NCOST)/2 ]);
end

%% initialize new cost matrices
Pgencost            = zeros(nGL, COST + 2*np - 1);
Pgencost(:, MODEL)  = PW_LINEAR * ones(nGL, 1);
Pgencost(:, [STARTUP SHUTDOWN]) = gencost(1:nGL, [STARTUP SHUTDOWN]);
if haveQ
    Qgencost = Pgencost;
    Qgencost(:, [STARTUP SHUTDOWN]) = gencost(nGL+(1:nGL), [STARTUP SHUTDOWN]);
end

for i = 1:nGL
    %% convert active power bids & offers into piecewise linear segments
    if idxPb(i)     %% there is a bid for this unit
        if gen(i, PMIN) >= 0 && any(bids.P.qty(idxPb(i), :))
            error('Pmin >= 0, bid not allowed for gen %d', i);
        end
        [xxPb, yyPb, nPb] = offbid2pwl(bids.P.qty(idxPb(i), :), bids.P.prc(idxPb(i), :), 1, lim.P.min_bid);
    else
        nPb = 0;
    end
    if idxPo(i)     %% there is an offer for this unit
        if gen(i, PMAX) <= 0 && any(offers.P.qty(idxPo(i), :))
            error('Pmax <= 0, offer not allowed for gen %d', i);
        end
        [xxPo, yyPo, nPo] = offbid2pwl(offers.P.qty(idxPo(i), :), offers.P.prc(idxPo(i), :), 0, lim.P.max_offer);
    else
        nPo = 0;
    end
    %% convert reactive power bids & offers into piecewise linear segments
    if haveQ
        if idxQb(i)     %% there is a bid for this unit
            if gen(i, QMIN) >= 0 && any(bids.Q.qty(idxQb(i), :))
                error('Qmin >= 0, reactive bid not allowed for gen %d', i);
            end
            [xxQb, yyQb, nQb] = offbid2pwl(bids.Q.qty(idxQb(i), :), bids.Q.prc(idxQb(i), :), 1, lim.Q.min_bid);
        else
            nQb = 0;
        end
        if idxQo(i)     %% there is an offer for this unit
            if gen(i, QMAX) <= 0 && any(offers.Q.qty(idxQo(i), :))
                error('Qmax <= 0, reactive offer not allowed for gen %d', i);
            end
            [xxQo, yyQo, nQo] = offbid2pwl(offers.Q.qty(idxQo(i), :), offers.Q.prc(idxQo(i), :), 0, lim.Q.max_offer);
        else
            nQo = 0;
        end
    else
        nQb = 0;
        nQo = 0;
    end

    %% collect the pwl segments for active power
    if nPb > 1 && nPo > 1           %% bid and offer (positive and negative qtys)
        if xxPb(end) || yyPb(end) || xxPo(1) || yyPo(1)
            error('Oops ... these 4 numbers should be zero: %g %g %g %g\n', ...
                xxPb(end), yyPb(end), xxPo(1), yyPo(1));
        end
        xxP = [xxPb xxPo(2:end)];
        yyP = [yyPb yyPo(2:end)];
        npP = nPb + nPo - 1;
    elseif  nPb <= 1 && nPo > 1 %% offer only
        xxP = xxPo;
        yyP = yyPo;
        npP = nPo;
    elseif  nPb > 1 && nPo <= 1 %% bid only
        xxP = xxPb;
        yyP = yyPb;
        npP = nPb;
    else
        npP = 0;
    end

    %% collect the pwl segments for reactive power
    if nQb > 1 && nQo > 1           %% bid and offer (positive and negative qtys)
        if xxQb(end) || yyQb(end) || xxQo(1) || yyQo(1)
            error('Oops ... these 4 numbers should be zero: %g %g %g %g\n', ...
                xxQb(end), yyQb(end), xxQo(1), yyQo(1));
        end
        xxQ = [xxQb xxQo(2:end)];
        yyQ = [yyQb yyQo(2:end)];
        npQ = nQb + nQo - 1;
    elseif  nQb <= 1 && nQo > 1 %% offer only
        xxQ = xxQo;
        yyQ = yyQo;
        npQ = nQo;
    elseif  nQb > 1 && nQo <= 1 %% bid only
        xxQ = xxQb;
        yyQ = yyQb;
        npQ = nQb;
    else
        npQ = 0;
    end

    %% initialize new gen limits
    Pmin = gen(i, PMIN);
    Pmax = gen(i, PMAX);
    Qmin = gen(i, QMIN);
    Qmax = gen(i, QMAX);

    %% update real part of gen and gencost
    if npP
        %% update gen limits
        if gen(i, PMAX) > 0
            Pmax = max(xxP);
            if Pmax < gen(i, PMIN) || Pmax > gen(i, PMAX)
                error('offer quantity (%g) must be between max(0,PMIN) (%g) and PMAX (%g)', ...
                    Pmax, max([0,gen(i, PMIN)]), gen(i, PMAX));
            end
        end
        if gen(i, PMIN) < 0
            Pmin = min(xxP);
            if Pmin >= gen(i, PMIN) && Pmin <= gen(i, PMAX)
                if isload(gen(i, :))
                    Qmin = gen(i, QMIN) * Pmin / gen(i, PMIN);
                    Qmax = gen(i, QMAX) * Pmin / gen(i, PMIN);
                end
            else
                error('bid quantity (%g) must be between max(0,-PMAX) (%g) and -PMIN (%g)', ...
                    -Pmin, max([0 -gen(i, PMAX)]), -gen(i, PMIN));
            end
        end

        %% update gencost
        Pgencost(i, NCOST) = npP;
        Pgencost(i,      COST:2:( COST + 2*npP - 2 )) = xxP;
        Pgencost(i,  (COST+1):2:( COST + 2*npP - 1 )) = yyP;
    else
        %% no capacity bid/offered for active power
        if npQ && ~isload(gen(i,:)) && gen(i, PMIN) <= 0 && gen(i, PMAX) >= 0
            %% but we do have a reactive bid/offer and we can dispatch
            %% at zero real power without shutting down
            Pmin = 0;
            Pmax = 0;
            Pgencost(i, 1:ngc) = gencost(i, 1:ngc);
        else            %% none for reactive either
            %% shut down the unit
            gen(i, GEN_STATUS) = 0;
        end
    end

    %% update reactive part of gen and gencost
    if npQ
        %% update gen limits
        if gen(i, QMAX) > 0
            Qmax = min([ Qmax max(xxQ) ]);
            if Qmax >= gen(i, QMIN) && Qmax <= gen(i, QMAX)
                if isload(gen(i, :))
                    Pmin = gen(i, PMIN) * Qmax / gen(i, QMAX);
                end
            else
                error('reactive offer quantity (%g) must be between max(0,QMIN) (%g) and QMAX (%g)', ...
                    Qmax, max([0,gen(i, QMIN)]), gen(i, QMAX));
            end
        end
        if gen(i, QMIN) < 0
            Qmin = max([ Qmin min(xxQ) ]);
            if Qmin >= gen(i, QMIN) && Qmin <= gen(i, QMAX)
                if isload(gen(i, :))
                    Pmin = gen(i, PMIN) * Qmin / gen(i, QMIN);
                end
            else
                error('reactive bid quantity (%g) must be between max(0,-QMAX) (%g) and -QMIN (%g)', ...
                    -Qmin, max([0 -gen(i, QMAX)]), -gen(i, QMIN));
            end
        end

        %% update gencost
        Qgencost(i, NCOST) = npQ;
        Qgencost(i,      COST:2:( COST + 2*npQ - 2 )) = xxQ;
        Qgencost(i,  (COST+1):2:( COST + 2*npQ - 1 )) = yyQ;
    else
        %% no capacity bid/offered for reactive power
        if haveQ
            if npP && gen(i, QMIN) <= 0 && gen(i, QMAX) >= 0
                %% but we do have an active bid/offer and we might be able to
                %% dispatch at zero reactive power without shutting down
                if isload(gen(i, :)) && (gen(i, QMAX) > 0 || gen(i, QMIN) < 0)
                    %% load w/non-unity power factor, zero Q => must shut down
                    gen(i, GEN_STATUS) = 0;
                else    %% can dispatch at zero reactive without shutting down
                    Qmin = 0;
                    Qmax = 0;
                end
                Qgencost(i, 1:ngc) = gencost(nGL+i, 1:ngc);
            else            %% none for reactive either
                %% shut down the unit
                gen(i, GEN_STATUS) = 0;
            end
        end
    end

    if gen(i, GEN_STATUS)       %% running
        gen(i, PMIN) = Pmin;    %% update limits
        gen(i, PMAX) = Pmax;
        gen(i, QMIN) = Qmin;
        gen(i, QMAX) = Qmax;
    else                        %% shut down
        %% do not modify cost
        Pgencost(i, 1:ngc) = gencost(i, 1:ngc);
        if haveQ
            Qgencost(i, 1:ngc) = gencost(nGL+i, 1:ngc);
        end
    end
end
if ~haveQ
    Qgencost = zeros(0, size(Pgencost, 2));
end
np = max([ Pgencost(:, NCOST); Qgencost(:, NCOST) ]);
ngc = NCOST + 2*np;
gencost = [ Pgencost(:, 1:ngc); Qgencost(:, 1:ngc) ];


%%-----  offbid2pwl()  -----
function [xx, yy, n] = offbid2pwl(qty, prc, isbid, lim)

if any(qty < 0)
    error('offer/bid quantities must be non-negative');
end

%% strip zero quantities and optionally strip prices beyond lim
if nargin < 4 || isempty(lim)
    valid = find(qty);
else
    if isbid
        valid = find(qty & prc >= lim);
    else
        valid = find(qty & prc <= lim);
    end
end

if isbid    
    n = length(valid);
    qq = qty(valid(n:-1:1));    %% row vector of quantities
    pp = prc(valid(n:-1:1));    %% row vector of prices
else
    qq = qty(valid);            %% row vector of quantities
    pp = prc(valid);            %% row vector of prices
end
n = length(qq) + 1;             %% number of points to define pwl function

%% form piece-wise linear total cost function
if n > 1        %% otherwise, leave all cost info zero (specifically NCOST)
    xx = [0 cumsum(qq)];
    yy = [0 cumsum(pp .* qq)];
    if isbid
        xx = xx - xx(end);
        yy = yy - yy(end);
    end
else
    xx = [];
    yy = [];
end

%%-----  idx_vecs()  -----
function [idxPo, idxPb, idxQo, idxQb] = idx_vecs(offers, bids, G, L, haveQ)

nG = length(G);
nL = length(L);
nGL = nG + nL;

idxPo = zeros(nGL, 1);
idxPb = zeros(nGL, 1);
idxQo = zeros(nGL, 1);
idxQb = zeros(nGL, 1);

%% numbers of offers/bids submitted
nPo = size(offers.P.qty, 1);
nPb = size(  bids.P.qty, 1);
if haveQ
    nQo = size(offers.Q.qty, 1);
    nQb = size(  bids.Q.qty, 1);
end

%% make sure dimensions of qty and prc offers/bids match
if any(size(offers.P.qty) ~= size(offers.P.prc))
    error('dimensions of offers.P.qty (%d x %d) and offers.P.prc (%d x %d) do not match',...
        size(offers.P.qty), size(offers.P.prc));
end
if any(size(bids.P.qty) ~= size(bids.P.prc))
    error('dimensions of bids.P.qty (%d x %d) and bids.P.prc (%d x %d) do not match',...
        size(bids.P.qty), size(bids.P.prc));
end
if haveQ
    if any(size(offers.Q.qty) ~= size(offers.Q.prc))
        error('dimensions of offers.Q.qty (%d x %d) and offers.Q.prc (%d x %d) do not match',...
            size(offers.Q.qty), size(offers.Q.prc));
    end
    if any(size(bids.Q.qty) ~= size(bids.Q.prc))
        error('dimensions of bids.Q.qty (%d x %d) and bids.Q.prc (%d x %d) do not match',...
            size(bids.Q.qty), size(bids.Q.prc));
    end
end

%% active power offer indices
if nPo == nGL
    idxPo = (1:nGL)';
elseif nPo == nG
    idxPo(G) = (1:nG)';
elseif nPo ~= 0
    error('number of active power offers must be zero or match either the number of generators or the total number of rows in gen');
end

%% active power bid indices
if nPb == nGL
    idxPb = (1:nGL)';
elseif nPb == nL
    idxPb(L) = (1:nL)';
elseif nPb ~= 0
    error('number of active power bids must be zero or match either the number of dispatchable loads or the total number of rows in gen');
end

if haveQ
    %% reactive power offer indices
    if nQo == nGL
        idxQo = (1:nGL)';
    elseif nQo == nG
        idxQo(G) = (1:nG)';
    elseif nQo ~= 0
        error('number of reactive power offers must be zero or match either the number of generators or the total number of rows in gen');
    end
    
    %% reactive power bid indices
    if nQb == nGL
        idxQb = (1:nGL)';
    elseif nQb ~= 0
        error('number of reactive power bids must be zero or match the total number of rows in gen');
    end
end
