# -*- coding: utf-8 -*-
"""
Created on Fri Apr  3 17:34:57 2020

@author: abaghdad
"""

import matplotlib.pyplot as plt
from datetime import timedelta
from matplotlib.dates import DateFormatter
import matplotlib.dates as mdates
import numpy as np
from datetime import datetime
import time
import csv    #avg = 50
from cycler import cycler
from collections import defaultdict
from shutil import copyfile
from matplotlib.animation import FuncAnimation
import matplotlib
import datetime


#%matplotlib qt
import matplotlib.pyplot as plt

def import_data(name):
    #import data from csv file  
    #my_data = np.genfromtxt(name+'.csv', delimiter=',', skip_header=1)

    copyfile(name+'.csv', name+'_jupyterCOPY.csv') #make a working copy
    i=0
    
    output_titles = []
    output_data = []

    with open(name+'_jupyterCOPY.csv', 'rt') as csvfile:
        fileInput = csv.reader(csvfile, delimiter=',')
        next(fileInput, None)
        for row in fileInput:
            for column in row:
                output_titles.append(column.rsplit(':', 1)[0])
                #variables have to be defined globally
                #brake.append(float(row[0].rsplit(':', 1)[1]))
            break
        
        output_data_temp = []
        for row in fileInput:
            temp_row = []
            for column in row:
                temp_row.append(column.rsplit(':', 1)[1])
            output_data_temp.append(temp_row)

        column_num = len(output_data_temp[0])
        for i in range (column_num):
            temp_column = []
            for row in output_data_temp:
                temp_column.append(row[i])
            output_data.append(temp_column)
            
 #       for row in fileInput:
 #           temporary_row = [];
 #           for column in row:
 #               temporary_row.append(column.rsplit(':', 1)[1])
 #           output_data.append(temporary_row)
    return [output_titles, output_data]

def plot_over_first_row(output_titles, output_data, fig):       

    #hole_speed_avg = movingaverage(hole_speed,avg)
    #t_avg = t[avg-1:]    
    t_axis = fig.add_subplot(111);
    t_axis.set_xlabel(output_titles[0])
    colors=plt.cm.rainbow(np.linspace(0,1,len(output_titles)))

    
    i = 0;
    for element_title, element_data in zip(output_titles, output_data):
        if(i == 0):
            a=1; #just some bullshit to make Spiyder compile without errors
        else:
            ax_temp = t_axis.twinx()
            ax_temp.set_ylabel(element_title) 
            ax_temp.plot([float(i) for i in output_data[0]], [float(i) for i in element_data], color = colors[i],  linewidth = 1, label=element_title)
            ax_temp.tick_params(axis='y',direction="out",labelcolor = colors[i], grid_color =colors[i], grid_alpha=0, pad = (i-1)*80)
            plt.tight_layout()
        i = i + 1
    
    #plt.show()

def plot_over_datetime(output_titles, output_data, datetime_format,fig):       

    #hole_speed_avg = movingaverage(hole_speed,avg)
    #t_avg = t[avg-1:]    
    t_axis = fig.add_subplot(111);
    t_axis.set_xlabel(output_titles[0])
    colors=plt.cm.rainbow(np.linspace(0,1,len(output_titles)))
    for element in output_data[0]: 
        element = matplotlib.dates.date2num(datetime.datetime.strptime(element,datetime_format))
    
    i = 0;
    for element_title, element_data in zip(output_titles, output_data):
        if(i == 0):
            a=1; #just some bullshit to make Spiyder compile without errors
        else:
            ax_temp = t_axis.twinx()
            ax_temp.set_ylabel(element_title) 
            ax_temp.plot_date(output_data[0], [float(i) for i in element_data], color = colors[i],  linewidth = 1, label=element_title)
            ax_temp.tick_params(axis='y',direction="out",labelcolor = colors[i], grid_color =colors[i], grid_alpha=0, pad = (i-1)*80)
            plt.tight_layout()
        i = i + 1
    #plt.show()

def plotFile(i,filename, fig):
    [titles, data] = import_data(filename)
    plt.clf()
    plot_over_first_row(titles, data, fig)

## just for testing
#fig = plt.figure()
#ani = FuncAnimation(plt.gcf(), livePlotFile,fargs=("test", fig), interval=1000)

def plotFileOverTime(i,filename, datetime_format, fig):
    [titles, data] = import_data(filename)
    plt.clf()
    plot_over_datetime(titles, data, datetime_format, fig)
    
#%matplotlib qt
plt.style.use('fivethirtyeight')
fig = plt.figure()
ani = FuncAnimation(plt.gcf(), plotFile,fargs=("dataArd", fig), interval=50)