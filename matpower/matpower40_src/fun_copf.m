function [f, g] = fun_copf(x, om, Ybus, Yf, Yt, Afeq, bfeq, Af, bf, mpopt, il)
%------------------------------  deprecated  ------------------------------
%   OPF solvers based on CONSTR & LPCONSTR to be removed in future version.
%--------------------------------------------------------------------------
%FUN_COPF  Evaluates objective function and constraints for OPF.
%   [F, G] = FUN_COPF(X, OM, YBUS, YF, YT, AFEQ, BFEQ, AF, BF, MPOPT)
%   [F, G] = FUN_COPF(X, OM, YBUS, YF, YT, AFEQ, BFEQ, AF, BF, MPOPT, IL)
%
%   Objective and constraint evaluation function for AC optimal power
%   flow, suitable for use with CONSTR.
%
%   Inputs:
%     X : optimization vector
%     OM : OPF model object
%     YBUS : bus admittance matrix
%     YF : admittance matrix for "from" end of constrained branches
%     YT : admittance matrix for "to" end of constrained branches
%     AFEQ : sparse A matrix for linear equality constraints
%     BFEQ : right hand side vector for linear equality constraints
%     AF : sparse A matrix for linear inequality constraints
%     BF : right hand side vector for linear inequality constraints
%     MPOPF : MATPOWER options vector
%     IL : (optional) vector of branch indices corresponding to
%          branches with flow limits (all others are assumed to be
%          unconstrained). The default is [1:nl] (all branches).
%          YF and YT contain only the rows corresponding to IL.
%
%   Outputs:
%     F   : value of objective function
%     G  : vector of constraint values, in the following order
%             nonlinear equality (power balance)
%             linear equality
%             nonlinear inequality (flow limits)
%             linear inequality
%          The flow limits (limit^2 - flow^2) can be apparent power
%          real power or current, depending on value of OPF_FLOW_LIM
%          in MPOPT (only for constrained lines)

%   MATPOWER
%   $Id: fun_copf.m,v 1.17 2010/06/09 14:56:58 ray Exp $
%   by Carlos E. Murillo-Sanchez, PSERC Cornell & Universidad Autonoma de Manizales
%   and Ray Zimmerman, PSERC Cornell
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

%%----- initialize -----
%% define named indices into data matrices
[GEN_BUS, PG, QG, QMAX, QMIN, VG, MBASE, GEN_STATUS, PMAX, PMIN, ...
    MU_PMAX, MU_PMIN, MU_QMAX, MU_QMIN, PC1, PC2, QC1MIN, QC1MAX, ...
    QC2MIN, QC2MAX, RAMP_AGC, RAMP_10, RAMP_30, RAMP_Q, APF] = idx_gen;
[F_BUS, T_BUS, BR_R, BR_X, BR_B, RATE_A, RATE_B, RATE_C, ...
    TAP, SHIFT, BR_STATUS, PF, QF, PT, QT, MU_SF, MU_ST, ...
    ANGMIN, ANGMAX, MU_ANGMIN, MU_ANGMAX] = idx_brch;
[PW_LINEAR, POLYNOMIAL, MODEL, STARTUP, SHUTDOWN, NCOST, COST] = idx_cost;

%% default args
if nargin < 11
    il = [];
end

%% unpack data
mpc = get_mpc(om);
[baseMVA, bus, gen, branch, gencost] = ...
    deal(mpc.baseMVA, mpc.bus, mpc.gen, mpc.branch, mpc.gencost);
cp = get_cost_params(om);
[N, Cw, H, dd, rh, kk, mm] = deal(cp.N, cp.Cw, cp.H, cp.dd, ...
                                    cp.rh, cp.kk, cp.mm);
vv = get_idx(om);

%% problem dimensions
ny = getN(om, 'var', 'y');  %% number of piece-wise linear costs
nxyz = length(x);           %% total number of control vars of all types

%% set default constrained lines
if isempty(il)
    nl = size(branch, 1);   %% number of branches
    il = (1:nl);            %% all lines have limits by default
end

%% grab Pg & Qg
Pg = x(vv.i1.Pg:vv.iN.Pg);  %% active generation in p.u.
Qg = x(vv.i1.Qg:vv.iN.Qg);  %% reactive generation in p.u.

