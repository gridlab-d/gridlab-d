function [V, lambda, success, iterNum] = cpf_correctVoltage(baseMVA, bus, gen, Ybus, V_predicted, lambda_predicted, initQPratio, loadvarloc)
%CPF_CORRECTVOLTAGE  Do correction for predicted voltage in cpf.
%   [INPUT PARAMETERS]
%   loadvarloc: (in internal bus numbering)
%   created by Rui Bo on 2007/11/12

%   MATPOWER
%   $Id: cpf_correctVoltage.m,v 1.4 2010/04/26 19:45:26 ray Exp $
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

%% define named indices into bus, gen, branch matrices
[PQ, PV, REF, NONE, BUS_I, BUS_TYPE, PD, QD, GS, BS, BUS_AREA, VM, ...
    VA, BASE_KV, ZONE, VMAX, VMIN, LAM_P, LAM_Q, MU_VMAX, MU_VMIN] = idx_bus;

%% get bus index lists of each type of bus
[ref, pv, pq] = bustypes(bus, gen);

%% set load as lambda indicates
lambda = lambda_predicted;
bus(loadvarloc, PD) = lambda*baseMVA;
bus(loadvarloc, QD) = lambda*baseMVA*initQPratio;

%% compute complex bus power injections (generation - load)
SbusInj = makeSbus(baseMVA, bus, gen);

%% prepare initial guess
V0 = V_predicted; % use predicted voltage to set the initial guess

%% run power flow to get solution of the current point
mpopt = mpoption('VERBOSE', 0);
[V, success, iterNum] = newtonpf(Ybus, SbusInj, V0, ref, pv, pq, mpopt); %% run NR's power flow solver
