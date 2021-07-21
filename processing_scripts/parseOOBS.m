clear
clc

[file,path] = uigetfile('/*.TXT','Multiselect','on');

%%

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
%using splitapply could be a much faster solution to these loops.
sn = unique(file_sn);
for i = 1:numel(sn)
    %read each daily txt file and append
    tmp = table();
    for j = find(sn(i)==file_sn)'
        tmp = [tmp; readtable(filepaths{j})];
    end
    tmp.dt = datetime(tmp.time, 'ConvertFrom', 'posixtime','Format','dd-MM-yyyy HH:mm:ss.SSSS');
    tmp.R0_V = tmp.R0 ./ 2^15 .* 5; %convert int16 representation of volts to float 
    
    %Loop through each burst. Identified by the temperature reading.
    measIdx = [find(~isnan(tmp.temp)); length(tmp.temp)+1];
    for j = 1:numel(measIdx)-1
        %-100 on first idx is an unfortunate kluge to correct for the 100
        %background measurements taken each burst before the temperature
        %reading that is used to mark the burst.
        if j ~= numel(measIdx)-1
             idx = measIdx(j)-100:measIdx(j+1)-101;
        else
            idx = measIdx(j)-100:measIdx(j+1)-1;
        end
        tmp.timeInterp(idx,1) = linspace(min(tmp.dt(idx)),max(tmp.dt(idx)),numel(tmp.dt(idx)));
        
        burstID(idx,1) = j;
        resampled.time(j,1) = mean(tmp.dt(idx));
        resampled.R0_V(j,1) = median(tmp.R0_V(idx)); 
        resampled.R0_V_sd(j,1) = std(tmp.R0_V(idx));
        resampled.temp(j,1) = tmp.temp(measIdx(j));
    end
    tmp.burstID = burstID;
    
    
    %store tmp table in data cell array
    d{i,1} = tmp;
end

% clearvars -except sn d
%% plots
close all

figure
set(gcf,'Units','normalized')
set(gcf,'Position',[0.1 0.1 0.8 0.8])
hold on
for i = 1:numel(d)
    legendStrings{i} = sprintf("OpenOBS %d",sn(i));
    yyaxis left
    plot(d{i}.timeInterp,d{i}.R0_V,'.')
    yyaxis right
    plot(d{i}.dt,d{i}.temp,'.')

    yyaxis left
end
legend(legendStrings)


figure
set(gcf,'Units','normalized')
set(gcf,'Position',[0.1 0.1 0.8 0.8])
hold on
for i = 1:numel(d)
    plot(resampled.time,resampled.R0_V,'.')
end



