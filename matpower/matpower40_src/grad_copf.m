function [df, dg, d2f] = grad_copf(x, om, Ybus, Yf, Yt, Afeq, bfeq, Af, bf, mpopt, il)
%------------------------------  deprecated  ------------------------------
%   OPF solvers based on CONSTR & LPCONSTR to be removed in future version.
%--------------------------------------------------------------------------
%GRAD_COPF  Evaluates gradient of objective function and constraints for OPF.
%   [DF, DG, D2F] = GRAD_COPF(X, OM, YBUS, YF, YT, AFEQ, BFEQ, AF, BF, MPOPT)
%   [DF, DG, D2F] = GRAD_COPF(X, OM, YBUS, YF, YT, AFEQ, BFEQ, AF, BF, MPOPT, IL)
%
%   Gradient (and Hessian) evaluation routine for AC optimal power flow costs
%   and constraints, suitable for use with CONSTR.
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
%     DF  : gradient of objective function (column vector)
%     DG  : constraint gradients, column j is gradient of G(j)
%           see FUN_COPF for order of elements in G
%     D2F : (optional) Hessian of objective function (sparse matrix)
%
%   See also FUN_COPF.

%   MATPOWER
%   $Id: grad_copf.m,v 1.19 2010/04/27 18:55:02 ray Exp $
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
nb = size(bus, 1);          %% number of buses
nl = size(branch, 1);       %% number of branches
ng = size(gen, 1);          %% number of dispatchable injections
ny = getN(om, 'var', 'y');  %% number of piece-wise linear costs
nxyz = length(x);           %% total number of control vars of all types

%% set default constrained lines
if isempty(il)
    il = (1:nl);            %% all lines have limits by default
end
nl2 = length(il);           %% number of constrained lines

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
else
  ccost = zeros(1, nxyz);
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

%%----- evaluate cost gradient -----
%% index ranges
iPg = vv.i1.Pg:vv.iN.Pg;
iQg = vv.i1.Qg:vv.iN.Qg;

%% polynomial cost of P and Q
df_dPgQg = zeros(2*ng, 1);          %% w.r.t p.u. Pg and Qg
df_dPgQg(ipol) = baseMVA * polycost(gencost(ipol, :), xx(ipol), 1);
df = zeros(nxyz, 1);
df(iPg) = df_dPgQg(1:ng);
df(iQg) = df_dPgQg((1:ng) + ng);

%% piecewise linear cost of P and Q
df = df + ccost';  % As in MINOS, the linear cost row is additive wrt
                   % any nonlinear cost.

