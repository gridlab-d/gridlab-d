function [V, lambda, converged, iterNum] = cpf_correctLambda(baseMVA, bus, gen, Ybus, Vm_assigned, V_predicted, lambda_predicted, initQPratio, loadvarloc, ref, pv, pq)
%CPF_CORRECTLAMBDA  Correct lambda in correction step near load point.
%   function: correct lambda(ie, real power of load) in cpf correction step
%   near the nose point. Use NR's method to solve the nonlinear equations
%   [INPUT PARAMETERS]
%   loadvarloc: (in internal bus numbering)
%   created by Rui Bo on 2007/11/12

%   MATPOWER
%   $Id: cpf_correctLambda.m,v 1.4 2010/04/26 19:45:26 ray Exp $
%   by Rui Bo
%   and Ray Zimmerman, PSERC Cornell
%   Copyright (c) 1996-2010 by Power System Engineering Research Center (PSERC)
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

%% options
tol     = 1e-5; % mpopt(2);
max_it  = 100;  % mpopt(3);
verbose = 0;    % mpopt(31);

%% initialize
j = sqrt(-1);
converged = 0;
i = 0;
V = V_predicted;
lambda = lambda_predicted;
Va = angle(V);
Vm = abs(V);

%% set up indexing for updating V
npv = length(pv);
npq = length(pq);
j1 = 1;         j2 = npv;           %% j1:j2 - V angle of pv buses
j3 = j2 + 1;    j4 = j2 + npq;      %% j3:j4 - V angle of pq buses
j5 = j4 + 1;    j6 = j4 + npq;      %% j5:j6 - V mag of pq buses
j7 = j6 + 1;                        %% j7 - lambda

pv_bus = ~isempty(find(pv == loadvarloc));

%% set load as lambda indicates
bus(loadvarloc, PD) = lambda*baseMVA;
bus(loadvarloc, QD) = lambda*baseMVA*initQPratio;

%% compute complex bus power injections (generation - load)
SbusInj = makeSbus(baseMVA, bus, gen);

%% evalute F(x0)
mis = V .* conj(Ybus * V) - SbusInj;
mis = -mis; % NOTE: use reverse mismatch and correspondingly use '(-)Jacobian" obtained from dSbus_dV 
F = [   real(mis(pv));
        real(mis(pq));
        imag(mis(pq));
        abs(V(loadvarloc)) - Vm_assigned(loadvarloc);   ];
    
%% do Newton iterations
while (~converged & i < max_it)
    %% update iteration counter
    i = i + 1;

    %% evaluate Jacobian
    [dSbus_dVm, dSbus_dVa] = dSbus_dV(Ybus, V);

    j11 = real(dSbus_dVa([pv; pq], [pv; pq]));
    j12 = real(dSbus_dVm([pv; pq], pq));
    j21 = imag(dSbus_dVa(pq, [pv; pq]));
    j22 = imag(dSbus_dVm(pq, pq));

    J = [   j11 j12;
            j21 j22;    ];

    %% evaluate dDeltaP/dLambda, dDeltaQ/dLambda, dDeltaVm/dLambda,
    %% dDeltaVm/dVa, dDeltaVm/dVm
    dDeltaP_dLambda = zeros(npv+npq, 1);
    dDeltaQ_dLambda = zeros(npq, 1);
    if pv_bus % pv bus
        dDeltaP_dLambda(find(pv == loadvarloc)) = -1;                         % corresponding to deltaP
    else % pq bus
        dDeltaP_dLambda(npv + find(pq == loadvarloc)) = -1;                   % corresponding to deltaP
        dDeltaQ_dLambda(find(pq == loadvarloc)) = -initQPratio;   % corresponding to deltaQ
    end
    dDeltaVm_dLambda = zeros(1, 1);
    dDeltaVm_dVa = zeros(1, npv+npq);
    dDeltaVm_dVm = zeros(1, npq);
    dDeltaVm_dVm(1, find(pq == loadvarloc)) = -1;

    %% form augmented Jacobian
    J12 = [dDeltaP_dLambda; 
           dDeltaQ_dLambda];
    J21 = [dDeltaVm_dVa   dDeltaVm_dVm];
    J22 = dDeltaVm_dLambda;
    augJ = [    -J   J12;
                J21 J22;    ];

    %% compute update step
    dx = -(augJ \ F);

    %% update voltage. 
    % NOTE: voltage magnitude of pv buses, voltage magnitude
    % and angle of reference bus are not updated, so they keep as constants
    % (ie, the value as in the initial guess)
    if npv
        Va(pv) = Va(pv) + dx(j1:j2);
    end
    if npq
        Va(pq) = Va(pq) + dx(j3:j4);
        Vm(pq) = Vm(pq) + dx(j5:j6);
    end
    lambda = lambda + dx(j7);

    V = Vm .* exp(j * Va); % NOTE: angle is in radians in pf solver, but in degree in case data
    Vm = abs(V);            %% update Vm and Va again in case
    Va = angle(V);          %% we wrapped around with a negative Vm

    %% set load as lambda indicates
    bus(loadvarloc, PD) = lambda*baseMVA;
    bus(loadvarloc, QD) = lambda*baseMVA*initQPratio;

    %% compute complex bus power injections (generation - load)
    SbusInj = makeSbus(baseMVA, bus, gen);
    
    %% evalute F(x)
    mis = V .* conj(Ybus * V) - SbusInj;
    mis = -mis;
    F = [   real(mis(pv));
            real(mis(pq));
            imag(mis(pq));
            abs(V(loadvarloc)) - Vm_assigned(loadvarloc);   ];

    %% check for convergence
    normF = norm(F, inf);
    if verbose > 1
        fprintf('\niteration [%3d]\t\tnorm of mismatch: %10.3e', i, normF);
    end
    if normF < tol
        converged = 1;
    end
end

iterNum = i;
