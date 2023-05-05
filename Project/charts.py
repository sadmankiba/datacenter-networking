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
    plt.show()

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
# 18.1
# 16.6
# 16.7
# 17.5
# 19.3
def storage_many_one_to_one_nw_bw():
    pass

if __name__ == '__main__':
    storage_many_one_to_one_nw_one()
    plt.show()