%% generalized cost term
if ~isempty(N)
  HwC = H * w + Cw;
  AA = N' * M * (LL + 2 * QQ * diagrr);
  df = df + AA * HwC;
  
  %% numerical check
  if 0    %% 1 to check, 0 to skip check
    ddff = zeros(size(df));
    step = 1e-7;
    tol  = 1e-3;
    for k = 1:length(x)
      xx = x;
      xx(k) = xx(k) + step;
      ddff(k) = (fun_copf(xx, om, Ybus, Yf, Yt, Afeq, bfeq, Af, bf, mpopt, il) - f) / step;
    end
    if max(abs(ddff - df)) > tol
      idx = find(abs(ddff - df) == max(abs(ddff - df)));
      fprintf('\nMismatch in gradient\n');
      fprintf('idx             df(num)         df              diff\n');
      fprintf('%4d%16g%16g%16g\n', [ 1:length(df); ddff'; df'; abs(ddff - df)' ]);
      fprintf('MAX\n');
      fprintf('%4d%16g%16g%16g\n', [ idx'; ddff(idx)'; df(idx)'; abs(ddff(idx) - df(idx))' ]);
      fprintf('\n');
    end
  end     %% numeric check
end

%% ---- evaluate cost Hessian -----
if nargout > 2
  pcost = gencost(1:ng, :);
  if size(gencost, 1) > ng
      qcost = gencost(ng+1:2*ng, :);
  else
      qcost = [];
  end
  
  %% polynomial generator costs
  d2f_dPg2 = sparse(ng, 1);             %% w.r.t. p.u. Pg
  d2f_dQg2 = sparse(ng, 1);             %% w.r.t. p.u. Qg
  ipolp = find(pcost(:, MODEL) == POLYNOMIAL);
  d2f_dPg2(ipolp) = baseMVA^2 * polycost(pcost(ipolp, :), Pg(ipolp)*baseMVA, 2);
  if ~isempty(qcost)          %% Qg is not free
      ipolq = find(qcost(:, MODEL) == POLYNOMIAL);
      d2f_dQg2(ipolq) = baseMVA^2 * polycost(qcost(ipolq, :), Qg(ipolq)*baseMVA, 2);
  end
  i = (pgbas:qgend)';
  d2f = sparse(i, i, [d2f_dPg2; d2f_dQg2], nxyz, nxyz);

  %% generalized cost
  if ~isempty(N)
      d2f = d2f + AA * H * AA' + 2 * N' * M * QQ * sparse(1:nw, 1:nw, HwC, nw, nw) * N;
  end
end

%%----- evaluate partials of constraints -----
%% reconstruct V
Va = x(vv.i1.Va:vv.iN.Va);
Vm = x(vv.i1.Vm:vv.iN.Vm);
V = Vm .* exp(1j * Va);

%% compute partials of injected bus powers
[dSbus_dVm, dSbus_dVa] = dSbus_dV(Ybus, V);           %% w.r.t. V
neg_Cg = sparse(gen(:, GEN_BUS), 1:ng, -1, nb, ng);   %% Pbus w.r.t. Pg
                                                      %% Qbus w.r.t. Qg

%% compute partials of Flows w.r.t. V
if mpopt(24) == 2     %% current
  [dFf_dVa, dFf_dVm, dFt_dVa, dFt_dVm, Ff, Ft] = dIbr_dV(branch(il,:), Yf, Yt, V);
else                  %% power
  [dFf_dVa, dFf_dVm, dFt_dVa, dFt_dVm, Ff, Ft] = dSbr_dV(branch(il,:), Yf, Yt, V);
end
if mpopt(24) == 1     %% real part of flow (active power)
  dFf_dVa = real(dFf_dVa);
  dFf_dVm = real(dFf_dVm);
  dFt_dVa = real(dFt_dVa);
  dFt_dVm = real(dFt_dVm);
  Ff = real(Ff);
  Ft = real(Ft);
end

%% squared magnitude of flow (of complex power or current, or real power)
[df_dVa, df_dVm, dt_dVa, dt_dVm] = ...
        dAbr_dV(dFf_dVa, dFf_dVm, dFt_dVa, dFt_dVm, Ff, Ft);

%% index ranges
iVa = vv.i1.Va:vv.iN.Va;
iVm = vv.i1.Vm:vv.iN.Vm;
iPg = vv.i1.Pg:vv.iN.Pg;
iQg = vv.i1.Qg:vv.iN.Qg;
nleq = size(Afeq, 1);
nliq = size(Af, 1);

%% construct Jacobian of constraints
dg = sparse(nxyz, 2*nb+2*nl2+nleq+nliq);
%% equality constraints
dg([iVa iVm], 1:2*nb) = [
  real(dSbus_dVa), real(dSbus_dVm);   %% P mismatch w.r.t Va, Vm
  imag(dSbus_dVa), imag(dSbus_dVm);   %% Q mismatch w.r.t Va, Vm
 ]';
dg(iPg, 1:nb)        = neg_Cg';         %% P mismatch w.r.t Pg
dg(iQg, (1:nb) + nb) = neg_Cg';         %% Q mismatch w.r.t Qg
dg(:, (1:nleq)+2*nb) = Afeq';           %% linear equalities
%% inequality constraints
dg([iVa iVm], (1:2*nl2)+2*nb+nleq) = [
  df_dVa, df_dVm;                       %% "from" flow limit
  dt_dVa, dt_dVm;                       %% "to" flow limit
]';
dg(:, (1:nliq)+2*nb+2*nl2+nleq) = Af';   %% linear inequalities

%% use non-sparse matrices
dg = full(dg);
