
import os
import re
import matplotlib.pyplot as plt
import numpy as np

thread_numbers = [1, 8, 16, 32, 64, 96]
types = "FOR"
#Get row size by looking to filenames in output folder
files = os.listdir("output")
row_size = 22295  #specifically for matrix of size 22295

dir_path = f"pics_bar/"
if not os.path.exists(dir_path):
    os.makedirs(dir_path)

#we want to create a bar chart comparing the times for different thread numbers for the FOR type and my_impl type for the matrix of size 22295
for tn in thread_numbers:
    for_values = []
    my_impl_values = []
    filename = f"output/{tn}_{row_size}_FOR.bgn"
    if tn == 1:
        filename = f"output/{tn}_{row_size}.bgn"
    try:
        with open(filename, "r") as f:
            content = f.read()
            for_values.append(float(re.search(r'([-+]?\d*\.\d+|\d+)', content).group(0)))
    except FileNotFoundError:
        print(f"Warning: {filename} not found; inserting NaN")
        for_values.append(np.nan)
        continue
    filename = f"output_my_impl/{tn}_{row_size}.bgn"
    try:
        with open(filename, "r") as f:
            content = f.read()
            my_impl_values.append(float(re.search(r'([-+]?\d*\.\d+|\d+)', content).group(0)))
    except FileNotFoundError:
        print(f"Warning: {filename} not found; inserting NaN")
        my_impl_values.append(np.nan)
        continue
    plt.bar(tn - 2, for_values[0], width=4, label='FOR', color='b')
    plt.bar(tn + 2, my_impl_values[0], width=4, label='My Implementation', color='r')
plt.xlabel("Number of Threads")
plt.ylabel("Time (ms)")
plt.title(f"Comparison of FOR vs My Implementation for matrix size {row_size}")
plt.xticks(thread_numbers)
#legend just once in total, not once per thread number
plt.legend(["FOR", "My Implementation"])
plt.grid()
plt.savefig(f"pics_bar/for_vs_my_impl_comparison.png", dpi=150, bbox_inches='tight')
plt.clf()  # Clear figure for next type 