function outputsesoln(idx, sigma, z, z_est, error_sqrsum)
%OUTPUTSESOLN  Output state estimation solution.
%   created by Rui Bo on 2008/01/14

%   MATPOWER
%   $Id: outputsesoln.m,v 1.5 2010/04/26 19:45:26 ray Exp $
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

fd = 1; % output to screen

fprintf(fd, '\n================================================================================');
fprintf(fd, '\n|     Comparison of measurements and their estimations                         |');
fprintf(fd, '\n|     NOTE: In the order of PF, PT, PG, Va, QF, QT, QG, Vm (if applicable)     |');
fprintf(fd, '\n================================================================================');
fprintf(fd, '\n    Type        Index      Measurement   Estimation');
fprintf(fd, '\n                 (#)         (pu)          (pu)    ');
fprintf(fd, '\n -----------  -----------  -----------   ----------');
cnt = 0;
len = length(idx.idx_zPF);
for i = 1: len
    fprintf(fd, '\n      PF        %3d      %10.4f     %10.4f', idx.idx_zPF(i), z(i+cnt), z_est(i+cnt));
end
cnt = cnt + len;
len = length(idx.idx_zPT);
for i = 1: len
    fprintf(fd, '\n      PT        %3d      %10.4f     %10.4f', idx.idx_zPT(i), z(i+cnt), z_est(i+cnt));
end
cnt = cnt + len;
len = length(idx.idx_zPG);
for i = 1: len
    fprintf(fd, '\n      PG        %3d      %10.4f     %10.4f', idx.idx_zPG(i), z(i+cnt), z_est(i+cnt));
end
cnt = cnt + len;
len = length(idx.idx_zVa);
for i = 1: len
    fprintf(fd, '\n      Va        %3d      %10.4f     %10.4f', idx.idx_zVa(i), z(i+cnt), z_est(i+cnt));
end
cnt = cnt + len;
len = length(idx.idx_zQF);
for i = 1: len
    fprintf(fd, '\n      QF        %3d      %10.4f     %10.4f', idx.idx_zQF(i), z(i+cnt), z_est(i+cnt));
end
cnt = cnt + len;
len = length(idx.idx_zQT);
for i = 1: len
    fprintf(fd, '\n      QT        %3d      %10.4f     %10.4f', idx.idx_zQT(i), z(i+cnt), z_est(i+cnt));
end
cnt = cnt + len;
len = length(idx.idx_zQG);
for i = 1: len
    fprintf(fd, '\n      QG        %3d      %10.4f     %10.4f', idx.idx_zQG(i), z(i+cnt), z_est(i+cnt));
end
cnt = cnt + len;
len = length(idx.idx_zVm);
for i = 1: len
    fprintf(fd, '\n      Vm        %3d      %10.4f     %10.4f', idx.idx_zVm(i), z(i+cnt), z_est(i+cnt));
end

fprintf(fd, '\n\n[Weighted sum of squared errors]:\t%f\n', error_sqrsum);
