function [co, cb, r, dispatch, success] = ...
            smartmkt(mpc, offers, bids, mkt, mpopt)
%SMARTMKT  Runs the PowerWeb smart market.
%   [CO, CB, RESULTS, DISPATCH, SUCCESS] = SMARTMKT(MPC, ...
%       OFFERS, BIDS, MKT, MPOPT) runs the ISO smart market.

%   MATPOWER
%   $Id: smartmkt.m 4738 2014-07-03 00:55:39Z dchassin $
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
%% default arguments
if nargin < 5
    mpopt = mpoption;       %% use default options
end

%% options
verbose = mpopt(31);

%% initialize some stuff
G = find( ~isload(mpc.gen) );       %% real generators
L = find(  isload(mpc.gen) );       %% dispatchable loads
nL = length(L);
if isfield(offers, 'Q') || isfield(bids, 'Q')
    haveQ = 1;
else
    haveQ = 0;
end

if haveQ && mkt.auction_type ~= 0 && mkt.auction_type ~= 5
    error(['smartmkt: Combined active/reactive power markets ', ...
            'are only implemented for auction types 0 and 5']);
end

%% set power flow formulation based on market
mpopt = mpoption(mpopt, 'PF_DC', strcmp(mkt.OPF, 'DC'));

%% define named indices into data matrices
[PQ, PV, REF, NONE, BUS_I, BUS_TYPE, PD, QD, GS, BS, BUS_AREA, VM, ...
    VA, BASE_KV, ZONE, VMAX, VMIN, LAM_P, LAM_Q, MU_VMAX, MU_VMIN] = idx_bus;
[GEN_BUS, PG, QG, QMAX, QMIN, VG, MBASE, GEN_STATUS, PMAX, PMIN, ...
    MU_PMAX, MU_PMIN, MU_QMAX, MU_QMIN, PC1, PC2, QC1MIN, QC1MAX, ...
    QC2MIN, QC2MAX, RAMP_AGC, RAMP_10, RAMP_30, RAMP_Q, APF] = idx_gen;
[PW_LINEAR, POLYNOMIAL, MODEL, STARTUP, SHUTDOWN, NCOST, COST] = idx_cost;
[QUANTITY, PRICE, FCOST, VCOST, SCOST, PENALTY] = idx_disp;

%% set up cost info & generator limits
mkt.lim = pricelimits(mkt.lim, isfield(offers, 'Q') || isfield(bids, 'Q'));
[gen, genoffer] = off2case(mpc.gen, mpc.gencost, offers, bids, mkt.lim);

%% move Pmin and Pmax limits out slightly to avoid problems
%% with lambdas caused by rounding errors when corner point
%% of cost function lies at exactly Pmin or Pmax
if any(find(genoffer(:, MODEL) == PW_LINEAR))
    gg = find( ~isload(gen) );      %% skip dispatchable loads
    gen(gg, PMIN) = gen(gg, PMIN) - 100 * mpopt(16) * ones(size(gg));
    gen(gg, PMAX) = gen(gg, PMAX) + 100 * mpopt(16) * ones(size(gg));
end

%%-----  solve the optimization problem  -----
%% attempt OPF
mpc2 = mpc;
mpc2.gen = gen;
mpc2.gencost = genoffer;
[r, success] = uopf(mpc2, mpopt);
[bus, gen] = deal(r.bus, r.gen);
if verbose && ~success
    fprintf('\nSMARTMARKET: non-convergent UOPF');
end

