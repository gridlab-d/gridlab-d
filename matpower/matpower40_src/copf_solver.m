function [results, success, raw] = copf_solver(om, mpopt)
%------------------------------  deprecated  ------------------------------
%   OPF solver based on CONSTR to be removed in a future version.
%--------------------------------------------------------------------------
%COPF_SOLVER  Solves AC optimal power flow using CONSTR (Opt Tbx 1.x & 2.x).
%
%   [RESULTS, SUCCESS, RAW] = COPF_SOLVER(OM, MPOPT)
%
%   Inputs are an OPF model object and a MATPOWER options vector.
%
%   Outputs are a RESULTS struct, SUCCESS flag and RAW output struct.
%
%   RESULTS is a MATPOWER case struct (mpc) with the usual baseMVA, bus
%   branch, gen, gencost fields, along with the following additional
%   fields:
%       .order      see 'help ext2int' for details of this field
%       .x          final value of optimization variables (internal order)
%       .f          final objective function value
%       .mu         shadow prices on ...
%           .var
%               .l  lower bounds on variables
%               .u  upper bounds on variables
%           .nln
%               .l  lower bounds on nonlinear constraints
%               .u  upper bounds on nonlinear constraints
%           .lin
%               .l  lower bounds on linear constraints
%               .u  upper bounds on linear constraints
%
%   SUCCESS     1 if solver converged successfully, 0 otherwise
%
%   RAW         raw output in form returned by MINOS
%       .xr     final value of optimization variables
%       .pimul  constraint multipliers
%       .info   solver specific termination code
%
%   See also OPF, CONSTR, FUN_COPF, GRAD_COPF.

%   MATPOWER
%   $Id: copf_solver.m 4738 2014-07-03 00:55:39Z dchassin $
%   by Ray Zimmerman, PSERC Cornell
%   and Carlos E. Murillo-Sanchez, PSERC Cornell & Universidad Autonoma de Manizales
%   Copyright (c) 2000-2010 by Power System Engineering Research Center (PSERC)
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

%%----- initialization -----
%% define named indices into data matrices
[PQ, PV, REF, NONE, BUS_I, BUS_TYPE, PD, QD, GS, BS, BUS_AREA, VM, ...
    VA, BASE_KV, ZONE, VMAX, VMIN, LAM_P, LAM_Q, MU_VMAX, MU_VMIN] = idx_bus;
[GEN_BUS, PG, QG, QMAX, QMIN, VG, MBASE, GEN_STATUS, PMAX, PMIN, ...
    MU_PMAX, MU_PMIN, MU_QMAX, MU_QMIN, PC1, PC2, QC1MIN, QC1MAX, ...
    QC2MIN, QC2MAX, RAMP_AGC, RAMP_10, RAMP_30, RAMP_Q, APF] = idx_gen;
[F_BUS, T_BUS, BR_R, BR_X, BR_B, RATE_A, RATE_B, RATE_C, ...
    TAP, SHIFT, BR_STATUS, PF, QF, PT, QT, MU_SF, MU_ST, ...
    ANGMIN, ANGMAX, MU_ANGMIN, MU_ANGMAX] = idx_brch;

%% options
verbose = mpopt(31);    %% VERBOSE

%% unpack data
mpc = get_mpc(om);
[baseMVA, bus, gen, branch] = ...
    deal(mpc.baseMVA, mpc.bus, mpc.gen, mpc.branch);
vv = get_idx(om);

%% problem dimensions
nb = size(bus, 1);          %% number of buses
nl = size(branch, 1);       %% number of branches
ny = getN(om, 'var', 'y');  %% number of piece-wise linear costs

%% bounds on optimization vars
[x0, LB, UB] = getv(om);

%% linear constraints
%% WORKAROUND: Add bounds on all vars to A, l, u
nxyz = length(x0);
om2 = om;
om2 = add_constraints(om2, 'varlims', speye(nxyz, nxyz), LB, UB);
[vv, ll, nn] = get_idx(om2);
[A, l, u] = linear_constraints(om2);

%% so, can we do anything good about lambda initialization?
if all(bus(:, LAM_P) == 0)
  bus(:, LAM_P) = (10)*ones(nb, 1);
