function drawPVcurves(casedata, loadvarloc, corrected_list, combined_list, flag_combinedCurve, busesToDraw)
%DRAWPVCURVES  Draw PV curves for specified buses.
%   [INPUT PARAMETERS]
%   corrected_list, combined_list: data points obtained from CPF solver
%   loadvarloc: load variation location(in external bus numbering). Single bus supported so far.
%   flag_combinedCurve: flag indicating if the prediction-correction curve will be drawn
%   busesToDraw: bus indices whose PV curve will be be drawn
%   created by Rui Bo on 2008/01/13

%   MATPOWER
%   $Id: drawPVcurves.m,v 1.6 2010/04/26 19:45:26 ray Exp $
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

%% assign default parameters
if nargin < 6
    busesToDraw = loadvarloc;   % draw the curve for the load changing bus
end
if isempty(busesToDraw)
    busesToDraw = loadvarloc;   % draw the curve for the load changing bus
end

%% load the case & convert to internal bus numbering
[baseMVA, bus, gen, branch] = loadcase(casedata);
nb = size(bus, 1);

correctedDataNum = size(corrected_list, 2) - 1;
combinedDataNum  = size(combined_list, 2) - 1;

%% prepare data for drawing
lambda_corrected = corrected_list(nb+1, [2:correctedDataNum+1]);
lambda_combined  = combined_list(nb+1, [2:combinedDataNum+1]);

fprintf('Start plotting CPF curve(s)...\n');
for j = 1:length(busesToDraw)%for i = 1+npv+1:1+npv+npq
    i = find(corrected_list(:, 1) == busesToDraw(j)); % find row index
    
    %% get voltage magnitudes
    Vm_corrected = abs(corrected_list(i, [2:correctedDataNum+1]));
    Vm_combined  = abs(combined_list(i, [2:combinedDataNum+1]));
    
    %% create a new figure
    figure;
    hold on;
    
    %% plot PV curve
    plot(lambda_corrected, Vm_corrected, 'bx-');
    
    %% plot CPF prediction-correction curve
    if flag_combinedCurve == true
        plot(lambda_combined, Vm_combined, 'r.-');
        legend('CPF Curve', 'Prediction-Correction Curve');
        legend('Location', 'Best');
    end
    
    %% add plot title
    title(['Vm at bus ' int2str(busesToDraw(j)) ' w.r.t. load (p.u.) at ' int2str(loadvarloc)]);
end
fprintf('Plotting is done.\n');
