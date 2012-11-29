function [r, co, cb, f, dispatch, success, et] = ...
                runmarket(mpc, offers, bids, mkt, mpopt, fname, solvedcase)
%RUNMARKET  Runs PowerWeb-style smart market.
%   [RESULTS, CO, CB, F, DISPATCH, SUCCESS, ET] = ...
%           RUNMARKET(MPC, OFFERS, BIDS, MKT, MPOPT, FNAME, SOLVEDCASE)
%
%   Computes the new generation and price schedules (cleared offers and bids)
%   based on the OFFERS and BIDS submitted. See OFF2CASE for a
%   description of the OFFERS and BIDS arguments. MKT is a struct with the
%   following fields:
%       auction_type - market used for dispatch and pricing
%       t            - time duration of the dispatch period in hours
%       u0           - vector of gen commitment status from prev period
%       lim          - offer/bid/price limits (see 'help pricelimits')
%       OPF          - 'AC' or 'DC', default is 'AC'
%
%   MPOPT is an optional MATPOWER options vector (see MPOPTION for
%   details). The values for the auction_type field are defined as follows:
%
%      0 - discriminative pricing (price equal to offer or bid)
%      1 - last accepted offer auction
%      2 - first rejected offer auction
%      3 - last accepted bid auction
%      4 - first rejected bid auction
%      5 - first price auction (marginal unit, offer or bid, sets the price)
%      6 - second price auction (if offer is marginal, then price is set
%                                   by min(FRO,LAB), if bid, then max(FRB,LAO)
%      7 - split the difference pricing (price set by last accepted offer & bid)
%      8 - LAO sets seller price, LAB sets buyer price
%
%   The default auction_type is 5, where the marginal block (offer or bid)
%   sets the price. The default lim sets no offer/bid or price limits. The
%   default previous commitment status u0 is all ones (assume everything was
%   running) and the default duration t is 1 hour. The results may
%   optionally be printed to a file (appended if the file exists) whose name
%   is given in FNAME (in addition to printing to STDOUT). Optionally
%   returns the final values of the solved case in results, the cleared
%   offers and bids in CO and CB, the objective function value F, the old
%   style DISPATCH matrix, the convergence status of the OPF in SUCCESS, and
%   the elapsed time ET. If a name is given in SOLVEDCASE, the solved case
%   will be written to a case file in MATPOWER format with the specified
%   name.
%
%   See also OFF2CASE.

%   MATPOWER
%   $Id: runmarket.m,v 1.12 2010/05/24 15:51:50 ray Exp $
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

%%-----  initialize  -----
%% default arguments
if nargin < 7
    solvedcase = '';                        %% don't save solved case
    if nargin < 6
        fname = '';                         %% don't print results to a file
        if nargin < 5
            mpopt = mpoption;               %% use default options
            if nargin < 4
                mkt = [];      %% use default market
                if nargin < 3
                    bids = struct([]);
                    if nargin < 2
                        offers = struct([]);
                        if nargin < 1
                            mpc = 'case9';  %% default data file is 'case9.m'
                        end
                    end
                end
            end
        end
    end
end

%% read data & convert to internal bus numbering
mpc = loadcase(mpc);

%% assign default arguments
if isempty(mkt)
    mkt = struct( 'OPF', [], 'auction_type', [], 'lim', [], 'u0', [], 't', []);
end
if ~isfield(mkt, 'OPF') || isempty(mkt.OPF)
    mkt.OPF = 'AC';         %% default OPF is AC
end
if ~isfield(mkt, 'auction_type') || isempty(mkt.auction_type)
    mkt.auction_type = 5;   %% default auction type is first price
end
if ~isfield(mkt, 'lim') || isempty(mkt.lim)
    mkt.lim = pricelimits([], isfield(offers, 'Q') || isfield(bids, 'Q'));
end
if ~isfield(mkt, 'u0') || isempty(mkt.u0)
    mkt.u0 = ones(size(mpc.gen, 1), 1); %% default for previous gen commitment, all on
end
if ~isfield(mkt, 't') || isempty(mkt.t)
    mkt.t = 1;              %% default dispatch duration in hours
end

%% if offers not defined, use gencost
if isempty(offers) || isempty(offers.P.qty)
    [q, p] = case2off(mpc.gen, mpc.gencost);

    %% find indices for gens and variable loads
    G = find( ~isload(mpc.gen) );   %% real generators
    L = find(  isload(mpc.gen) );   %% variable loads
    offers = struct( 'P', struct( 'qty', q(G, :), 'prc', p(G, :) ) );
    bids   = struct( 'P', struct( 'qty', q(L, :), 'prc', p(L, :) ) );
end
if isempty(bids)
	np = size(offers.P.qty, 2);
    bids = struct( 'P', struct('qty', zeros(0,np), 'prc', zeros(0,np)));
end

%% start the clock
t0 = clock;

%% run the market
[co, cb, r, dispatch, success] = smartmkt(mpc, offers, bids, mkt, mpopt);

%% compute elapsed time
et = etime(clock, t0);

%% print results
if fname
    [fd, msg] = fopen(fname, 'at');
    if fd == -1
        error(msg);
    else
        printmkt(r, mkt.t, dispatch, success, fd, mpopt);
        fclose(fd);
    end
end
printmkt(r, mkt.t, dispatch, success, 1, mpopt);

%% save solved case
if solvedcase
    savecase(solvedcase, r);
end

if nargout > 3
    f = r.f;
end
