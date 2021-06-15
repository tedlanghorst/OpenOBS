clear
clc

[file,path] = uigetfile('/*.TXT','Multiselect','on');

%assemble cell array of the selected file paths
if isa(file,'cell')
    filepaths = cell(numel(file),1);
    for i = 1:numel(file)
        filepaths{i} = fullfile(path,file{i});
    end
elseif isa(file,'char')
    filepaths{1} = fullfile(path,file);
end

%look for the sensor serial number in each file.
sn = NaN(numel(filepaths),1);
for i = 1:numel(filepaths)
    fid = fopen(filepaths{i});
    for j = 1:5 %scan first 5 lines
        tline = fgetl(fid);
        if contains(tline,"OpenOBS SN:")
            sn(i) = str2double(tline(12:end));
            break
        end
    end
    fclose(fid);
end

%create a table for each serial number.
%store in a cell array with corresponding SN array.
[sn,~,fileSN] = unique(sn);
for i = 1:numel(sn)
    burstID = [];
    timeInterp = [];
    
    tmp = table();
    for j = find(sn(i)==fileSN)'
        tmp = [tmp; readtable(filepaths{j})];
    end
    
    measIdx = [find(~isnan(tmp.temp)); height(tmp.temp)+1];
    for j = 1:numel(measIdx)-1
        idx = measIdx(j):measIdx(j+1)-1;
        burstID(idx,1) = j;
        timeInterp(idx,1) = linspace(min(tmp.time(idx)),max(tmp.time(idx)),numel(tmp.time(idx)));
    end
    tmp.burstID = burstID;
    tmp.timeInterp = datetime(timeInterp, 'ConvertFrom', 'posixtime' );
    tmp.dt = datetime(tmp.time, 'ConvertFrom', 'posixtime' );
    tmp.R0_V = tmp.R0 ./ 2^15 .*5;
    
    d{i,1} = tmp;
end

% for i = 1:numel(d)
%     %temp is only read on first entry of each wake cycle.
%     measIdx = [find(~isnan(d{i}.temp)); height(d{i}.temp)+1];
%     for j = 1:numel(measIdx)-1
%         idx = measIdx(j):measIdx(j+1)-1;
% 
%         dt(j,1) = mean(d{i}.dt(idx));
%         R0_V_mean(j,1) = mean(d{i}.R0_V(idx));
%         R0_V_std(j,1) = mean(d{i}.R0_V(idx));
%         temp(j,1) = d{i}.temp(measIdx(j));
%     end
% end


clearvars -except sn d


%% plots
close all

figure
subplot(2,1,1)
hold on
for i = 1:numel(d)
    legendStrings{i} = sprintf("OpenOBS %d",sn(i));
    plot(d{i}.timeInterp,d{i}.R0_V,'.')
end

legend(legendStrings)

% close all
% figure
% 
% yyaxis right
% plot(d.dt,d.temp,'*')
% ylabel('Temperature [\circC]')
% 
% yyaxis left
% plot(d.dt,d.R0_V,'.');
% ylabel('OBS reading [Volts]')
% 
% %%
% 

% 
% 
% figure
% yyaxis right
% plot(dt,temp,'*')
% ylabel('Temperature [\circC]')
% 
% yyaxis left
% plot(dt,R0_V,'.');
% ylabel('OBS reading [Volts]')


