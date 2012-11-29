function [co, cb] = auction(offers, bids, auction_type, limit_prc, gtee_prc)
%AUCTION  Clear auction based on OPF results (qty's and lambdas).
%   [CO, CB] = AUCTION(OFFERS, BIDS, AUCTION_TYPE, LIMIT_PRC, GTEE_PRC)
%   Clears a set of BIDS and OFFERS based on the results of an OPF, where the
%   pricing is adjusted for network losses and binding constraints.
%   The arguments OFFERS and BIDS are structs with the following fields:
%       qty       - m x n, offer/bid quantities, m offers/bids, n blocks
%       prc       - m x n, offer/bid prices
%       lam       - m x n, corresponding lambdas
%       total_qty - m x 1, total quantity cleared for each offer/bid
%
%   There are 8 types of auctions implemented, specified by AUCTION_TYPE.
%
%      0 - discriminative pricing (price equal to offer or bid)
%      1 - last accepted offer auction
%      2 - first rejected offer auction
%      3 - last accepted bid auction
%      4 - first rejected bid auction
%      5 - first price auction (marginal unit, offer or bid, sets the price)
%      6 - second price auction (if offer is marginal, price set by
%             min(FRO,LAB), else max(FRB,LAO)
%      7 - split the difference pricing (set by last accepted offer & bid)
%      8 - LAO sets seller price, LAB sets buyer price
%
%   Whether or not cleared offer (bid) prices are guaranteed to be greater
%   (less) than or equal to the corresponding offer (bid) price is specified by
%   a flag GTEE_PRC.offer (GTEE_PRC.bid). The default is value true.
%   
%   Offer/bid and cleared offer/bid min and max prices are specified in the
%   LIMIT_PRC struct with the following fields:
%       max_offer
%       min_bid
%       max_cleared_offer
%       min_cleared_bid
%   Offers (bids) above (below) max_offer (min_bid) are treated as withheld
%   and cleared offer (bid) prices above (below) max_cleared_offer
%   (min_cleared_bid) are clipped to max_cleared offer (min_cleared_bid) if
%   given. All of these limit prices are ignored if the field is missing
%   or is empty.
%
%   See also RUNMARKET, SMARTMKT.

%   MATPOWER
%   $Id: auction.m,v 1.26 2010/04/26 19:45:26 ray Exp $
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

%%-----  initialization  -----
%% define named indices into data matrices
[PQ, PV, REF, NONE, BUS_I, BUS_TYPE, PD, QD, GS, BS, BUS_AREA, VM, ...
    VA, BASE_KV, ZONE, VMAX, VMIN, LAM_P, LAM_Q, MU_VMAX, MU_VMIN] = idx_bus;
[GEN_BUS, PG, QG, QMAX, QMIN, VG, MBASE, GEN_STATUS, PMAX, PMIN, ...
    MU_PMAX, MU_PMIN, MU_QMAX, MU_QMIN, PC1, PC2, QC1MIN, QC1MAX, ...
    QC2MIN, QC2MAX, RAMP_AGC, RAMP_10, RAMP_30, RAMP_Q, APF] = idx_gen;

%% initialize some stuff
delta = 1e-3;       %% prices smaller than this are not used to determine X
zero_tol = 1e-5;
% zero_tol = 0.1;   %% fmincon is SO bad with prices that it is
                    %% NOT recommended for use with auction.m
big_num = 1e6;
if isempty(bids)
    bids = struct(  'qty', [], ...
                    'prc', [], ...
                    'lam', [], ...
                    'total_qty', [] );
end
if nargin < 4 || isempty(limit_prc)
    limit_prc = struct( 'max_offer', [], 'min_bid', [], ...
                        'max_cleared_offer', [], 'min_cleared_bid', [] );
else
    if ~isfield(limit_prc, 'max_offer'),         limit_prc.max_offer = [];         end
    if ~isfield(limit_prc, 'min_bid'),           limit_prc.min_bid = [];           end
    if ~isfield(limit_prc, 'max_cleared_offer'), limit_prc.max_cleared_offer = []; end
    if ~isfield(limit_prc, 'min_cleared_bid'),   limit_prc.min_cleared_bid = [];   end
end
if nargin < 5 || isempty(gtee_prc)
    gtee_prc = struct(  'offer', 1, 'bid', 1    );
else
    if ~isfield(gtee_prc, 'offer'), gtee_prc.offer = 1; end
    if ~isfield(gtee_prc, 'bid'),   gtee_prc.bid = 1;   end
end

[nro, nco] = size(offers.qty);
[nrb, ncb] = size(bids.qty);

%% determine cleared quantities
if isempty(limit_prc.max_offer)
    [co.qty, o.on, o.off] = clear_qty(offers.qty, offers.total_qty);
else
    mask = offers.prc <= limit_prc.max_offer;
    [co.qty, o.on, o.off] = clear_qty(offers.qty, offers.total_qty, mask);
end
if isempty(limit_prc.min_bid)
    [cb.qty, b.on, b.off] = clear_qty(bids.qty, bids.total_qty);
else
    mask = bids.prc <= limit_prc.min_bid;
    [cb.qty, b.on, b.off] = clear_qty(bids.qty, bids.total_qty, mask);
end

%% initialize cleared prices
co.prc = zeros(nro, nco);       %% cleared offer prices
cb.prc = zeros(nrb, ncb);       %% cleared bid prices

%%-----  compute exchange rates to scale lam to get desired pricing  -----
%% The locationally adjusted offer/bid price, when normalized to an arbitrary
%% reference location where lambda is equal to ref_lam, is:
%%      norm_prc = prc * (ref_lam / lam)
%% Then we can define the ratio between the normalized offer/bid prices
%% and the ref_lam as an exchange rate X:
%%      X = norm_prc / ref_lam = prc / lam
%% This X represents the ratio between the marginal unit (setting lambda)
%% and the offer/bid price in question.

if auction_type == 0 || auction_type == 5   %% don't bother scaling anything
    X = struct( 'LAO', 1, ...
                'FRO', 1, ...
                'LAB', 1, ...
                'FRB', 1);
else
    X = compute_exchange_rates(offers, bids, o, b);
end

%% cleared offer/bid prices for different auction types
if auction_type == 0        %% discriminative
    co.prc = offers.prc;
    cb.prc = bids.prc;
elseif auction_type == 1    %% LAO
    co.prc = offers.lam * X.LAO;
    cb.prc = bids.lam   * X.LAO;
elseif auction_type == 2    %% FRO
    co.prc = offers.lam * X.FRO;
    cb.prc = bids.lam   * X.FRO;
elseif auction_type == 3    %% LAB
    co.prc = offers.lam * X.LAB;
    cb.prc = bids.lam   * X.LAB;
elseif auction_type == 4    %% FRB
    co.prc = offers.lam * X.FRB;
    cb.prc = bids.lam   * X.FRB;
elseif auction_type == 5    %% 1st price
    co.prc = offers.lam;
    cb.prc = bids.lam;
elseif auction_type == 6    %% 2nd price
    if abs(1 - X.LAO) < zero_tol
        co.prc = offers.lam * min(X.FRO,X.LAB);
        cb.prc = bids.lam   * min(X.FRO,X.LAB);
    else
        co.prc = offers.lam * max(X.LAO,X.FRB);
        cb.prc = bids.lam   * max(X.LAO,X.FRB);
    end
elseif auction_type == 7    %% split the difference
    co.prc = offers.lam * (X.LAO + X.LAB) / 2;
    cb.prc = bids.lam   * (X.LAO + X.LAB) / 2;
elseif auction_type == 8    %% LAO seller, LAB buyer
    co.prc = offers.lam * X.LAO;
    cb.prc = bids.lam   * X.LAB;
end

%% guarantee that cleared offer prices are >= offers
if gtee_prc.offer
    clip = o.on .* (offers.prc - co.prc);
    co.prc = co.prc + (clip > zero_tol) .* clip;
end

%% guarantee that cleared bid prices are <= bids
if gtee_prc.bid
    clip = b.on .* (bids.prc - cb.prc);
    cb.prc = cb.prc + (clip < -zero_tol) .* clip;
end

%% clip cleared offer prices by limit_prc.max_cleared_offer
if ~isempty(limit_prc.max_cleared_offer)
    co.prc = co.prc + (co.prc > limit_prc.max_cleared_offer) .* ...
        (limit_prc.max_cleared_offer - co.prc);
end

%% clip cleared bid prices by limit_prc.min_cleared_bid
if ~isempty(limit_prc.min_cleared_bid)
    cb.prc = cb.prc + (cb.prc < limit_prc.min_cleared_bid) .* ...
        (limit_prc.min_cleared_bid - co.prc);
end

%% make prices uniform after clipping (except for discrim auction)
%% since clipping may only affect a single block of a multi-block generator
if auction_type ~= 0
    %% equal to largest price in row
    if nco > 1
        co.prc = diag(max(co.prc')) * ones(nro,nco);
    end
    if ncb > 1
        cb.prc = diag(min(cb.prc')) * ones(nrb,ncb);
    end
end


function X = compute_exchange_rates(offers, bids, o, b, delta)
%COMPUTE_EXCHANGE_RATES Determine the scale factors for LAO, FRO, LAB, FRB
%  Inputs:
%   offers, bids (same as for auction)
%   o, b  - structs with on, off fields, each same dim as qty field of offers
%           or bids, 1 if corresponding block is accepted, 0 otherwise
%   delta - optional prices smaller than this are not used to determine X
%  Outputs:
%   X     - struct with fields LAO, FRO, LAB, FRB containing scale factors
%           to use for each type of auction

if nargin < 5
    delta = 1e-3;       %% prices smaller than this are not used to determine X
end
zero_tol = 1e-5;

%% eliminate terms with lam < delta (X would not be accurate)
olam = offers.lam;
blam = bids.lam;
olam(olam(:) < delta) = NaN;
blam(blam(:) < delta) = NaN;

%% eliminate rows for 0 qty offers/bids
[nro, nco] = size(offers.qty);
[nrb, ncb] = size(bids.qty);
omask = ones(nro,nco);
if nco == 1
    temp = offers.qty;
else
    temp = sum(offers.qty')';
end
omask(temp == 0, :) = NaN;
bmask = ones(nrb,ncb);
if ncb == 1
    temp = bids.qty;
else
    temp = sum(bids.qty')';
end
bmask(temp == 0, :) = NaN;

%% by default, don't scale anything
X.LAO = 1;
X.FRO = 1;
X.LAB = 1;
X.FRB = 1;

%% don't scale if we have any negative lambdas or all are too close to 0
if all(all(offers.lam > -zero_tol))
    %% ratios
    Xo = omask .* offers.prc ./ olam;
    Xb = bmask .* bids.prc   ./ blam;

    %% exchange rate for LAO (X.LAO * lambda == LAO, for corresponding lambda)
    X.LAO = o.on .* Xo;
    X.LAO( o.off(:) ) = NaN;
    X.LAO( X.LAO(:) > 1+zero_tol ) = NaN;   %% don't let gens @ Pmin set price
    X.LAO = max( X.LAO(:) );

    %% exchange rate for FRO (X.FRO * lambda == FRO, for corresponding lambda)
    X.FRO = o.off .* Xo;
    X.FRO( o.on(:) ) = NaN;
    X.FRO = min( X.FRO(:) );

    if nrb
        %% exchange rate for LAB (X.LAB * lambda == LAB, for corresponding lambda)
        X.LAB = b.on .* Xb;
        X.LAB( b.off(:) ) = NaN;
        X.LAB( X.LAB(:) < 1-zero_tol ) = NaN;   %% don't let set price
        X.LAB = min( X.LAB(:) );
        
        %% exchange rate for FRB (X.FRB * lambda == FRB, for corresponding lambda)
        X.FRB = b.off .* Xb;
        X.FRB( b.on(:) ) = NaN;
        X.FRB = max( X.FRB(:) );
    end
end


function [cqty, on, off] = clear_qty(qty, total_cqty, mask)
%CLEAR_QTY  Computed cleared offer/bid quantities from totals.
%  Inputs:
%   qty        - m x n, offer/bid quantities, m offers/bids, n blocks
%   total_cqty - m x 1, total cleared quantity for each offer/bid
%   mask       - m x n, boolean indicating which offers/bids are valid (not withheld)
%  Outputs:
%   cqty       - m x n, cleared offer/bid quantities, m offers/bids, n blocks
%   on         - m x n, 1 if partially or fully accepted, 0 if rejected
%   off        - m x n, 1 if rejected, 0 if partially or fully accepted

[nr, nc] = size(qty);
accept  = zeros(nr,nc);
cqty    = zeros(nr,nc);
for i = 1:nr            %% offer/bid i
    for j = 1:nc        %% block j
        if qty(i, j)    %% ignore zero quantity offers/bids
            %% compute fraction of the block accepted ...
            accept(i, j) = (total_cqty(i) - sum(qty(i, 1:j-1))) / qty(i, j);
            %% ... clipped to the range [0, 1]  (i.e. 0-100%)
            if accept(i, j) > 1
                accept(i, j) = 1;
            elseif accept(i, j) < 1.0e-5
                accept(i, j) = 0;
            end
            cqty(i, j) = qty(i, j) * accept(i, j);
        end
    end
end

if nargin == 3
    accept = mask .* accept;
end

on  = (accept  > 0);
off = (accept == 0);
