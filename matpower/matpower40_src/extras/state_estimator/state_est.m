function [V, converged, i] = state_est(branch, Ybus, Yf, Yt, Sbus, V0, ref, pv, pq, mpopt)
%STATE_EST  Solves a state estimation problem.
%   [V, CONVERGED, I] = STATE_EST(BRANCH, YBUS, YF, YT, SBUS, ...
%                                   V0, REF, PV, PQ, MPOPT)
%   State estimator (under construction) based on code from James S. Thorp.

%   MATPOWER
%   $Id: state_est.m,v 1.11 2010/04/26 19:45:26 ray Exp $
%   by Ray Zimmerman, PSERC Cornell
%   based on code by James S. Thorp, June 2004
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

[F_BUS, T_BUS, BR_R, BR_X, BR_B, RATE_A, RATE_B, RATE_C, ...
    TAP, SHIFT, BR_STATUS, PF, QF, PT, QT, MU_SF, MU_ST, ...
    ANGMIN, ANGMAX, MU_ANGMIN, MU_ANGMAX] = idx_brch;

%% default arguments
if nargin < 10
    mpopt = mpoption;
end

%% options
tol     = mpopt(2);
max_it  = mpopt(3);
verbose = mpopt(31);

%% initialize
converged = 0;
i = 0;
nb = length(V0);
nbr = size(Yf, 1);
nref = [pv;pq];             %% indices of all non-reference buses
f = branch(:, F_BUS);       %% list of "from" buses
t = branch(:, T_BUS);       %% list of "to" buses

%%-----  evaluate Hessian  -----
[dSbus_dVm, dSbus_dVa] = dSbus_dV(Ybus, V0);
[dSf_dVa, dSf_dVm, dSt_dVa, dSt_dVm, Sf, St] = dSbr_dV(branch, Yf, Yt, V0);
H = [
    real(dSf_dVa)   real(dSf_dVm);
    real(dSt_dVa)   real(dSt_dVm);
    real(dSbus_dVa) real(dSbus_dVm);
    speye(nb)       sparse(nb,nb);
    imag(dSf_dVa)   imag(dSf_dVm);
    imag(dSt_dVa)   imag(dSt_dVm);
    imag(dSbus_dVa) imag(dSbus_dVm);
    sparse(nb,nb)   speye(nb);
];

%% true measurement
z = [
    real(Sf);
    real(St);
    real(Sbus);
    angle(V0);
    imag(Sf);
    imag(St);
    imag(Sbus);
    abs(V0);
];

%% create inverse of covariance matrix with all measurements
fullscale = 30;
sigma = [
    0.02 * abs(Sf)      + 0.0052 * fullscale * ones(nbr,1);
    0.02 * abs(St)      + 0.0052 * fullscale * ones(nbr,1);
    0.02 * abs(Sbus)    + 0.0052 * fullscale * ones(nb,1);
    0.2 * pi/180 * 3*ones(nb,1);
    0.02 * abs(Sf)      + 0.0052 * fullscale * ones(nbr,1);
    0.02 * abs(St)      + 0.0052 * fullscale * ones(nbr,1);
    0.02 * abs(Sbus)    + 0.0052 * fullscale * ones(nb,1);
    0.02 * abs(V0)      + 0.0052 * 1.1 * ones(nb,1);
] ./ 3;
ns = length(sigma);
W = sparse(1:ns, 1:ns ,  sigma .^ 2, ns, ns );
WInv = sparse(1:ns, 1:ns ,  1 ./ sigma .^ 2, ns, ns );

