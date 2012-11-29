function lim = pricelimits(lim, haveQ)
%PRICELIMITS  Fills in a struct with default values for offer/bid limits.
%   LIM = PRICELIMITS(LIM, HAVEQ)
%   The final structure looks like:
%       LIM.P.min_bid           - bids below this are withheld
%            .max_offer         - offers above this are withheld
%            .min_cleared_bid   - cleared bid prices below this are clipped
%            .max_cleared_offer - cleared offer prices above this are clipped
%          .Q       (optional, same structure as P)

%   MATPOWER
%   $Id: pricelimits.m,v 1.5 2010/04/26 19:45:26 ray Exp $
%   by Ray Zimmerman, PSERC Cornell
%   Copyright (c) 2005-2010 by Power System Engineering Research Center (PSERC)
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

if isempty(lim)
    if haveQ
        lim = struct( 'P', fill_lim([]), 'Q', fill_lim([]) );
    else
        lim = struct( 'P', fill_lim([]) );
    end
else
    if ~isfield(lim, 'P')
        lim.P = [];
    end
    lim.P = fill_lim(lim.P);
    if haveQ
        if ~isfield(lim, 'Q')
            lim.Q = [];
        end
        lim.Q = fill_lim(lim.Q);
    end
end



function lim = fill_lim(lim)
if isempty(lim)
    lim = struct( 'max_offer', [], 'min_bid', [], ...
                  'max_cleared_offer', [], 'min_cleared_bid', [] );
else
    if ~isfield(lim, 'max_offer'),         lim.max_offer = [];         end
    if ~isfield(lim, 'min_bid'),           lim.min_bid = [];           end
    if ~isfield(lim, 'max_cleared_offer'), lim.max_cleared_offer = []; end
    if ~isfield(lim, 'min_cleared_bid'),   lim.min_cleared_bid = [];   end
end
