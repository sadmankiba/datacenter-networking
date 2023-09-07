import matplotlib.pyplot as plt
import numpy as np


#### Separate #####
def nw_many_bw():
    x = np.arange(4)
    y1 = np.array([26.2, 30.5 / 2, 21 / 3, 21 / 4])
    y2 = np.array([26.2, 30.5, 21, 21])
    plt.xticks([0, 1, 2, 3], ['1', '2', '3', '4'])
    plt.bar(x, y1, color='tab:blue', width=0.5)
    plt.bar(x,y2, alpha=0.5, color='cornflowerblue', width=0.5)
    plt.xlabel('Number of flows')
    plt.ylabel('Throughput (Gbps)')
    plt.legend(labels=['per-flow', 'total'])

# 1	1115
# 2	1940
# 4	2260
# 8	2400
def t_app_many_bw():
    x = range(1,5)
    y1 = np.array([1115, 1940 / 2, 2260 / 4, 2400 / 8])
    y2 = np.array([1115, 1940, 2260, 2400])
    plt.xticks([1, 2, 3, 4], ['1', '2', '4', '8'])
    plt.bar(x, y1, color='tab:blue', width=0.5)
    plt.bar(x, y2, alpha=0.5, color='cornflowerblue', width=0.5)
    plt.xlabel('Number of T-apps')
    plt.ylabel('Throughput (MBps)')
    plt.legend(labels=['per-app', 'total'])

def t_app_many_util():
    x = np.arange(4)
    y1 = np.array([40, 38, 39, 26])
    y2 = np.array([37, 35, 32, 16])

    # Plot bar chart of y1 and y2 side-by-side with x as x-axis 
    plt.bar(x - 0.2, y1, color = 'tab:blue', width = 0.4)
    plt.bar(x + 0.2, y2, color = 'tab:orange', width = 0.4)

    # Add xticks on the middle of the group bars
    plt.xlabel('Number of T-apps')
    plt.ylabel('CPU Utilization (%)')
    plt.xticks([0, 1, 2, 3], ['1', '2', '4', '8'])
    # Set yticks
    plt.yticks([10, 20, 30, 40, 50])

    # Set legend
    plt.legend(labels=['host', 'target'])

#### Mix #####
def t_app_many_bw_nw():
    x = np.arange(4)
    y1 = np.array([750, 701, 480, 263])
    y2 = np.array([0, 940, 550, 285])

    # Plot bar chart of y1 and y2 side-by-side with x as x-axis 
    plt.bar(x + 0.2, y2, color = 'tab:orange', width = 0.4, label='unshared core')
    plt.bar(x - 0.2, y1, color = 'tab:blue', width = 0.4, label='shared core')
    

    # Add xticks on the middle of the group bars
    plt.xlabel('Number of storage T-apps')
    plt.ylabel('T-app bandwidth (MBps)')
    plt.xticks([0, 1, 2, 3], ['1', '2', '4', '8'])
    # Set yticks
    plt.yticks([200, 400, 600, 800, 1000])

    # Set legend
    plt.legend()

def t_app_many_nw_bw():
    # Nw bandwidth in contention (Gbps)
    # Set font size 
    plt.rcParams.update({'font.size': 14})
    x = np.arange(4)
    y = np.array([18.1, 16.6, 17.5, 19.3])
    plt.xticks([0, 1, 2, 3], ['1', '2', '4', '8'])
    plt.bar(x, y, color='tab:blue', width=0.5)
    plt.xlabel('Number of T-apps')
    plt.ylabel('Nw app throughput (Gbps)')
    # Set yticks
    plt.yticks([5, 10, 15, 20, 25])
    # Plot a horizontal dashed line
    plt.axhline(y=26.2, color='r', linestyle='--')
    # Add a text above the horizontal line
    plt.text(0.5, 26.2, 'isolated', ha='center', va='bottom', color='r')  

