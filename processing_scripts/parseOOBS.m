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
file_sn = NaN(numel(filepaths),1);
for i = 1:numel(filepaths)
    fid = fopen(filepaths{i});
    for j = 1:5 %scan first 5 lines
        tline = fgetl(fid);
        if contains(tline,"OpenOBS SN:")
            file_sn(i) = str2double(tline(12:end));
            break
        end
    end
    fclose(fid);
end

%create a table for each serial number.
%store in a cell array with corresponding SN array.
sn = unique(file_sn);
for i = 1:numel(sn)
    burstID = [];
%     timeInterp = [];
    
    tmp = table();
    for j = find(sn(i)==file_sn)'
        tmp = [tmp; readtable(filepaths{j})];
    end
    
    tmp.dt = datetime(tmp.time, 'ConvertFrom', 'posixtime','Format','dd-MM-yyyy HH:mm:ss.SSSS');
    isWrongDay = tmp.dt < datetime(file(1:8),'InputFormat','ddMMyyyy');
    tmp(isWrongDay,:) = [];
    isWrongDay = tmp.dt > datetime(file(1:8),'InputFormat','ddMMyyyy') + 1;
    tmp(isWrongDay,:) = [];
    
    measIdx = [find(~isnan(tmp.temp)); height(tmp.temp)+1];
    for j = 1:numel(measIdx)-1
        idx = measIdx(j):measIdx(j+1)-1;
        burstID(idx,1) = j;
        tmp.timeInterp(idx,1) = linspace(min(tmp.dt(idx)),max(tmp.dt(idx)),numel(tmp.dt(idx)));
    end
    tmp.burstID = burstID;
%     tmp.timeInterp = datetime(timeInterp, 'ConvertFrom', 'posixtime','Format','dd-MM-yyyy HH:mm:ss.SSSS');
    
    tmp.R0_V = tmp.R0 ./ 2^15 .* 5;
    

    
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


% clearvars -except sn d


%% plots
close all

figure
set(gcf,'Units','normalized')
set(gcf,'Position',[0.1 0.1 0.8 0.8])
hold on
for i = 1:numel(d)
    legendStrings{i} = sprintf("OpenOBS %d",sn(i));
    plot(d{i}.timeInterp,d{i}.R0_V,'.')
%     plot(d{i}.dt,d{i}.R0_V,'.')
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


