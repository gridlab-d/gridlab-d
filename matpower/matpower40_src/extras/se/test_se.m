function test_se
%TEST_SE  Test state estimation.
%   created by Rui Bo on 2007/11/12

%   MATPOWER
%   $Id: test_se.m,v 1.5 2010/04/26 19:45:26 ray Exp $
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
% using data in Problem 6.7 in book 'Computational
% Methods for Electric Power Systems' by Mariesa Crow
%%------------------------------------------------------
%% which measurements are available
idx.idx_zPF = [1;2];
idx.idx_zPT = [3];
idx.idx_zPG = [1;2;3];
idx.idx_zVa = [];
idx.idx_zQF = [];
idx.idx_zQT = [];
idx.idx_zQG = [];
idx.idx_zVm = [2;3];

%% specify measurements
measure.PF = [0.12;0.10];
measure.PT = [-0.04];
measure.PG = [0.58;0.30;0.14];
measure.Va = [];
measure.QF = [];
measure.QT = [];
measure.QG = [];
measure.Vm = [1.04;0.98];

%% specify measurement variances
sigma.sigma_PF = 0.02;
sigma.sigma_PT = 0.02;
sigma.sigma_PG = 0.015;
sigma.sigma_Va = [];
sigma.sigma_QF = [];
sigma.sigma_QT = [];
sigma.sigma_QG = [];
sigma.sigma_Vm = 0.01;

%% check input data integrity
nbus = 3;
[success, measure, idx, sigma] = checkDataIntegrity(measure, idx, sigma, nbus);
if ~success
    error('State Estimation input data are not complete or sufficient!');
end

%% run state estimation
casename = 'case3bus_P6_6.m';
type_initialguess = 2; % flat start
[baseMVA, bus, gen, branch, success, et, z, z_est, error_sqrsum] = run_se(casename, measure, idx, sigma, type_initialguess);
