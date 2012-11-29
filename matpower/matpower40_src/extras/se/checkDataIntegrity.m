function [success, measure, idx, sigma] = checkDataIntegrity(measure, idx, sigma, nbus)
%CHECKDATAINTEGRITY  Check state estimation input data integrity.
%   returns 1 if the data is complete, 0 otherwise.
%   NOTE: for each type of measurements, the measurement vector and index
%   vector should have the same length. If not, the longer vector will be
%   truncated to have the same length as the shorter vector.
%   created by Rui Bo on Jan 9, 2010

%   MATPOWER
%   $Id: checkDataIntegrity.m,v 1.3 2010/04/26 19:45:26 ray Exp $
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
verbose = 2;    % mpopt(31);

success     = 1;    % pass integrity check?
nowarning   = 1;    % no warning found?

%% check input data consistency
% for PF
if length(measure.PF) ~= length(idx.idx_zPF)
    fprintf('Warning: measurement vector and index vector for PF do not have the same length. The longer vector will be truncated.\n');
    min_len = min(length(measure.PF), length(idx.idx_zPF));
    measure.PF  = measure.PF(1:min_len);
    idx.idx_zPF = idx.idx_zPF(1:min_len);
    nowarning = 0;
end
if ~isempty(idx.idx_zPF) && length(sigma.sigma_PF) <= 0 % no sigma defined
    fprintf('Error: Sigma for PF is not specified.\n');
    success = 0;
end
if length(sigma.sigma_PF) > 1
    fprintf('Warning: Sigma for PF is assigned multiple values. The first value will be used.\n');    
    sigma.sigma_PF = sigma.sigma_PF(1);
    nowarning = 0;
end

% for PT
if length(measure.PT) ~= length(idx.idx_zPT)
    fprintf('Warning: measurement vector and index vector for PT do not have the same length. The longer vector will be truncated.\n');
    min_len = min(length(measure.PT), length(idx.idx_zPT));
    measure.PT  = measure.PT(1:min_len);
    idx.idx_zPT = idx.idx_zPT(1:min_len);
    nowarning = 0;
end
if ~isempty(idx.idx_zPT) && length(sigma.sigma_PT) <= 0 % no sigma defined
    fprintf('Error: Sigma for PT is not specified.\n');
    success = 0;
end
if length(sigma.sigma_PT) > 1
    fprintf('Warning: Sigma for PT is assigned multiple values. The first value will be used.\n');    
    sigma.sigma_PT = sigma.sigma_PT(1);
    nowarning = 0;
end

% for PG
if length(measure.PG) ~= length(idx.idx_zPG)
    fprintf('Warning: measurement vector and index vector for PG do not have the same length. The longer vector will be truncated.\n');
    min_len = min(length(measure.PG), length(idx.idx_zPG));
    measure.PG  = measure.PG(1:min_len);
    idx.idx_zPG = idx.idx_zPG(1:min_len);
    nowarning = 0;
end
if ~isempty(idx.idx_zPG) && length(sigma.sigma_PG) <= 0 % no sigma defined
    fprintf('Error: Sigma for PG is not specified.\n');
    success = 0;
end
if length(sigma.sigma_PG) > 1
    fprintf('Warning: Sigma for PG is assigned multiple values. The first value will be used.\n');    
    sigma.sigma_PG = sigma.sigma_PG(1);
    nowarning = 0;
end

% for Va
if length(measure.Va) ~= length(idx.idx_zVa)
    fprintf('Warning: measurement vector and index vector for Va do not have the same length. The longer vector will be truncated.\n');
    min_len = min(length(measure.Va), length(idx.idx_zVa));
    measure.Va  = measure.Va(1:min_len);
    idx.idx_zVa = idx.idx_zVa(1:min_len);
    nowarning = 0;
end
if ~isempty(idx.idx_zVa) && length(sigma.sigma_Va) <= 0 % no sigma defined
    fprintf('Error: Sigma for Va is not specified.\n');
    success = 0;
end
if length(sigma.sigma_Va) > 1
    fprintf('Warning: Sigma for Va is assigned multiple values. The first value will be used.\n');    
    sigma.sigma_Va = sigma.sigma_Va(1);
    nowarning = 0;