end

%% split l <= A*x <= u into less than, equal to, greater than, and
%% doubly-bounded sets
ieq = find( abs(u-l) <= eps );          %% equality
igt = find( u >=  1e10 & l > -1e10 );   %% greater than, unbounded above
ilt = find( l <= -1e10 & u <  1e10 );   %% less than, unbounded below
ibx = find( (abs(u-l) > eps) & (u < 1e10) & (l > -1e10) );
Af  = [ A(ilt, :); -A(igt, :); A(ibx, :); -A(ibx, :) ];
bf  = [ u(ilt);   -l(igt);     u(ibx);    -l(ibx)];
Afeq = A(ieq, :);
bfeq = u(ieq);

%% build admittance matrices
[Ybus, Yf, Yt] = makeYbus(baseMVA, bus, branch);

%% find branches with flow limits
il = find(branch(:, RATE_A) ~= 0 & branch(:, RATE_A) < 1e10);
nl2 = length(il);           %% number of constrained lines

%% set mpopt defaults
mpopt(15)  = 2 * nb + length(bfeq); %% set number of equality constraints
if mpopt(19) == 0           %% CONSTR_MAX_IT
  mpopt(19) = 150 + 2*nb;
end

%% set up options for Optim Tbx's constr
otopt = foptions;           %% get default options for constr
otopt(1) = (verbose > 0);   %% set verbose flag appropriately
% otopt(9) = 1;             %% check user supplied gradients?
otopt(2)  = mpopt(17);      %% termination tolerance on 'x'
otopt(3)  = mpopt(18);      %% termination tolerance on 'F'
otopt(4)  = mpopt(16);      %% termination tolerance on constraint violation
otopt(13) = mpopt(15);      %% number of equality constraints
otopt(14) = mpopt(19);      %% maximum number of iterations

%% run optimization
[x, otopt, lambda] = constr('fun_copf', x0, otopt, [], [], 'grad_copf', ...
        om2, Ybus, Yf(il,:), Yt(il,:), Afeq, bfeq, Af, bf, mpopt, il);
%% get final objective function value & constraint values
[f, g] = feval('fun_copf', x, om2, Ybus, Yf(il,:), Yt(il,:), Afeq, bfeq, Af, bf, mpopt, il);

%% check for convergence
if otopt(10) >= otopt(14) || max(abs(g(1:otopt(13)))) > otopt(4) ...
                   || max(g((otopt(13)+1):length(g))) > otopt(4)
  success = 0;        %% did NOT converge
else
  success = 1;        %% DID converge
end
info = success;

%% update solution data
Va = x(vv.i1.Va:vv.iN.Va);
Vm = x(vv.i1.Vm:vv.iN.Vm);
Pg = x(vv.i1.Pg:vv.iN.Pg);
Qg = x(vv.i1.Qg:vv.iN.Qg);
V = Vm .* exp(1j*Va);

%%-----  calculate return values  -----
%% update voltages & generator outputs
bus(:, VA) = Va * 180/pi;
bus(:, VM) = Vm;
gen(:, PG) = Pg * baseMVA;
gen(:, QG) = Qg * baseMVA;
gen(:, VG) = Vm(gen(:, GEN_BUS));

%% compute branch flows
Sf = V(branch(:, F_BUS)) .* conj(Yf * V);  %% cplx pwr at "from" bus, p.u.
St = V(branch(:, T_BUS)) .* conj(Yt * V);  %% cplx pwr at "to" bus, p.u.
branch(:, PF) = real(Sf) * baseMVA;
branch(:, QF) = imag(Sf) * baseMVA;
branch(:, PT) = real(St) * baseMVA;
branch(:, QT) = imag(St) * baseMVA;

%% package up results
nA = length(u);
neq = length(ieq);
nlt = length(ilt);
ngt = length(igt);
nbx = length(ibx);

