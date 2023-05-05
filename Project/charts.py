import matplotlib.pyplot as plt
import numpy as np

def rem_storage_one_to_one_throughput():
    x = range(1,5)
    y1 = np.array([910, 1690 / 2, 2980 / 4, 2850 / 8])
    y2 = np.array([910, 1690, 2980, 2850])
    plt.xticks([1, 2, 3, 4], ['1', '2', '4', '8'])
    plt.bar(x, y1, alpha=0.8, color='blue')
    plt.bar(x,y2, alpha=0.5, color='red')
    plt.xlabel('Number of apps')
    plt.ylabel('Total throughput (MiBps)')

def rem_storage_one_to_one_util():
    x = np.arange(4)
    y1 = np.array([40, 38, 39, 26])
    y2 = np.array([37, 35, 32, 16])

    # Plot bar chart of y1 and y2 side-by-side with x as x-axis 
    plt.bar(x - 0.2, y1, color = 'b', width = 0.4)
    plt.bar(x + 0.2, y2, color = 'g', width = 0.4)

    # Add xticks on the middle of the group bars
    plt.xlabel('Number of apps')
    plt.ylabel('CPU Utilization (%)')
    plt.xticks([0, 1, 2, 3], ['1', '2', '4', '8'])
    # Set yticks
    plt.yticks([10, 20, 30, 40, 50])

    # Set legend
    plt.legend(labels=['sender', 'receiver'])


def storage_many_one_to_one_nw_one():
    x = np.arange(4)
    y1 = np.array([750, 701, 480, 263])
    y2 = np.array([0, 940, 550, 285])

    # Plot bar chart of y1 and y2 side-by-side with x as x-axis 
    plt.bar(x - 0.2, y1, color = 'tab:blue', width = 0.4)
    plt.bar(x + 0.2, y2, color = 'tab:orange', width = 0.4)

    # Add xticks on the middle of the group bars
    plt.xlabel('Number of storage T-apps')
    plt.ylabel('T-app bandwidth (MBps)')
    plt.xticks([0, 1, 2, 3], ['1', '2', '4', '8'])
    # Set yticks
    plt.yticks([200, 400, 600, 800, 1000])

    # Set legend
    plt.legend(labels=['in-contention', 'non-contention'])


# Nw bandwidth in contention (Gbps)
def storage_many_one_to_one_nw_bw():
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

def nw_one_to_one():
    x = np.arange(4)
    y1 = np.array([26.2, 30.5 / 2, 21 / 3, 21 / 4])
    y2 = np.array([26.2, 30.5, 21, 21])
    plt.xticks([0, 1, 2, 3], ['1', '2', '4', '8'])
    plt.bar(x, y1, color='tab:blue')
    plt.bar(x,y2, alpha=0.5, color='cornflowerblue')
    plt.xlabel('Number of flows')
    plt.ylabel('Total throughput (Gbps)')
    plt.legend(labels=['per-flow', 'total'])

def l_app_lat():
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
    # Plot y as bar and yerror as a cap line
    plt
    plt.errorbar(x, y, yerr=yerr, fmt='o', color='k', capsize=5, zorder=1)
    plt.xticks([0, 1, 2, 3, 4], ['isolated', '1', '2', '4', '8'])
    # draw the bar plot on top of the error bars
    plt.bar(x, y, color='tab:blue', width=0.5, zorder=2)
    plt.xlabel('Number of L-apps')
    plt.ylabel('Latency (us)')

if __name__ == '__main__':
    # Set font size 
    plt.rcParams.update({'font.size': 14})
    l_app_lat()
    plt.show()