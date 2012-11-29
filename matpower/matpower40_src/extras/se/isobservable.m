function TorF = isobservable(H, pv, pq)
%ISOBSERVABLE  Test for observability.
%   returns 1 if the system is observable, 0 otherwise.
%   created by Rui Bo on Jan 9, 2010

%   MATPOWER
%   $Id: isobservable.m,v 1.3 2010/04/26 19:45:26 ray Exp $
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

%% options
tol     = 1e-5; % mpopt(2);
check_reason = 1;	% check reason for system being not observable
                    % 0: no check
                    % 1: check (NOTE: may be time consuming due to svd calculation)

%% test if H is full rank
[m, n]  = size(H);
r       = rank(H);
if r < min(m, n)
    TorF = 0;
else
    TorF = 1;
end

%% look for reasons for system being not observable
if check_reason && ~TorF
    %% look for variables not being observed
    idx_trivialColumns = [];
    varNames = {};
    for j = 1:n
        normJ = norm(H(:, j), inf);
        if normJ < tol % found a zero column
            idx_trivialColumns = [idx_trivialColumns j];
            varName = getVarName(j, pv, pq);
            varNames{length(idx_trivialColumns)} = varName;
        end
    end

    if ~isempty(idx_trivialColumns) % found zero-valued column vector
        fprintf('Warning: The following variables are not observable since they are not related with any measurement!');
        varNames
        idx_trivialColumns
    else % no zero-valued column vector
        %% look for dependent column vectors
        for j = 1:n
            rr = rank(H(:, 1:j));
            if rr ~= j % found dependent column vector
                %% look for linearly depedent vector
                colJ = H(:, j); % j(th) column of H
                varJName = getVarName(j, pv, pq);
                for k = 1:j-1
                    colK = H(:, k);
                    if rank([colK colJ]) < 2 % k(th) column vector is linearly dependent of j(th) column vector
                        varKName = getVarName(k, pv, pq);
                        fprintf('Warning: %d(th) column vector (w.r.t. %s) of H is linearly dependent of %d(th) column vector (w.r.t. %s)!\n', j, varJName, k, varKName);
                        return;
                    end
                end
            end
        end
    fprintf('Warning: No specific reason was found for system being not observable.\n');
    end
end
