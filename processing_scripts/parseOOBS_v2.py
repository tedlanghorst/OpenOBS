#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Wed Aug 2 10:43:48 2022

@author: Ted
"""

import pandas as pd
import tkinter.filedialog as fd
import matplotlib as plt
import numpy as np

# %% load the data
filenames = fd.askopenfilenames()

#check for valid header on each file and record serial numbers
snList = []
goodFileList = []
for f in filenames:
    with open(f) as fOpen:
        headerLines = fOpen.readlines(100)
        if ('OpenOBS SN:' in headerLines[2]):
            #found the correct header in this file.
            snList.append(int(headerLines[2][-3:-1]))
            goodFileList.append(f)
        else:
            print('\nWARNING: missing header from file \"{filename}\"\n'.format(filename=f))
            continue
    
#check if we have matching serial numbers
nUniqueSN = len(set(snList))
if  nUniqueSN == 0:
    raise SystemExit('ERROR: No valid files found')
if nUniqueSN > 1:
    raise SystemExit(f'ERROR: Multiple SNs found: {snList}')

#read all the data into one dataframe
df = pd.concat((pd.read_csv(f,header=3) for f in goodFileList))

# %% process the data

#each burst/wake cycle is marked by a temperature reading
df['burstID'] = pd.notna(df.temp).cumsum()

#average measurements by their burst ID
df_burst = df.groupby(by=['burstID','gain']).agg('median').unstack()

#reformat multilevel column names
df_burst.columns = ["{0}_{1}".format(c[0],c[1]) for c in df_burst.columns]

#drop useless columns
df_burst.drop(columns=['time_0','millis_0','millis_1','temp_1'],inplace=True)

#rename with friendly names
df_burst.columns = ['unixTime','background','reading','temperature']

#add new columns
df_burst['datetime'] = pd.to_datetime(df_burst['unixTime'],unit='s')
df_burst['corrected'] = df_burst['reading'] - df_burst['background']
df_burst.loc[df_burst['background']>2000,'corrected'] = np.nan #2000 found experimentally


# %% save and plot

#save the data
savepath = fd.asksaveasfilename()
if savepath:
    if not savepath.endswith('.csv'): savepath+='.csv' 
    df_burst.to_csv(savepath)
else:
    print('\nData not saved to file\n')


df_burst.plot('datetime','corrected')

    