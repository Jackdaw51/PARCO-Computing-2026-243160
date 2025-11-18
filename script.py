#Create an array of static_1 files of increasing thread number

import os
import re
import matplotlib.pyplot as plt
import numpy as np

thread_numbers = [1, 8, 16, 32, 64, 96]
types = ["FOR","STATIC","DYNAMIC","GUIDED"]
chunk_sizes = [1,10,100,1000]

#Get row size by looking to filenames in output folder
files = os.listdir("output")
row_size = [f for f in files if f.startswith("1_")]

#add to a set the second number after the first underscore
row_size = set(int(f.split("_")[1].split(".")[0]) for f in row_size)

row_size = list(row_size)
row_size.sort()
#if doesn't exist create pics/mtx folders
for mtx in row_size:
    dir_path = f"pics/{mtx}"
    if not os.path.exists(dir_path):
        os.makedirs(dir_path)
print(row_size)

array_of_times = []
for mtx in row_size:
    for ty in types:
        for cs in chunk_sizes:
            times = []
            for tn in thread_numbers:
                if ty == "FOR" and tn == 1 :
                    filename = f"output/{tn}_11949.bgn"
                elif ty == "FOR" and tn > 1:
                    filename = f"output/{tn}_11949_{ty}.bgn"
                else:                
                    filename = f"output/{tn}_11949_{ty}_{cs}.bgn"
                try:
                    with open(filename, "r") as f:
                        content = f.read()
                except FileNotFoundError:
                    print(f"Warning: {filename} not found; inserting NaN")
                    times.append(np.nan)
                    continue

                # extract first floating number (e.g. "Time taken ...: 123.45 ms")
                m = re.search(r'([-+]?\d*\.\d+|\d+)', content)
                if m:
                    try:
                        times.append(float(m.group(0)))
                        continue
                    except ValueError:
                        pass

                # fallback: try last non-empty line as a number
                lines = [ln.strip() for ln in content.splitlines() if ln.strip()]
                if lines:
                    try:
                        times.append(float(lines[-1]))
                    except ValueError:
                        print(f"Warning: could not parse numeric time in {filename}; inserting NaN")
                        times.append(np.nan)
                else:
                    print(f"Warning: {filename} is empty; inserting NaN")
                    times.append(np.nan)

            # scatter once per chunk size so each chunk gets its own color/legend entry
            if ty == "FOR":
                plt.scatter(thread_numbers, times, s=100)
                continue
            plt.scatter(thread_numbers, times, label=f"chunk={cs}", s=100)
        plt.xlabel("Number of Threads")
        plt.ylabel("Time (ms)")
        plt.title("Static Scheduling (different chunk sizes)")
        plt.xticks(thread_numbers)
        plt.grid()
        if ty != "FOR":
            plt.legend(title="Chunk size")
        plt.savefig(f"pics/{mtx}/{ty}_scheduling_times.png", dpi=150, bbox_inches='tight')
        plt.clf()  # Clear figure for next type