%% extract multipliers (lambda is ordered as Pmis, Qmis, Afeq, Sf, St, Af)
%% nonlinear constraints
inln = [(1:2*nb), (1:2*nl2) + 2*nb+neq];
kl = find(lambda(inln) < 0);
ku = find(lambda(inln) > 0);
nl_mu_l = zeros(2*(nb+nl2), 1);
nl_mu_u = zeros(2*(nb+nl2), 1);
nl_mu_l(kl) = -lambda(inln(kl));
nl_mu_u(ku) =  lambda(inln(ku));

%% linear constraints
ilin = [(1:neq)+2*nb, (1:(nlt+ngt+2*nbx)) + 2*nb+neq+2*nl2];
kl = find(lambda(ilin(1:neq)) < 0);
ku = find(lambda(ilin(1:neq)) > 0);

mu_l = zeros(nA, 1);
mu_l(ieq) = -lambda(ilin(1:neq));
mu_l(ieq(ku)) = 0;
mu_l(igt) = lambda(ilin(neq+nlt+(1:ngt)));
mu_l(ibx) = lambda(ilin(neq+nlt+ngt+nbx+(1:nbx)));

mu_u = zeros(nA, 1);
mu_u(ieq) = lambda(ilin(1:neq));
mu_u(ieq(kl)) = 0;
mu_u(ilt) = lambda(ilin(neq+(1:nlt)));
mu_u(ibx) = lambda(ilin(neq+nlt+ngt+(1:nbx)));

%% variable bounds
muLB = mu_l(ll.i1.varlims:ll.iN.varlims);
muUB = mu_u(ll.i1.varlims:ll.iN.varlims);
mu_l(ll.i1.varlims:ll.iN.varlims) = [];
mu_u(ll.i1.varlims:ll.iN.varlims) = [];

%% line constraint is actually on square of limit
%% so we must fix multipliers
muSf = zeros(nl, 1);
muSt = zeros(nl, 1);
muSf(il) = 2 * nl_mu_u((1:nl2)+2*nb    ) .* branch(il, RATE_A) / baseMVA;
muSt(il) = 2 * nl_mu_u((1:nl2)+2*nb+nl2) .* branch(il, RATE_A) / baseMVA;

%% resize mu for nonlinear constraints
nl_mu_l = [nl_mu_l(1:2*nb); zeros(2*nl, 1)];
nl_mu_u = [nl_mu_u(1:2*nb); muSf; muSt];

%% update Lagrange multipliers
bus(:, MU_VMAX)  = muUB(vv.i1.Vm:vv.iN.Vm);
bus(:, MU_VMIN)  = muLB(vv.i1.Vm:vv.iN.Vm);
gen(:, MU_PMAX)  = muUB(vv.i1.Pg:vv.iN.Pg) / baseMVA;
gen(:, MU_PMIN)  = muLB(vv.i1.Pg:vv.iN.Pg) / baseMVA;
gen(:, MU_QMAX)  = muUB(vv.i1.Qg:vv.iN.Qg) / baseMVA;
gen(:, MU_QMIN)  = muLB(vv.i1.Qg:vv.iN.Qg) / baseMVA;
bus(:, LAM_P)    = (nl_mu_u(nn.i1.Pmis:nn.iN.Pmis) - nl_mu_l(nn.i1.Pmis:nn.iN.Pmis)) / baseMVA;
bus(:, LAM_Q)    = (nl_mu_u(nn.i1.Qmis:nn.iN.Qmis) - nl_mu_l(nn.i1.Qmis:nn.iN.Qmis)) / baseMVA;
branch(:, MU_SF) = muSf / baseMVA;
branch(:, MU_ST) = muSt / baseMVA;

mu = struct( ...
  'var', struct('l', muLB, 'u', muUB), ...
  'nln', struct('l', nl_mu_l, 'u', nl_mu_u), ...
  'lin', struct('l', mu_l, 'u', mu_u) );

results = mpc;
[results.bus, results.branch, results.gen, ...
    results.om, results.x, results.mu, results.f] = ...
        deal(bus, branch, gen, om, x, mu, f);

pimul = [ ...
  results.mu.nln.l - results.mu.nln.u;
  results.mu.lin.l - results.mu.lin.u;
  -ones(ny>0, 1);
  results.mu.var.l - results.mu.var.u;
];
raw = struct('xr', x, 'pimul', pimul, 'info', info);