def t_app_many_util_nw():
    # bar chart in groups of 2
    # Sender core util in contention	Sender core util in non-contention	Receiver core util in contention	Receiver core util in non-contention
    # 40	37
    # 38	35
    # 39	32
    # 26	16
    # 95%		95%	
    # 91%	44%	94%	56%
    # 92%	36%	90%	42%
    # 94%	30%	85%	35%
    x = np.arange(9)
    # core util of t-apps without contention    
    y1 = np.array([40, 38, 39, 26, 0, 37, 35, 32, 16])
    # core util in unshared in contention of t-apps and 1 nw-app
    y2 = np.array([0, 44, 36, 30, 0, 0, 56, 42, 35]) 
    
    # Plot bar chart of y1 and y2 side-by-side with x as x-axis 
    plt.bar(x - 0.2, y1, color = 'tab:purple', width = 0.4)
    plt.bar(x + 0.2, y2, color = 'tab:olive', width = 0.4)

    # Add xticks on the middle of the group bars
    # Set xlabel position slightly below 
    plt.xlabel('Number of T-apps', y=-20)
    # Add text below x-axis
    plt.text(1.5, -7.5, 'Host', ha='center', va='bottom', color='slategrey')
    plt.text(6.5, -7.5, 'Target', ha='center', va='bottom', color='slategrey')
    plt.ylabel('CPU Utilization (%)')
    plt.xticks([0, 1, 2, 3, 4, 5, 6, 7, 8], ['1', '2', '4', '8', '', '1', '2', '4', '8'])

    # Set legend
    # Set legend position at top
    plt.legend(labels=['without nw-app', 'with nw-app'], loc='upper center', bbox_to_anchor=(0.5, 1.15), ncol=2)
     # Set yticks
    plt.yticks([10, 20, 30, 40, 50])

def t_app_bw_nw_many():
    # 750
    # 701
    # 181
    # 145
    x = np.arange(5)
    y = np.array([1115, 750, 701, 181, 145])
    plt.xticks([0, 1, 2, 3, 4], ['0', '1', '2', '3', '4'])
    plt.bar(x, y, color='tab:blue', width=0.5)
    plt.xlabel('Number of network apps')
    plt.ylabel('Throughput (MBps)')

def t_app_nw_many_bw():
    # 18.1	0
    # 13.6	11.4
    # 7	7.1
    # 5.25	5.3
    x = np.arange(4)
    y1 = np.array([18.1, 13.6, 7, 5.25])
    y2 = np.array([0, 11.4, 7.1, 5.3])

    # Plot bar chart of y1 and y2 side-by-side with x as x-axis 
    plt.bar(x + 0.2, y2, color = 'tab:orange', width = 0.4, label='unshared core')
    plt.bar(x - 0.2, y1, color = 'tab:blue', width = 0.4, label='shared core')
    

    # Add xticks on the middle of the group bars
    plt.xlabel('Number of network apps')
    plt.ylabel('Avg bandwidth (Gbps)')
    plt.xticks([0, 1, 2, 3], ['1', '2', '3', '4'])

    # Set legend
    plt.legend()

def t_app_util_nw_many():
    # bar chart in groups of 2
    # Sender core util in contention	Sender core util in non-contention	Receiver core util in contention	Receiver core util in non-contention
    # 95%		95%	
    # 87%	47%	74%	68%
    # 57%	24%	26%	33%
    # 41%	18%	20%	22%
    x = np.arange(9)
    y1 = np.array([0, 47, 24, 18, 0, 0, 68, 33, 22])
    y2 = np.array([95, 87, 57, 41, 0, 95, 74, 26, 20])

    # Plot bar chart of y1 and y2 side-by-side with x as x-axis 
    plt.bar(x - 0.2, y1, color = 'tab:purple', width = 0.4)
    plt.bar(x + 0.2, y2, color = 'tab:olive', width = 0.4)

    # Add xticks on the middle of the group bars
    # Set xlabel position slightly below 
    plt.xlabel('Number of network apps', y=-20)
    # Add text below x-axis
    plt.text(0.5, -15, 'Sender', ha='center', va='bottom', color='slategrey')
    plt.text(7.5, -15, 'Receiver', ha='center', va='bottom', color='slategrey')
    plt.ylabel('CPU Utilization (%)')
    plt.xticks([0, 1, 2, 3, 4, 5, 6, 7, 8], ['1', '2', '3', '4', '', '1', '2', '3', '4'])

    # Set legend
    # Set legend position at top
    plt.legend(labels=['unshared core', 'shared core'], loc='upper center', bbox_to_anchor=(0.5, 1.15), ncol=2)
     # Set yticks
    plt.yticks([20, 40, 60, 80, 100])

