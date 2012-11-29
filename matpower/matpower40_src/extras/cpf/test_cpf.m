function test_cpf
%TEST_CPF  Test continuation power flow (CPF).
%   created by Rui Bo on 2007/11/12

%   MATPOWER
%   $Id: test_cpf.m,v 1.4 2010/04/26 19:45:26 ray Exp $
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

casename = 'case30';%'case6bus'; %'case30'

%% test cpf
fprintf('\n------------testing continuation power flow (CPF) solver\n');
loadvarloc = 7;%6;%7                 % bus number at which load changes
sigmaForLambda = 0.2;%0.05;          % stepsize for Lambda
sigmaForVoltage = 0.05;%0.025;       % stepsize for voltage
[max_lambda, predicted_list, corrected_list, combined_list, success, et] = cpf(casename, loadvarloc, sigmaForLambda, sigmaForVoltage);
fprintf('maximum lambda is %f\n\n', max_lambda);

%% draw PV curve
flag_combinedCurve = true;
busesToDraw = [];%[3:6];
drawPVcurves(casename, loadvarloc, corrected_list, combined_list, flag_combinedCurve, busesToDraw);

