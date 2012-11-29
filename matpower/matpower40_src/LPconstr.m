function [x, lambda, converged] = LPconstr(FUN,x,mpopt,step0,VLB,VUB,GRADFUN,LPEQUSVR, ...
                    P1,P2,P3,P4,P5,P6,P7,P8,P9,P10,P11,P12,P13,P14,P15)
%------------------------------  deprecated  ------------------------------
%   OPF solvers based on LPCONSTR to be removed in a future version.
%--------------------------------------------------------------------------
%LPCONSTR  Finds solution of NLP problem based on successive LP.
%   The key is to set up the problem as follows:
%           Min f(xi, xo)
%       S.T. g1(xi, xo) =0
%            g2(xi, xo) =<0
%   where the number of equations in g1 is the same as the number of
%   elements in xi.
%
%   [X, LAMBDA, CONVERGED]=LPCONSTR('FUN',x, mpopt, step0 ,VLB,VUB,'GRADFUN', 
%   'LPEQUSVR', P1,P2,..) starts at x and finds a constrained minimum to
%   the function which is described in FUN (usually an M-file: FUN.M).
%   The function 'FUN' should return two arguments: a scalar value of the
%   function to be minimized, F, and a matrix of constraints, G:
%   [F,G]=FUN(X). F is minimized such that G < zeros(G).
%
%   LPCONSTR allows a vector of optional parameters to be defined. For 
%   more information type HELP LPOPTION.
%
%   VLB,VUB define a set of lower and upper bounds on the design variables, X, 
%   so that the solution is always in the range VLB <= X <= VUB.
%
%   The function 'GRADFUN' is entered which returns the partial derivatives 
%   of the function and the constraints at X:  [gf,GC] = GRADFUN(X).
%
%   The problem-dependent parameters P1,P2,... directly are passed to the 
%   functions FUN and GRADFUN: FUN(X,P1,P2,...) and GRADFUN(X,P1,P2,...).
%
%   LAMBDA contains the Lagrange multipliers.
%
%   to be worked out:
%   write a generalizer equation solver
%
%   See also LPOPF_SOLVER.

%   MATPOWER
%   $Id: LPconstr.m,v 1.12 2010/04/26 19:45:25 ray Exp $
%   by Deqiang (David) Gan, PSERC Cornell & Zhejiang University
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

% ------------------------------ setting up -----------------------------------

if nargin < 8, error('\ LPconstr needs more arguments ! \   '); end


nvars = length(x);
nequ =  mpopt(15);

% set up the arguments of FUN
if ~any(FUN<48) % Check alphanumeric
        etype = 1;
        evalstr = [FUN,];
        evalstr=[evalstr, '(x'];
        for i=1:nargin - 8
                etype = 2;
                evalstr = [evalstr,',P',int2str(i)];
        end
        evalstr = [evalstr, ')'];
else
        etype = 3;
        evalstr=[FUN,'; g=g(:);'];
end

%set up the arguments of GRADFUN
if ~any(GRADFUN<48) % Check alphanumeric
        gtype = 1;
        evalstr2 = [GRADFUN,'(x'];
        for i=1:nargin - 8
                gtype = 2;
                evalstr2 = [evalstr2,',P',int2str(i)];
        end
        evalstr2 = [evalstr2, ')'];
else
        gtype = 3;
        evalstr2=[GRADFUN,';'];
end

%set up the arguments of LPEQUSVR
if ~any(LPEQUSVR<48) % Check alphanumeric
        lpeqtype = 1;
        evalstr3 = [LPEQUSVR,'(x'];
        for i=1:nargin - 8
                lpeqtype = 2;
                evalstr3 = [evalstr3,',P',int2str(i)];
        end
        evalstr3 = [evalstr3, ')'];
else
        lpeqtype = 3;
        evalstr3=[LPEQUSVR,';'];
end

% ----------------------------- the main loop ----------------------------------
verbose = mpopt(31);
itcounter = 0;
runcounter = 1;

stepsize = step0 * 0.02; % use this small stpesize to detect how close to optimum, so to choose better stepsize

%stepsize = step0;
%fprintf('\n LPconstr does not adaptively choose starting point \n');

f_best = 9.9e15;
f_best_run = 9.9e15;
max_slackvar_last = 9.9e15;
converged = 0;

if verbose
    fprintf(' it   obj function   max violation  max slack var    norm grad       norm dx\n');
    fprintf('----  -------------  -------------  -------------  -------------  -------------\n');
