clear
clc

[file,path] = uigetfile('/*.TXT','Multiselect','on');

%handles one or multiple files differently
if isa(file,'cell')
    d = table;
    for i = 1:numel(file)
        d = [d;readtable(fullfile(path,file{i}))];
    end
elseif isa(file,'char')
    d = readtable(fullfile(path,file));
end

d.dt = datetime(d.time, 'ConvertFrom', 'posixtime' );
d.R0_V = d.R0 ./ 2^15 .*5;

close all
figure

yyaxis right
plot(d.dt,d.temp,'*')
ylabel('Temperature [\circC]')

yyaxis left
plot(d.dt,d.R0_V,'.');
ylabel('OBS reading [Volts]')



