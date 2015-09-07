
import os, sys
from pylab import *
from matplotlib.dates import HOURLY, DateFormatter, rrulewrapper, RRuleLocator, drange, HourLocator
import datetime

f = open(sys.argv[1], "r")

results_test = False
ressults_global = False

node_count_dt   = []
node_count_c    = []
mc_percentages_dt  = []
mc_percentages_p   = []
uc_percentages_dt  = []
uc_percentages_p   = []

for line in f:
    #print line
    if "Test Results" in line:
        results_test = True
    elif "Overall Results" in line:
        results_test = False
        
    if "Multicast Successes" in line:
        parts = line.split(" ")
        parts[1] = { "Jan":1, "Feb":2, "Mar":3, "Apr":4, "May":5, "Jun":6, "Jul":7, "Aug":8, "Sep":9, "Oct":10, "Nov":11, "Dec":12}[parts[1]]
        t = datetime.datetime( int(parts[4]), int(parts[1]), int(parts[2]), int(parts[3].split(":")[0]), int(parts[3].split(":")[1]), int(parts[3].split(":")[2]))

        if (results_test == True):
            percentage = float(parts[11].split("(")[1].split("%)")[0])
            mc_percentages_dt.append(t)
            mc_percentages_p.append(percentage)
    
    if "Unicast Successes" in line:
        parts = line.split(" ")
        parts[1] = { "Jan":1, "Feb":2, "Mar":3, "Apr":4, "May":5, "Jun":6, "Jul":7, "Aug":8, "Sep":9, "Oct":10, "Nov":11, "Dec":12}[parts[1]]
        t = datetime.datetime( int(parts[4]), int(parts[1]), int(parts[2]), int(parts[3].split(":")[0]), int(parts[3].split(":")[1]), int(parts[3].split(":")[2]))

        if (results_test == True):
            percentage = float(parts[11].split("(")[1].split("%)")[0])
            uc_percentages_dt.append(t)
            uc_percentages_p.append(percentage)
    
    if "Discovered" in line:
        parts = line.split(" ")
        parts[1] = { "Jan":1, "Feb":2, "Mar":3, "Apr":4, "May":5, "Jun":6, "Jul":7, "Aug":8, "Sep":9, "Oct":10, "Nov":11, "Dec":12}[parts[1]]
        t = datetime.datetime( int(parts[4]), int(parts[1]), int(parts[2]), int(parts[3].split(":")[0]), int(parts[3].split(":")[1]), int(parts[3].split(":")[2]))

        node_count = int(parts[8])
        node_count_dt.append(t)
        node_count_c.append(node_count)
        
            

#rule = HourLocator(interval=2)
loc = HourLocator(interval=1)
formatter = DateFormatter('%H:%M')
ax = subplot(211)
plot_date(uc_percentages_dt, uc_percentages_p, "bo", label='Unicast')
plot_date(mc_percentages_dt, mc_percentages_p, "ro", label='Multicast')
legend(loc=0)
ax.xaxis.set_major_locator(loc)
ax.xaxis.set_major_formatter(formatter)
labels = ax.get_xticklabels()
setp(labels, rotation=30, fontsize=10)

ax = subplot(212)
plot_date(node_count_dt, node_count_c, label='Node Count')
legend(loc=0)
ax.xaxis.set_major_locator(loc)
ax.xaxis.set_major_formatter(formatter)
labels = ax.get_xticklabels()
setp(labels, rotation=30, fontsize=10)

show()