%% covariance of measurement residual
%R = H * inv( H' * WInv * H ) * H';

%% measurement with error
err = normrnd( zeros(size(sigma)), sigma );
% err = zeros(size(z));
% save err err 
% load err
% err(10) = 900 * W(10,10);     %% create a bad measurement
z = z + err;
    
%% use flat start for intial estimate
V = ones(nb,1);

%% compute estimated measurement
Sfe = V(f) .* conj(Yf * V);
Ste = V(t) .* conj(Yt * V);
Sbuse = V .* conj(Ybus * V);
z_est = [
    real(Sfe);
    real(Ste);
    real(Sbuse);
    angle(V);
    imag(Sfe);
    imag(Ste);
    imag(Sbuse);
    abs(V);
];

%% measurement residual       
delz = z - z_est;
normF = delz' * WInv * delz;  
chusqu = err' * WInv * err;  
     
%% check tolerance
if verbose > 1
    fprintf('\n it     norm( F )       step size');
    fprintf('\n----  --------------  --------------');
    fprintf('\n%3d    %10.3e      %10.3e', i, normF, 0);
end
if normF < tol
    converged = 1;
    if verbose > 1
        fprintf('\nConverged!\n');
    end
end

%% index vector for measurements that are to be used
%%%%%% NOTE: This is specific to the 30-bus system   %%%%%%
%%%%%%       where bus 1 is the reference bus which  %%%%%%
%%%%%%       is connected to branches 1 and 2        %%%%%%
vv=[(3:nbr), ...                    %% all but 1st two Pf
    (nbr+1:2*nbr), ...              %% all Pt
    (2*nbr+2:2*nbr+nb), ...         %% all but 1st Pbus
    (2*nbr+nb+2:2*nbr+2*nb), ...    %% all but 1st Va
    (2*nbr+2*nb+3:3*nbr+2*nb), ...  %% all but 1st two Qf
    (3*nbr+2*nb+1:4*nbr+2*nb), ...  %% all Qt
    (4*nbr+2*nb+2:4*nbr+3*nb), ...  %% all but 1st Qbus
    (4*nbr+3*nb+2:4*nbr+4*nb)]';    %% all but 1st Vm
%% index vector for state variables to be updated
ww = [ nref; nb+nref ];

%% bad data loop
one_at_a_time = 1; max_it_bad_data = 50;
% one_at_a_time = 0; max_it_bad_data = 5;
ibd = 1;
while (~converged && ibd <= max_it_bad_data)
    nm = length(vv);
    baddata = 0;

    %% find reduced Hessian, covariance matrix, measurements
    HH = H(vv,ww);
    WWInv = WInv(vv,vv);
    ddelz = delz(vv);
    VVa = angle(V(nref));
    VVm = abs(V(nref));
    
%   B0 = WWInv * (err(vv) .^ 2);
%   B00 = WWInv * (ddelz .^ 2);
%   [maxB0,i_maxB0] = max(B0)
%   [maxB00,i_maxB00] = max(B00)
    
    %%-----  do Newton iterations  -----
    max_it = 100;
    i = 0;
    while (~converged && i < max_it)
        %% update iteration counter
        i = i + 1;
        
        %% compute update step
        F = HH' * WWInv * ddelz;
        J = HH' * WWInv * HH;
        dx = (J \ F);
        
        %% update voltage
        VVa = VVa + dx(1:nb-1);
        VVm = VVm + dx(nb:2*nb-2);
        V(nref) = VVm .* exp(1j * VVa);

        %% compute estimated measurement
        Sfe = V(f) .* conj(Yf * V);
        Ste = V(t) .* conj(Yt * V);
        Sbuse = V .* conj(Ybus * V);
        z_est = [
            real(Sfe);
            real(Ste);
            real(Sbuse);
            angle(V);
            imag(Sfe);
            imag(Ste);
            imag(Sbuse);
            abs(V);
        ];
        
        %% measurement residual       
        delz = z - z_est;
        ddelz = delz(vv);
        normF = ddelz' * WWInv * ddelz;  

        %% check for convergence
        step = dx' * dx;
        if verbose > 1
            fprintf('\n%3d    %10.3e      %10.3e', i, normF, step);
        end
        if (step < tol) 
            converged = 1;
            if verbose
                fprintf('\nState estimator converged in %d iterations.\n', i);
            end
        end
    end
    if verbose
        if ~converged
            fprintf('\nState estimator did not converge in %d iterations.\n', i);
        end
    end
    
    %%-----  Chi squared test for bad data and bad data rejection  -----
    B = zeros(nm,1);
    bad_threshold = 6.25;       %% the threshold for bad data = sigma squared
    RR = inv(WWInv) - 0.95 * HH * inv(HH' * WWInv * HH) * HH';
%   RI = inv( inv(WWInv) - HH * inv(HH' * WWInv * HH) * HH' );
%   find(eig(full(inv(WWInv) - HH * inv(HH' * WWInv * HH) * HH')) < 0)
%   chi = ddelz' * RR * ddelz
    rr = diag(RR);

    B = ddelz .^ 2 ./ rr;
    [maxB,i_maxB] = max(B);
    if one_at_a_time
        if maxB >= bad_threshold
            rejected = i_maxB;
        else
            rejected = [];
        end
    else
        rejected = find( B >= bad_threshold );
    end
    if length(rejected)
        baddata = 1;
        converged = 0;
        if verbose
            fprintf('\nRejecting %d measurement(s) as bad data:\n', length(rejected));
            fprintf('\tindex\t      B\n');
            fprintf('\t-----\t-------------\n');
            fprintf('\t%4d\t%10.2f\n', [ vv(rejected), B(rejected) ]' );
        end
        
        %% update measurement index vector
        k = find( B < bad_threshold );
        vv = vv(k);
        nm = length(vv);
    end

    if (baddata == 0) 
        converged = 1;
        if verbose
            fprintf('\nNo remaining bad data, after discarding data %d time(s).\n', ibd-1);
            fprintf('Largest value of B = %.2f\n', maxB);
        end
    end
    ibd = ibd + 1;
end
