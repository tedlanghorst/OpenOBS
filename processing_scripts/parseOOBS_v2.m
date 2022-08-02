clear
clc

calPath = "/Users/Ted/GDrive/OpenOBS/Calibrations_v2/";
[files,path] = uigetfile('/*.TXT','multiselect','on');

d = table();
for i = 1:numel(files)
    filepath = fullfile(path,files{i});

    %look for the sensor serial number in header.
    fid = fopen(filepath);
    for j = 1:5 %scan first 5 lines
        tline = fgetl(fid);
        if contains(tline,"OpenOBS SN:")
            sn(i) = str2double(tline(12:end));
            break
        end 
    end
    fclose(fid);

    d = vertcat(d,readtable(filepath));
end

if numel(unique(sn)) ~= 1
    error("Multiple serial numbers found in files");
else
    sn = sn(1);
end

%convert int16 DN representation of volts to float 
d.R0_V = d.R0 ./ 2^15 .* 5;
%%
%Loop through each burst. Identified by the temperature reading.
measIdx = [find(~isnan(d.temp)); length(d.temp)+1];    
for j = 1:numel(measIdx)-1
    idx = measIdx(j):measIdx(j+1)-1;
    d.timePlusMillis(idx) = d.time(idx(1))+(d.millis(idx)./1000);
   
    %split background and sample measurements
    idxBackground = idx(d.gain(idx)==0);
    idxSample = idx(d.gain(idx)~=0);
    background = median(d.R0_V(idxBackground));
    
    rs.time(j,1) = mean(d.time(idxSample));
    if background>0.1
        rs.R0_V(j,1) = NaN;
        rs.R0_V_sd(j,1) = NaN;
    else
        rs.R0_V(j,1) = median(d.R0_V(idxSample))-background; 
        rs.R0_V_sd(j,1) = std(d.R0_V(idxSample));
    end
end
[rs.time,sortIdx] = sort(rs.time); %investigate why this is necessary.
rs.R0_V = rs.R0_V(sortIdx);
rs.R0_V_sd = rs.R0_V_sd(sortIdx);

%convert timestamp
d.dt = datetime(d.timePlusMillis, 'ConvertFrom', 'posixtime','Format','dd-MM-yyyy HH:mm:ss.SSSS');
rs.dt = datetime(rs.time, 'ConvertFrom', 'posixtime','Format','dd-MM-yyyy HH:mm:ss.SSSS');




%find and apply the most recent calibration file
calDir = dir(sprintf("%s%03u/*.mat",calPath,sn));
if isempty(calDir)
    rs.NTU = NaN(rs.time,1);
else
    [~,mostRecent] = max([calDir.datenum]);
    calFile = fullfile(calDir(mostRecent).folder,calDir(mostRecent).name);
    load(calFile,"lm");
    rs.NTU = predict(lm,rs.R0_V);
    rs.NTU_sd = predict(lm,rs.R0_V_sd);
end

rs.sn = sn;
save([path 'parsedData.mat'],"rs");

% plots
close all

figure
set(gcf,'Units','normalized')
set(gcf,'Position',[0.1 0.1 0.8 0.8])
hold on
isBackground = d.gain==0;
yyaxis left
plot(d.dt,d.R0_V,'.')
plot(d.dt(isBackground),d.R0_V(isBackground),'o')
ylabel("Reading [Volts]")
yyaxis right
plot(d.dt,d.temp,'.')
ylabel("Temperature [C]")
title("Raw OpenOBS Data")
yyaxis left %for zoom control

% figure
% set(gcf,'Units','normalized')
% set(gcf,'Position',[0.3 0.3 0.5 0.4])
% plot(rs.dt,rs.NTU,'.')
% title("Lab Calibrated NTUs")
% ylabel('NTU')

figure
set(gcf,'Units','normalized')
set(gcf,'Position',[0.3 0.3 0.5 0.4])
plot(rs.dt,rs.R0_V,'-')
title("Resampled measurements with background removed")
ylabel("Burst average - background [V]")


