import matplotlib.pyplot as plt

# Write a matplotlib code to plot bar chart
# for the following data:
# 1	1105
# 2	1588
# 4	2680
# 8	2880
def main():
    import matplotlib.pyplot as plt
    import numpy as np
    # Set labels for x axis
    plt.xlabel('Number of apps')
    plt.ylabel('Total throughput (MiBps)')
    # Use x-ticks to set the values for x axis
    # range 1- 4
    x = range(1,5)
    y = np.array([1105,1588,2680,2880])
    # Set x-ticks values
    plt.xticks([1, 2, 4, 8])
    plt.bar(x,y)
    plt.show()

if __name__ == '__main__':
    main()