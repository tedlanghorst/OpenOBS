clear
clc

[file,path] = uigetfile('/*.TXT','Multiselect','on');
filepath = fullfile(path,file);

%look for the sensor serial number in each file.

fid = fopen(filepath);
for j = 1:5 %scan first 5 lines
    tline = fgetl(fid);
    if contains(tline,"OpenOBS SN:")
        sn = str2double(tline(12:end));
        break
    end
end
fclose(fid);

burstID = [];
%     timeInterp = [];

data = readtable(filepath);
data.R0_V = data.R0 ./ 2^15 .* 5; %convert digital number to voltage

data.dt = datetime(data.time, 'ConvertFrom', 'posixtime','Format','dd-MM-yyyy HH:mm:ss.SSSS');
isWrongDay = data.dt < datetime(file(1:8),'InputFormat','ddMMyyyy');
data(isWrongDay,:) = [];
isWrongDay = data.dt > datetime(file(1:8),'InputFormat','ddMMyyyy') + 1;
data(isWrongDay,:) = [];

%loop through the bursts
measIdx = [find(~isnan(data.temp)); length(data.temp)+1];
for j = 1:numel(measIdx)-1
    idx = measIdx(j):measIdx(j+1)-1;
    data.burstID(idx,1) = j-1;
    data.timeInterp(idx,1) = linspace(min(data.dt(idx)),max(data.dt(idx)),numel(data.dt(idx)));
    
%     idx_background = 
%     
%     background = data.R0(
end 

%resample
measured = [];
time = [];
burstIDs = unique(data.burstID);
for j = 1:numel(burstIDs)
    bid = burstIDs(j);
    idx_background = find(data.burstID==bid & data.gain==0);
    idx_meas = find(data.burstID==bid & data.gain==1);
    
    background(j,1) = mean(data.R0_V(idx_background));
    if isnan(background(j,1)) 
        background(j,1) = 0; 
    end
    
    groups_1Hz = findgroups(data.time(idx_meas));
    measured = [measured; splitapply(@mean, data.R0_V(idx_meas), groups_1Hz) - background(j)];
%     measured(j,1) = mean(data.R0_V(idx_meas)) - background(j);
    time = [time; splitapply(@mean, data.dt(idx_meas), groups_1Hz)];
end

% clearvars -except sn data file

close all

figure
colormap flag
set(gcf,'Units','normalized')
set(gcf,'Position',[0.1 0.1 0.8 0.6])
hold on
% scatter(data.timeInterp,data.R0_V,10,data.gain,'filled')
plot(time,measured,'b.','markersize',10);
title(sprintf("Serial no. %03d", sn))
zoom on


%% save calibration data
%{
    Before running this section, make sure you use the brush tool to select
    each set of data and right click -> export using the variable names
    used below... 
%}
close all

gen_path = "/Users/Ted/GDrive/OpenOBS/Calibrations/";

standards = [0,20,100,500,1000];
measured = [mean(a(:,2)), std(a(:,2)); 
        mean(b(:,2)), std(b(:,2)); 
        mean(c(:,2)), std(c(:,2)); 
        mean(d(:,2)), std(d(:,2)); 
        mean(e(:,2)), std(e(:,2))];

lm = fitlm(measured(:,1),standards);
NTU = predict(lm,measured(:,1));
        
eb = errorbar(standards,measured(:,1),measured(:,2));
eb.LineWidth = 1;
xlabel("Standard (NTU)");
ylabel("Measured (Volts)");


save_path = sprintf(strcat(gen_path,"%03d"),sn);
if ~exist(save_path, 'dir')
    mkdir(save_path)
end
save(fullfile(save_path,file(1:8)),"measured","standards","NTU","lm","data")

%% look at a bunch of cal data
cal_path = dir(fullfile(gen_path,"*","03072021.mat"));

sn_ignore = [10,15];
lgd_names = {};

close all
figure
for i = 1:numel(cal_path)
    load(fullfile(cal_path(i).folder,cal_path(i).name))
    cal_data(i).sn = str2double(cal_path(i).folder(end-2:end));
    cal_data(i).standards = standards;
    cal_data(i).measured = measured(:,1);
    cal_data(i).measured_sd = measured(:,2);
    cal_data(i).NTU = NTU;
    cal_data(i).lm = lm;
    
    if any(cal_data(i).sn == sn_ignore)
        continue
    end
    
    
    subplot(1,2,1)
    hold on
    plot(standards,measured(:,1),'o-','Linewidth',1.5)
    
    subplot(1,2,2)
    hold on
    plot(standards+1,NTU'-standards,'o-','Linewidth',1.5)
    
    lgd_names{numel(lgd_names)+1} = cal_path(i).folder(end-2:end);
end

subplot(1,2,1)
xlabel("Standard (NTU)");
ylabel("Measured (Volts)");
title('Raw Signal')
axis square
box on
lgd = legend(lgd_names,'Location','NorthWest');
title(lgd,"S/N")

subplot(1,2,2)
xlabel("Standard (NTU)");
ylabel("Calibration - Standard (NTU)");
title('Calibration Error')
axis square
box on
set(gca,'XScale','log')
xlim = get(gca,'XLim');
plot(xlim,[0,0],'k--','Linewidth',1)

% set(gca,'YScale','log')



set(gcf,'Units','normalized')
set(gcf,'Position',[0.3 0.4 0.4 0.3]);


% lims = get(gca,'XLim') + [-50,50];
% set(gca,'XLim',lims,'YLim',lims)