%% put Pg & Qg back in gen
gen(:, PG) = Pg * baseMVA;  %% active generation in MW
gen(:, QG) = Qg * baseMVA;  %% reactive generation in MVAr

%%----- evaluate objective function -----
%% polynomial cost of P and Q
% use totcost only on polynomial cost; in the minimization problem
% formulation, pwl cost is the sum of the y variables.
ipol = find(gencost(:, MODEL) == POLYNOMIAL);   %% poly MW and MVAr costs
xx = [ gen(:, PG); gen(:, QG)];
if ~isempty(ipol)
  f = sum( totcost(gencost(ipol, :), xx(ipol)) );  %% cost of poly P or Q
else
  f = 0;
end

%% piecewise linear cost of P and Q
if ny > 0
  ccost = full(sparse(ones(1,ny), vv.i1.y:vv.iN.y, ones(1,ny), 1, nxyz));
  f = f + ccost * x;
end

%% generalized cost term
if ~isempty(N)
    nw = size(N, 1);
    r = N * x - rh;                 %% Nx - rhat
    iLT = find(r < -kk);            %% below dead zone
    iEQ = find(r == 0 & kk == 0);   %% dead zone doesn't exist
    iGT = find(r > kk);             %% above dead zone
    iND = [iLT; iEQ; iGT];          %% rows that are Not in the Dead region
    iL = find(dd == 1);             %% rows using linear function
    iQ = find(dd == 2);             %% rows using quadratic function
    LL = sparse(iL, iL, 1, nw, nw);
    QQ = sparse(iQ, iQ, 1, nw, nw);
    kbar = sparse(iND, iND, [   ones(length(iLT), 1);
                                zeros(length(iEQ), 1);
                                -ones(length(iGT), 1)], nw, nw) * kk;
    rr = r + kbar;                  %% apply non-dead zone shift
    M = sparse(iND, iND, mm(iND), nw, nw);  %% dead zone or scale
    diagrr = sparse(1:nw, 1:nw, rr, nw, nw);
    
    %% linear rows multiplied by rr(i), quadratic rows by rr(i)^2
    w = M * (LL + QQ * diagrr) * rr;

    f = f + (w' * H * w) / 2 + Cw' * w;
end

%% ----- evaluate constraints -----
%% reconstruct V
Va = x(vv.i1.Va:vv.iN.Va);
Vm = x(vv.i1.Vm:vv.iN.Vm);
V = Vm .* exp(1j * Va);

%% rebuild Sbus
Sbus = makeSbus(baseMVA, bus, gen); %% net injected power in p.u.

%% evaluate power flow equations
mis = V .* conj(Ybus * V) - Sbus;

%%----- evaluate constraint function values -----
%% first, the equality constraints (power flow)
geq = [ real(mis);              %% active power mismatch for all buses
        imag(mis) ];            %% reactive power mismatch for all buses

%% then, the inequality constraints (branch flow limits)
flow_max = (branch(il, RATE_A)/baseMVA).^2;
flow_max(flow_max == 0) = Inf;
if mpopt(24) == 2       %% current magnitude limit, |I|
  If = Yf * V;
  It = Yt * V;
  gineq = [ If .* conj(If) - flow_max;    %% branch current limits (from bus)
            It .* conj(It) - flow_max ];  %% branch current limits (to bus)
else
    %% compute branch power flows
    Sf = V(branch(il, F_BUS)) .* conj(Yf * V);   %% complex power injected at "from" bus (p.u.)
    St = V(branch(il, T_BUS)) .* conj(Yt * V);   %% complex power injected at "to" bus (p.u.)
    if mpopt(24) == 1   %% active power limit, P (Pan Wei)
      gineq = [ real(Sf).^2 - flow_max;   %% branch real power limits (from bus)
                real(St).^2 - flow_max ]; %% branch real power limits (to bus)
    else                %% apparent power limit, |S|
      gineq = [ Sf .* conj(Sf) - flow_max;    %% branch apparent power limits (from bus)
                St .* conj(St) - flow_max ];  %% branch apparent power limits (to bus)
    end
end

g = [geq; Afeq * x - bfeq; gineq; Af * x - bf];
