function [varargout] = mp_lp(varargin)
%------------------------------  deprecated  ------------------------------
%   Use QPS_MATPOWER instead.
%--------------------------------------------------------------------------
%MP_LP  Linear program solver.
%   [X, LAMBDAOUT, HOWOUT, SUCCESS] = ...
%       MP_LP(f, A, b, VLB, VUB, X0, N, VERBOSE, ALG, OPT)
%
%   A common wrapper for various LP solvers, using the calling syntax of
%   LP from version 1 of the Optimization Toolbox, with the exception
%   that verbose == 0 means no output. The optional argument alg
%   determines the solver.
%     alg = 100  :  BPMPD_MEX
%     alg = 200  :  MIPS, MATLAB Interior Point Solver
%                   pure MATLAB implementation of a primal-dual
%                   interior point method
%     alg = 250  :  MIPS-sc, a step controlled variant of MIPS
%     alg = 300  :  Optimization Toolbox, LINPROG or LP
%     alg = 400  :  IPOPT
%     alg = 500  :  CPLEX
%     alg = 600  :  MOSEK
%   If ALG is missing or equal to zero, the first available solver is used.
%   An additional optional argument OPT can be used to set algorithm
%   specific options.
%
%   From the Optimization Toolbox v.1 docs ...
%     X=LP(f,A,b) solves the linear programming problem:
%
%          min f'x    subject to:   Ax <= b 
%           x
%
%     X=LP(f,A,b,VLB,VUB) defines a set of lower and upper
%     bounds on the design variables, X, so that the solution is always in
%     the range VLB <= X <= VUB.
%  
%     X=LP(f,A,b,VLB,VUB,X0) sets the initial starting point to X0.
%  
%     X=LP(f,A,b,VLB,VUB,X0,N) indicates that the first N constraints defined
%     by A and b are equality constraints.
%  
%     X=LP(f,A,b,VLB,VUB,X0,N,DISPLAY) controls the level of warning
%     messages displayed.  Warning messages can be turned off with
%     DISPLAY = -1.
%  
%     [x,LAMBDA]=LP(f,A,b) returns the set of Lagrangian multipliers,
%     LAMBDA, at the solution. 
%  
%     [X,LAMBDA,HOW] = LP(f,A,b) also returns a string how that indicates 
%     error conditions at the final iteration.
%  
%     LP produces warning messages when the solution is either unbounded
%     or infeasible. 

%   MATPOWER
%   $Id: mp_lp.m 4738 2014-07-03 00:55:39Z dchassin $
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

[varargout{1:nargout}] = mp_qp([], varargin{:});
