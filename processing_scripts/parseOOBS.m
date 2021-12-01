clear
clc

calPath = "/Users/Ted/GDrive/OpenOBS/Calibrations/";
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
%using splitapply could be a much faster solution to these loops.
sn = unique(file_sn);
for i = 1:numel(sn)
    %read each daily txt file and append
    tmp = table();
    for j = find(sn(i)==file_sn)'
        tmp = [tmp; readtable(filepaths{j})];
    end
    %convert timestamp
    tmp.dt = datetime(tmp.time, 'ConvertFrom', 'posixtime','Format','dd-MM-yyyy HH:mm:ss.SSSS');
    %convert int16 DN representation of volts to float 
    tmp.R0_V = tmp.R0 ./ 2^15 .* 5;
    
    %Loop through each burst. Identified by the temperature reading.
    measIdx = [find(~isnan(tmp.temp)); length(tmp.temp)+1];    
    for j = 1:numel(measIdx)-1
        idx = measIdx(j):measIdx(j+1)-1;
        tmp.timeInterp(idx,1) = linspace(min(tmp.dt(idx)),max(tmp.dt(idx)),numel(tmp.dt(idx)));
        
        %split background and sample measurements
        idxBackground = idx(tmp.gain(idx)==0);
        idxSample = idx(tmp.gain(idx)~=0);
        background = median(tmp.R0_V(idxBackground));
        
        %average the sampling burst and filter high background meas.
        resampled.time(j,1) = mean(tmp.dt(idxSample));
        resampled.background(j,1) = background;
        resampled.temp(j,1) = tmp.temp(measIdx(j));
        
        if background>0.05
            resampled.R0_V(j,1) = NaN;
            resampled.R0_V_sd(j,1) = NaN;
        else
            resampled.R0_V(j,1) = median(tmp.R0_V(idxSample))-background; 
            resampled.R0_V_sd(j,1) = std(tmp.R0_V(idxSample));
        end
        burstID(idx,1) = j;
    end
    tmp.burstID = burstID;
    
    %find and apply the most recent calibration file
    calDir = dir(sprintf("%s%03u/*.mat",calPath,sn(i)));
    if isempty(calDir)
        resampled.NTU = NaN(resampled.time,1);
    else
        [~,mostRecent] = max([calDir.datenum]);
        calFile = fullfile(calDir(mostRecent).folder,calDir(mostRecent).name);
        load(calFile,"lm");
        resampled.NTU = predict(lm,resampled.R0_V);
        resampled.NTU_sd = predict(lm,resampled.R0_V_sd);
    end
    
    %store tmp table in data cell array
    d{i,1} = tmp;
end
%%
% plots
close all

figure
set(gcf,'Units','normalized')
set(gcf,'Position',[0.1 0.1 0.8 0.8])
hold on
for i = 1:numel(d)
    legendStrings{i} = sprintf("OpenOBS %03d",sn(i));
    yyaxis left
    plot(d{i}.timeInterp,d{i}.R0_V,'.')
    yyaxis right
    plot(d{i}.dt,d{i}.temp,'.')
end
legend(legendStrings)
title("Raw OpenOBS Data")
yyaxis right
ylabel("Temperature [C]")
yyaxis left
ylabel("Reading [Volts]")


figure
set(gcf,'Units','normalized')
set(gcf,'Position',[0.3 0.3 0.5 0.4])
hold on
for i = 1:numel(d)
    plot(resampled.time,resampled.NTU,'.')
end
legend(legendStrings)
title("Lab Calibrated NTUs")
ylabel('NTU')

