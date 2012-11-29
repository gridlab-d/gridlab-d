function test_se_14bus
%TEST_SE_14BUS	Test state estimation on IEEE 14-bus system
%	created by Rui Bo on Jan 6, 2010

%   MATPOWER
%   $Id: test_se_14bus.m,v 1.3 2010/04/26 19:45:26 ray Exp $
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

%%------------------------------------------------------
% using data for IEEE 14-bus system
%%------------------------------------------------------
% NOTE: 
% 1) all eight members of 'idx', 'measure' and 'sigma' must be
% defined. They should be null vectors([]) if they do not have data
% 2) all data used in this code are for testing purpose only
%% which measurements are available
idx.idx_zPF = [1;3;8;9;10;13;15;16;17;19];
idx.idx_zPT = [4;5;7;11];
idx.idx_zPG = [1;2;3;4;5];
idx.idx_zVa = [];
idx.idx_zQF = [1;3;8;9;10;13;15;19];
idx.idx_zQT = [4;5;7;11];
idx.idx_zQG = [1;2];
idx.idx_zVm = [2;3;6;8;10;14];

%% specify measurements
measure.PF = [1.5708;0.734;0.2707;0.1546;0.4589;0.1834;0.2707;0.0523;0.0943;0.0188];
measure.PT = [-0.5427;-0.4081;0.6006;-0.0816];
measure.PG = [2.32;0.4;0;0;0];
measure.Va = [];
measure.QF = [-0.1748;0.0594;-0.154;-0.0264;-0.2084;0.0998;0.148;0.0141];
measure.QT = [0.0213;-0.0193;-0.1006;-0.0864];
measure.QG = [-0.169;0.424];
measure.Vm = [1;1;1;1;1;1];

%% specify measurement variances
sigma.sigma_PF = 0.02;
sigma.sigma_PT = 0.02;
sigma.sigma_PG = 0.015;
sigma.sigma_Va = [];
sigma.sigma_QF = 0.02;
sigma.sigma_QT = 0.02;
sigma.sigma_QG = 0.015;
sigma.sigma_Vm = 0.01;

%% check input data integrity
nbus = 14;
[success, measure, idx, sigma] = checkDataIntegrity(measure, idx, sigma, nbus);
if ~success
    error('State Estimation input data are not complete or sufficient!');
end
    
%% run state estimation
casename = 'case14.m';
type_initialguess = 2; % flat start
[baseMVA, bus, gen, branch, success, et, z, z_est, error_sqrsum] = run_se(casename, measure, idx, sigma, type_initialguess);