end
while (converged == 0) && (itcounter < mpopt(22)) && (runcounter < mpopt(23))

    itcounter = itcounter + 1; 
    if verbose, fprintf('%3d ', itcounter); end


    % ----- fix xi temporarily, solve equations g1(xi, xo)=0 to get xo (by Newton method).
    if lpeqtype == 1,
        [x, success_lf] = feval(LPEQUSVR,x);
    elseif lpeqtype == 2
        [x, success_lf] = eval(evalstr3);
    else
        eval(evalstr3);
    end


    if success_lf == 0; 
        fprintf('\n      Load flow did not converge. LPconstr restarted with reduced stepsize! '); 
        x = xbackup;
        stepsize = 0.7*stepsize;
    end


    % ----- compute f, g, df_dx, dg_dx 

    if etype == 1,              % compute g(x)
            [f, g] = feval(FUN,x);
    elseif etype == 2
            [f, g] = eval(evalstr);
    else
            eval(evalstr);
    end
    if gtype == 1               % compute jacobian matrix
            [df_dx, dg_dx] = feval(GRADFUN, x);
    elseif gtype == 2
            [df_dx, dg_dx] = eval(evalstr2);
    else
            eval(evalstr2);
    end
    dg_dx = dg_dx';
    max_g = max(g);

    if verbose, fprintf('   %-12.6g   %-12.6g', f, max_g); end



    % ----- solve the linearized NP, that is, solve a LP to get dx
    a_lp = dg_dx; f_lp = df_dx; rhs_lp = -g; vubdx = stepsize; vlbdx = -stepsize;
    if isempty(VUB) ~= 1 || isempty(VLB) ~= 1
        error('sorry, at this stage LPconstr can not solve a problem with VLB or VUB ');
    end

    % put slack variable into the LP problem such that the LP problem is always feasible

    temp = find( ( g./(abs(g) + ones(length(g), 1) ))  > 0.1*mpopt(16));


    if isempty(temp) ~= 1
            n_slack = length(temp);
            if issparse(a_lp)
                a_lp = [a_lp, sparse(temp, 1:n_slack, -1, size(a_lp,1), n_slack)];
            else
                a_lp = [a_lp, full(sparse(temp, 1:n_slack, -1, size(a_lp,1), n_slack))];
            end
            vubdx = [vubdx; g(temp) + 1.0e4*ones(n_slack, 1)];
            vlbdx = [vlbdx; zeros(n_slack, 1)];
            f_lp = [f_lp; 9.9e6 * max(df_dx) * ones(n_slack, 1)];
    end


    % Ray's heuristics of deleting constraints

    if itcounter ==1
            idx_workc = [];
            flag_workc = zeros(3 * length(rhs_lp) + 2 * nvars, 1);
    else
            flag_workc = flag_workc - 1;
            flag_workc(idx_bindc) = 20 * ones(size(idx_bindc));

            if itcounter > 20
                    idx_workc = find(flag_workc > 0);
            end
    end



    [dx, lambda, idx_workc, idx_bindc] = LPsetup(a_lp, f_lp, rhs_lp, nequ, vlbdx, vubdx, idx_workc, mpopt);


    if length(dx) == nvars
        max_slackvar = 0;
    else
        max_slackvar = max(dx(nvars+1:length(dx))); if max_slackvar < 1.0e-8, max_slackvar = 0; end;
    end

    if verbose, fprintf('   %-12.6g', max_slackvar); end


    dx = dx(1 : nvars); % stripe off the reduendent slack variables


    % ----- update x, compute the objective function

    x = x + dx;
    xbackup = x;
    dL_dx = df_dx + dg_dx' * lambda;    % at optimal point, dL_dx should be zero (from KT condition)
    %norm_df = norm(df_dx, inf);
    norm_dL = norm(dL_dx, inf);
    if abs(f) < 1.0e-10
        norm_grad = norm_dL;
    else    
        norm_grad = norm_dL/abs(f);
        %norm_grad = norm_dL/norm_df;  % this is more stringent

    end
    norm_dx = norm(dx ./ step0, inf);

    if verbose, fprintf('   %-12.6g   %-12.6g\n', norm_grad, norm_dx); end

    % ----- check stopping conditions

    if (norm_grad < mpopt(20)) && (max_g < mpopt(16)) && (norm_dx < mpopt(21))
        converged = 1;  break;
    end

%   if max_slackvar > 1.0e-8 && itcounter > 60, break; end


    if norm_dx < 0.05 * mpopt(21),      % stepsize is overly small, so what is happening?

        if max_g < mpopt(16) && abs(f_best - f_best_run)/f_best_run < 1.0e-4 

            % The solution is the same as that we got in previous run. So we conclude that
            % the stopping conditions are overly stringent, and LPconstr HAS found the solution.
            converged = 1;  
            break;  

        else
            % stepsize is overly small to make good progress, we'd better restart using larger stepsize
            f_best_run = f_best;
            stepsize = 0.4* step0; 

            if verbose
                fprintf('\n----- restarted with larger stepsize\n');
            end

            runcounter = runcounter + 1;
        end;
    end


    % ----- adjust stepsize

    if itcounter == 1                           % the 1th iteration is a trial one
                                        % whihc sets up starting stepsize
        if     norm_grad <       mpopt(20)
                        stepsize = 0.015 * step0;   % use extra-small stepsize
        elseif norm_grad < 2.0 * mpopt(20)
                        stepsize = 0.05 * step0;    % use very small stepsize
        elseif norm_grad < 4.0 * mpopt(20)
                        stepsize = 0.3  * step0;    % use small stepsize
        elseif norm_grad < 6.0 * mpopt(20)
                        stepsize = 0.6  * step0;    % use less small stepsize
        else
                        stepsize =     step0;       % use large stepsize            
        end
    end

    if itcounter > 2
        if max_slackvar > max_slackvar_last + 1.0e-10
            stepsize = 0.7* stepsize; 
        end

        if max_slackvar < 1.0e-7        % the trust region method
            actual_df  = f_last - f;
            if abs(predict_df) > 1.0e-12
                    ratio = actual_df/predict_df;
            else
                ratio = -99999;
            end

            if ratio < 0.25 || f > f_last * 0.9999
                    stepsize = 0.5 * stepsize;
            elseif ratio > 0.80
                    stepsize = 1.05 * stepsize;
            end

            if norm(stepsize ./ step0, inf) > 3.0, stepsize = 3*step0; end;    % ceiling of stepsize
        end;
    end

    max_slackvar_last = max_slackvar;
    f_best = min(f, f_best);
    f_last = f;
    predict_df = -(df_dx(1:nvars))' * dx(1:nvars);
end

% ------ recompute f and g
if etype == 1,              % compute g(x)
            [f, g] = feval(FUN,x);
elseif etype == 2
            [f, g] = eval(evalstr);
else
        eval(evalstr);
end

i = find(g < -mpopt(16));
lambda(i) = zeros(size(i));
