function [V, converged, iterNum, z, z_est, error_sqrsum] = doSE(baseMVA, bus, gen, branch, Ybus, Yf, Yt, V0, ref, pv, pq, measure, idx, sigma)
%DOSE  Do state estimation.
%   created by Rui Bo on 2007/11/12

%   MATPOWER
%   $Id: doSE.m,v 1.5 2010/04/26 19:45:26 ray Exp $
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
[F_BUS, T_BUS, BR_R, BR_X, BR_B, RATE_A, RATE_B, ...
    RATE_C, TAP, SHIFT, BR_STATUS, PF, QF, PT, QT, MU_SF, MU_ST] = idx_brch;
[GEN_BUS, PG, QG, QMAX, QMIN, VG, MBASE, ...
    GEN_STATUS, PMAX, PMIN, MU_PMAX, MU_PMIN, MU_QMAX, MU_QMIN] = idx_gen;

%% options
tol     = 1e-5; % mpopt(2);
max_it  = 100;  % mpopt(3);
verbose = 0;    % mpopt(31);

%% initialize
j = sqrt(-1);
converged = 0;
i = 0;
V = V0;
Va = angle(V);
Vm = abs(V);

nb = size(Ybus, 1);
f = branch(:, F_BUS);       %% list of "from" buses
t = branch(:, T_BUS);       %% list of "to" buses

%% get non reference buses
nonref = [pv;pq];

%% form measurement vector 'z'. NOTE: all are p.u. values
z = [
    measure.PF
    measure.PT
    measure.PG
    measure.Va
    measure.QF
    measure.QT
    measure.QG
    measure.Vm
    ];

%% form measurement index vectors
idx_zPF = idx.idx_zPF;
idx_zPT = idx.idx_zPT;
idx_zPG = idx.idx_zPG;
idx_zVa = idx.idx_zVa;
idx_zQF = idx.idx_zQF;
idx_zQT = idx.idx_zQT;
idx_zQG = idx.idx_zQG;
idx_zVm = idx.idx_zVm;

%% get R inverse matrix
sigma_vector = [
    sigma.sigma_PF*ones(size(idx_zPF, 1), 1)
    sigma.sigma_PT*ones(size(idx_zPT, 1), 1)
    sigma.sigma_PG*ones(size(idx_zPG, 1), 1)
    sigma.sigma_Va*ones(size(idx_zVa, 1), 1)
    sigma.sigma_QF*ones(size(idx_zQF, 1), 1)
    sigma.sigma_QT*ones(size(idx_zQT, 1), 1)
    sigma.sigma_QG*ones(size(idx_zQG, 1), 1)
    sigma.sigma_Vm*ones(size(idx_zVm, 1), 1)
    ]; % NOTE: zero-valued elements of simga are skipped
sigma_square = sigma_vector.^2;
R_inv = diag(1./sigma_square);

%% do Newton iterations
while (~converged & i < max_it)
    %% update iteration counter
    i = i + 1;
    
    %% --- compute estimated measurement ---
    Sfe = V(f) .* conj(Yf * V);
    Ste = V(t) .* conj(Yt * V);
    %% compute net injection at generator buses
    gbus = gen(:, GEN_BUS);
    Sgbus = V(gbus) .* conj(Ybus(gbus, :) * V);
    Sgen = Sgbus * baseMVA + (bus(gbus, PD) + j*bus(gbus, QD));   %% inj S + local Sd
    Sgen = Sgen/baseMVA;
    z_est = [ % NOTE: all are p.u. values
        real(Sfe(idx_zPF));
        real(Ste(idx_zPT));
        real(Sgen(idx_zPG));
        angle(V(idx_zVa));
        imag(Sfe(idx_zQF));
        imag(Ste(idx_zQT));
        imag(Sgen(idx_zQG));
        abs(V(idx_zVm));
    ];

    %% --- get H matrix ---
    [dSbus_dVm, dSbus_dVa] = dSbus_dV(Ybus, V);
    [dSf_dVa, dSf_dVm, dSt_dVa, dSt_dVm, Sf, St] = dSbr_dV(branch, Yf, Yt, V);
%     genbus_row = findBusRowByIdx(bus, gbus);
    genbus_row = gbus;  %% rdz, this should be fine if using internal bus numbering

    %% get sub-matrix of H relating to line flow
    dPF_dVa = real(dSf_dVa); % from end
    dQF_dVa = imag(dSf_dVa);   
    dPF_dVm = real(dSf_dVm);
    dQF_dVm = imag(dSf_dVm);
    dPT_dVa = real(dSt_dVa);% to end
    dQT_dVa = imag(dSt_dVa);   
    dPT_dVm = real(dSt_dVm);
    dQT_dVm = imag(dSt_dVm);   
    %% get sub-matrix of H relating to generator output
    dPG_dVa = real(dSbus_dVa(genbus_row, :));
    dQG_dVa = imag(dSbus_dVa(genbus_row, :));
    dPG_dVm = real(dSbus_dVm(genbus_row, :));
    dQG_dVm = imag(dSbus_dVm(genbus_row, :));
    %% get sub-matrix of H relating to voltage angle
    dVa_dVa = eye(nb);
    dVa_dVm = zeros(nb, nb);
    %% get sub-matrix of H relating to voltage magnitude
    dVm_dVa = zeros(nb, nb);
    dVm_dVm = eye(nb);
    H = [
        dPF_dVa(idx_zPF, nonref)   dPF_dVm(idx_zPF, nonref);
        dPT_dVa(idx_zPT, nonref)   dPT_dVm(idx_zPT, nonref);
        dPG_dVa(idx_zPG, nonref)   dPG_dVm(idx_zPG, nonref);
        dVa_dVa(idx_zVa, nonref)   dVa_dVm(idx_zVa, nonref);
        dQF_dVa(idx_zQF, nonref)   dQF_dVm(idx_zQF, nonref);
        dQT_dVa(idx_zQT, nonref)   dQT_dVm(idx_zQT, nonref);
        dQG_dVa(idx_zQG, nonref)   dQG_dVm(idx_zQG, nonref);
        dVm_dVa(idx_zVm, nonref)   dVm_dVm(idx_zVm, nonref);
        ];
    
    %% compute update step
    J = H'*R_inv*H;
    F = H'*R_inv*(z-z_est); % evalute F(x)
    if ~isobservable(H, pv, pq)
        error('doSE: system is not observable');
    end
    dx = (J \ F);

    %% check for convergence
    normF = norm(F, inf);
    if verbose > 1
        fprintf('\niteration [%3d]\t\tnorm of mismatch: %10.3e', i, normF);
    end
    if normF < tol
        converged = 1;
    end
    
    %% update voltage
    Va(nonref) = Va(nonref) + dx(1:size(nonref, 1));
    Vm(nonref) = Vm(nonref) + dx(size(nonref, 1)+1:2*size(nonref, 1));
    V = Vm .* exp(j * Va); % NOTE: angle is in radians in pf solver, but in degree in case data
    Vm = abs(V);            %% update Vm and Va again in case
    Va = angle(V);          %% we wrapped around with a negative Vm
end

iterNum = i;

%% get weighted sum of squared errors
error_sqrsum = sum((z - z_est).^2./sigma_square);