%%-----  compute quantities, prices & costs  -----
%% compute quantities & prices
ng = size(gen, 1);
if success      %% OPF solved case fine
    %% create map of external bus numbers to bus indices
    i2e = bus(:, BUS_I);
    e2i = sparse(max(i2e), 1);
    e2i(i2e) = (1:size(bus, 1))';

    %% get nodal marginal prices from OPF
    gbus    = e2i(gen(:, GEN_BUS));                 %% indices of buses w/gens
    npP     = max([ size(offers.P.qty, 2) size(bids.P.qty, 2) ]);
    lamP    = sparse(1:ng, 1:ng, bus(gbus, LAM_P), ng, ng) * ones(ng, npP); %% real power prices
    lamQ    = sparse(1:ng, 1:ng, bus(gbus, LAM_Q), ng, ng) * ones(ng, npP); %% reactive power prices
    
    %% compute fudge factor for lamP to include price of bundled reactive power
    pf   = zeros(length(L), 1);                 %% for loads Q = pf * P
    Qlim =  (gen(L, QMIN) == 0) .* gen(L, QMAX) + ...
            (gen(L, QMAX) == 0) .* gen(L, QMIN);
    pf = Qlim ./ gen(L, PMIN);

    gtee_prc.offer = 1;         %% guarantee that cleared offers are >= offers
    Poffer = offers.P;
    Poffer.lam = lamP(G,:);
    Poffer.total_qty = gen(G, PG);
    
    Pbid = bids.P;
    Pbid.total_qty = -gen(L, PG);
    if haveQ
        Pbid.lam = lamP(L,:);   %% use unbundled lambdas
        gtee_prc.bid = 0;       %% allow cleared bids to be above bid price
    else
        Pbid.lam = lamP(L,:) + sparse(1:nL, 1:nL, pf, nL, nL) * lamQ(L,:);  %% bundled lambdas
        gtee_prc.bid = 1;       %% guarantee that cleared bids are <= bids
    end

    [co.P, cb.P] = auction(Poffer, Pbid, mkt.auction_type, mkt.lim.P, gtee_prc);

    if haveQ
        npQ = max([ size(offers.Q.qty, 2) size(bids.Q.qty, 2) ]);
        
        %% get nodal marginal prices from OPF
        lamQ    = sparse(1:ng, 1:ng, bus(gbus, LAM_Q), ng, ng) * ones(ng, npQ); %% reactive power prices

        Qoffer = offers.Q;
        Qoffer.lam = lamQ;      %% use unbundled lambdas
        Qoffer.total_qty = (gen(:, QG) > 0) .* gen(:, QG);
        
        Qbid = bids.Q;
        Qbid.lam = lamQ;        %% use unbundled lambdas
        Qbid.total_qty = (gen(:, QG) < 0) .* -gen(:, QG);

        %% too complicated to scale with mixed bids/offers
        %% (only auction_types 0 and 5 allowed)
        [co.Q, cb.Q] = auction(Qoffer, Qbid, mkt.auction_type, mkt.lim.Q, gtee_prc);
    end

    quantity    = gen(:, PG);
    price       = zeros(ng, 1);
    price(G)    = co.P.prc(:, 1);   %% need these for prices for
    price(L)    = cb.P.prc(:, 1);   %% gens that are shut down
    if npP == 1
        k = find( co.P.qty );
        price(G(k)) = co.P.prc(k, :);
        k = find( cb.P.qty );
        price(L(k)) = cb.P.prc(k, :);
    else
        k = find( sum( co.P.qty' )' );
        price(G(k)) = sum( co.P.qty(k, :)' .* co.P.prc(k, :)' )' ./ sum( co.P.qty(k, :)' )';
        k = find( sum( cb.P.qty' )' );
        price(L(k)) = sum( cb.P.qty(k, :)' .* cb.P.prc(k, :)' )' ./ sum( cb.P.qty(k, :)' )';
    end
else        %% did not converge even with imports
    quantity    = zeros(ng, 1);
    price       = mkt.lim.P.max_offer * ones(ng, 1);
    co.P.qty = zeros(size(offers.P.qty));
    co.P.prc = zeros(size(offers.P.prc));
    cb.P.qty = zeros(size(bids.P.qty));
    cb.P.prc = zeros(size(bids.P.prc));
    if haveQ
        co.Q.qty = zeros(size(offers.Q.qty));
        co.Q.prc = zeros(size(offers.Q.prc));
        cb.Q.qty = zeros(size(bids.Q.qty));
        cb.Q.prc = zeros(size(bids.Q.prc));
    end
end


%% compute costs in $ (note, NOT $/hr)
fcost   = mkt.t * totcost(mpc.gencost, zeros(ng, 1) );      %% fixed costs
vcost   = mkt.t * totcost(mpc.gencost, quantity     ) - fcost;  %% variable costs
scost   =   (~mkt.u0 & gen(:, GEN_STATUS) >  0) .* ...
                mpc.gencost(:, STARTUP) + ...               %% startup costs
            ( mkt.u0 & gen(:, GEN_STATUS) <= 0) .* ...
                mpc.gencost(:, SHUTDOWN);                   %% shutdown costs

%% store in dispatch
dispatch = zeros(ng, PENALTY);
dispatch(:, [QUANTITY PRICE FCOST VCOST SCOST]) = [quantity price fcost vcost scost];