end

% for QF
if length(measure.QF) ~= length(idx.idx_zQF)
    fprintf('Warning: measurement vector and index vector for QF do not have the same length. The longer vector will be truncated.\n');
    min_len = min(length(measure.QF), length(idx.idx_zQF));
    measure.QF  = measure.QF(1:min_len);
    idx.idx_zQF = idx.idx_zQF(1:min_len);
    nowarning = 0;
end
if ~isempty(idx.idx_zQF) && length(sigma.sigma_QF) <= 0 % no sigma defined
    fprintf('Error: Sigma for QF is not specified.\n');
    success = 0;
end
if length(sigma.sigma_QF) > 1
    fprintf('Warning: Sigma for QF is assigned multiple values. The first value will be used.\n');    
    sigma.sigma_QF = sigma.sigma_QF(1);
    nowarning = 0;
end

% for QT
if length(measure.QT) ~= length(idx.idx_zQT)
    fprintf('Warning: measurement vector and index vector for QT do not have the same length. The longer vector will be truncated.\n');
    min_len = min(length(measure.QT), length(idx.idx_zQT));
    measure.QT  = measure.QT(1:min_len);
    idx.idx_zQT = idx.idx_zQT(1:min_len);
    nowarning = 0;
end
if ~isempty(idx.idx_zQT) && length(sigma.sigma_QT) <= 0 % no sigma defined
    fprintf('Error: Sigma for QT is not specified.\n');
    success = 0;
end
if length(sigma.sigma_QT) > 1
    fprintf('Warning: Sigma for QT is assigned multiple values. The first value will be used.\n');    
    sigma.sigma_QT = sigma.sigma_QT(1);
    nowarning = 0;
end

% for QG
if length(measure.QG) ~= length(idx.idx_zQG)
    fprintf('Warning: measurement vector and index vector for QG do not have the same length. The longer vector will be truncated.\n');
    min_len = min(length(measure.QG), length(idx.idx_zQG));
    measure.QG  = measure.QG(1:min_len);
    idx.idx_zQG = idx.idx_zQG(1:min_len);
    nowarning = 0;
end
if ~isempty(idx.idx_zQG) && length(sigma.sigma_QG) <= 0 % no sigma defined
    fprintf('Error: Sigma for QG is not specified.\n');
    success = 0;
end
if length(sigma.sigma_QG) > 1
    fprintf('Warning: Sigma for QG is assigned multiple values. The first value will be used.\n');    
    sigma.sigma_QG = sigma.sigma_QG(1);
    nowarning = 0;
end

% for Vm
if length(measure.Vm) ~= length(idx.idx_zVm)
    fprintf('Warning: measurement vector and index vector for Vm do not have the same length. The longer vector will be truncated.\n');
    min_len = min(length(measure.Vm), length(idx.idx_zVm));
    measure.Vm  = measure.Vm(1:min_len);
    idx.idx_zVm = idx.idx_zVm(1:min_len);
    nowarning = 0;
end
if ~isempty(idx.idx_zVm) && length(sigma.sigma_Vm) <= 0 % no sigma defined
    fprintf('Error: Sigma for Vm is not specified.\n');
    success = 0;
end
if length(sigma.sigma_Vm) > 1
    fprintf('Warning: Sigma for Vm is assigned multiple values. The first value will be used.\n');    
    sigma.sigma_Vm = sigma.sigma_Vm(1);
    nowarning = 0;
end

% pause when warnings are present
if success && ~nowarning
    fprintf('Press any key to continue...\n');
    pause;
end

%% check if total number of measurements is no less than total number of
%% variables to be estimated
allMeasure = [
                measure.PF
                measure.PT
                measure.PG
                measure.Va
                measure.QF
                measure.QT
                measure.QG
                measure.Vm    
                ];
if length(allMeasure) < 2*(nbus - 1)
    fprintf('Error: There are less measurements (%d) than number of variables to be estimated (%d).\n', length(allMeasure), 2*(nbus - 1));
    success = 0;
end
