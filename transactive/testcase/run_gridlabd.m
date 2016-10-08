% run gridlabd

update_glm;

system('..\..\vs2005\x64\debug\gridlabd test_wecc2024_case.glm'); 
fprintf('Generating plots...');
make_plots('WECC2024');
%for m = 1:size(controlarea,1)
%    areaname = char(controlarea(m,:).AreaName{1});
%    fprintf('.');
%    make_plots(areaname);
%    make_plots([areaname,'_L']);
%    make_plots([areaname,'_G']);
%end
fprintf('done.\n');