def l_app_many_lat_nw():
    # 1 (no nw app)	61	55
    # 1	174	276
    # 2	230	350
    # 3	240	350
    # 4	265	385
    # 8	310	430
    x = np.arange(5)
    y = np.array([61, 174, 230, 265, 310])
    # Add error bar on y
    yerr = np.array([0, 276 - 174, 350 - 230, 350 - 265, 430 - 310])
    plt.errorbar(x, y, yerr=yerr, fmt='o', color='k', capsize=5, zorder=1)
    plt.xticks([0, 1, 2, 3, 4], ['isolated', '1', '2', '4', '8'])
    # draw the bar plot on top of the error bars
    plt.bar(x, y, color='tab:blue', width=0.5, zorder=2)
    plt.xlabel('Number of L-apps')
    plt.ylabel('Latency (us)')

def l_app_many_nw_bw():
    # 23.4
    # 22.6
    # 21.4
    # 19.2
    x = np.arange(4)
    y = np.array([23.4, 22.6, 21.4, 19.2])
    plt.xticks([0, 1, 2, 3], ['1', '2', '4', '8'])
    plt.bar(x, y, color='tab:blue', width=0.5)
    plt.xlabel('Number of L-apps')
    plt.ylabel('Throughput (Gbps)')
    # Plot a horizontal dashed line
    plt.axhline(y=26.2, color='r', linestyle='--')
    # Add a text above the horizontal line
    plt.text(0.5, 26.2, 'isolated', ha='center', va='bottom', color='r')

def l_app_many_util_nw():
    # 97%		92%
    # 99		90%
    # 100%		90%
    # 100%		90%
    x = np.arange(4)
    y1 = np.array([97, 99, 100, 100])
    y2 = np.array([92, 90, 90, 90])

    # Plot bar chart of y1 and y2 side-by-side with x as x-axis 
    plt.bar(x - 0.2, y1, color = 'tab:purple', width = 0.4)
    plt.bar(x + 0.2, y2, color = 'tab:olive', width = 0.4)

    # Add xticks on the middle of the group bars
    plt.xlabel('Number of L-apps')
    plt.ylabel('CPU Utilization (%)')
    plt.xticks([0, 1, 2, 3], ['1', '2', '4', '8'])
    # Set yticks
    plt.yticks([20, 40, 60, 80, 100])

    # Set legend
    plt.legend(labels=['host', 'target'], loc='upper center', bbox_to_anchor=(0.5, 1.15), ncol=2)
    
def l_app_lat_nw_many():
    # 1	310	430
    # 2	340	490
    # 3	1160	2780
    # 4	1400	4000
    x = np.arange(4)
    y = np.array([310, 340, 1160, 1400])
    # Add error bar on y
    yerr = np.array([430 - 310, 490 - 340, 2780 - 1160, 4000 - 1400])
    lolims = np.array([0, 0, 0, 0], dtype=bool)
    plt.errorbar(x, y, yerr=yerr, fmt='o', color='k', capsize=5, zorder=1, lolims=lolims)
    plt.xticks([0, 1, 2, 3], ['1', '2', '3', '4'])
    # draw the bar plot on top of the error bars
    plt.bar(x, y, color='tab:blue', width=0.5, zorder=2)
    plt.xlabel('Number of Nw apps')
    plt.ylabel('Latency (us)')
    # Make y axis logarithmic and lower value to 0 
    plt.yscale('log')
    plt.ylim(bottom=10)
    plt.yticks([10, 100, 1000, 10000])

def l_app_nw_many_bw():
    # 19.2
    # 12.5
    # 7.4
    # 5.4
    x = np.arange(4)
    y1 = np.array([26.2, 30.5 / 2, 21 / 3, 21 / 4])
    y2 = np.array([19.2, 12.5, 7.4, 5.4])
    plt.xticks([0, 1, 2, 3], ['1', '2', '3', '4'])
    plt.bar(x - 0.2, y1, color = 'tab:blue', width = 0.4, label='without L-apps')
    plt.bar(x + 0.2, y2, color = 'tab:orange', width = 0.4, label='with L-apps')
    plt.xlabel('Number of Nw apps')
    plt.ylabel('Throughput (Gbps)')
    plt.legend()

if __name__ == '__main__':
    # Set font size 
    plt.rcParams.update({'font.size': 14})
    l_app_nw_many_bw()
    plt.show()