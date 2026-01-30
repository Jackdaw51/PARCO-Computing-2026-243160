#Create an array of static_1 files of increasing thread number

import os
import re
import matplotlib.pyplot as plt
import numpy as np

thread_numbers = [1, 8, 16, 32, 64, 96]
#Get row size by looking to filenames in output_my_impl folder
files = os.listdir("output_my_impl")
row_size = [f for f in files if f.startswith("1_")]


#add to a set the second number after the first underscore
row_size = set(int(f.split("_")[1].split(".")[0]) for f in row_size)

row_size = list(row_size)
row_size.sort()
#if doesn't exist create pics/mtx folders
for mtx in row_size:
    dir_path = f"pics_my_impl/{mtx}"
    if not os.path.exists(dir_path):
        os.makedirs(dir_path)
print(row_size)
for mtx in row_size:
    array_of_avgs = []
    array_of_perms = []
    array_of_sorts = []
    array_of_inv_perms = []
    array_of_total_times = []
    for tn in thread_numbers:
        filename = f"output_my_impl/{tn}_{mtx}.bgn"
        try:
            with open(filename, "r") as f:
                content = f.read()
        except FileNotFoundError:
            print(f"Warning: {filename} not found; inserting NaN")
            times.append(np.nan)
            continue

        # extract floating number 
        # format is 
        # avg time: XXX.XXXX ms
        # permutation time: XXX.XXXX ms
        # sorting time: XXX.XXXX ms
        # inverse permutation time: XXX.XXXX ms

        #search for strings separated by new lines
        values = re.findall(r'([-+]?\d*\.\d+|\d+)', content)
        if values and len(values) >= 4:
            array_of_avgs.append(float(values[0]))
            array_of_perms.append(float(values[1]))
            array_of_sorts.append(float(values[2]))
            array_of_inv_perms.append(float(values[3]))
            array_of_total_times.append(float(values[0]) + float(values[1]) + float(values[2]) + float(values[3]))
        else:
            print(f"Warning: No time found in {filename}; inserting NaN")
            array_of_avgs.append(np.nan)
            array_of_perms.append(np.nan)
            array_of_sorts.append(np.nan)
            array_of_inv_perms.append(np.nan)
            array_of_total_times.append(np.nan)
        print(mtx)
        print(thread_numbers)
        print(array_of_avgs)
    plt.scatter(thread_numbers, array_of_avgs, s=100)
    plt.scatter(thread_numbers, array_of_perms, s=100)
    plt.scatter(thread_numbers, array_of_sorts, s=100)
    plt.scatter(thread_numbers, array_of_inv_perms, s=100)
    plt.scatter(thread_numbers, array_of_total_times, s=100)
    plt.xlabel("Number of Threads")
    plt.ylabel("Time (ms)")
    plt.title("My implementation")
    plt.xticks(thread_numbers)
    plt.legend(["Average SpMV Time", "Permutation Time", "Sorting Time", "Inverse Permutation Time", "Total Time"])
    plt.grid()
    plt.savefig(f"pics_my_impl/{mtx}/my_impl_times.png", dpi=150, bbox_inches='tight')
    plt.clf()  # Clear figure for next type
