function [xout, lambdaout, howout, success] = mp_qp(H,f,A,b,VLB,VUB,x0,N,verbose,alg,opt)
%------------------------------  deprecated  ------------------------------
%   Use QPS_MATPOWER instead.
%--------------------------------------------------------------------------
%MP_QP  Quadratic program solver.
%   [X, LAMBDAOUT, HOWOUT, SUCCESS] = ...
%       MP_QP(H, f, A, b, VLB, VUB, X0, N, VERBOSE, ALG, OPT)
%
%   A common wrapper for various QP solvers, using the calling syntax of
%   QP from version 1 of the Optimization Toolbox, with the exception
%   that verbose == 0 means no output. The optional argument alg
%   determines the solver.
%     alg = 100  :  BPMPD_MEX
%     alg = 200  :  MIPS, MATLAB Interior Point Solver
%                   pure MATLAB implementation of a primal-dual
%                   interior point method
%     alg = 250  :  MIPS-sc, a step controlled variant of MIPS
%     alg = 300  :  Optimization Toolbox, QUADPROG or QP
%     alg = 400  :  IPOPT
%     alg = 500  :  CPLEX
%     alg = 600  :  MOSEK
%   If ALG is missing or equal to zero, the first available solver is used.
%   An additional optional argument OPT can be used to set algorithm
%   specific options.
%
%   From the Optimization Toolbox v.1 docs ...
%     X=QP(H,f,A,b) solves the quadratic programming problem:
%
%          min 0.5*x'Hx + f'x   subject to:  Ax <= b 
%           x
%
%     X=QP(H,f,A,b,VLB,VUB) defines a set of lower and upper
%     bounds on the design variables, X, so that the solution  
%     is always in the range VLB <= X <= VUB.
%  
%     X=QP(H,f,A,b,VLB,VUB,X0) sets the initial starting point to X0.
%  
%     X=QP(H,f,A,b,VLB,VUB,X0,N) indicates that the first N constraints 
%     defined by A and b are equality constraints.
%  
%     X=QP(H,f,A,b,VLB,VUB,X0,N,DISPLAY) controls the level of warning
%     messages displayed.  Warning messages can be turned off with
%     DISPLAY = -1.
%  
%     [x,LAMBDA]=QP(H,f,A,b) returns the set of Lagrangian multipliers,
%     LAMBDA, at the solution.
%  
%     [X,LAMBDA,HOW] = QP(H,f,A,b) also returns a string HOW that 
%     indicates error conditions at the final iteration.
%  
%     QP produces warning messages when the solution is either unbounded
%     or infeasible. 

%   MATPOWER
%   $Id: mp_qp.m 4738 2014-07-03 00:55:39Z dchassin $
%   by Ray Zimmerman, PSERC Cornell
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

%% set up options
if nargin < 11
    opt = [];
    if nargin < 10
        alg = 0;
    end
end
if verbose == -1
    verbose = 0;
end
qps_opt = struct('alg', alg, 'verbose', verbose);
if ~isempty(opt)
    qps_opt.mips_opt = opt;
end

%% create lower limit for linear constraints
m = size(A, 1);
l = b;
l((N+1):m) = -Inf * ones(m-N, 1);

%% call solver
[xout, fval, howout, output, lambda] = qps_matpower(H, f, A, l, b, VLB, VUB, x0, qps_opt);

%% prepare output
if nargout > 1
    lambdaout = [   lambda.mu_u - lambda.mu_l;
                    lambda.lower;
                    lambda.upper    ];
    if nargout > 3
        success = (howout == 1);
    end
end
