import pandas as pd
import matplotlib.pyplot as plt
import os

CSV_FILE = "results_averaged.csv"  
OUTPUT_DIR = "plots"

if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

try:
    df = pd.read_csv(CSV_FILE)
except FileNotFoundError:
    print(f"Error: Could not find {CSV_FILE}. Did you run the benchmark?")
    exit(1)

df.columns = df.columns.str.strip()

plt.figure(figsize=(10, 6))
plt.title("Strong Scaling Speedup (Fixed Problem Size)", fontsize=14)
plt.xlabel("Number of MPI Ranks", fontsize=12)
plt.ylabel("Speedup (T1 / Tn)", fontsize=12)
plt.grid(True, which="both", ls="--", alpha=0.7)

strong_df = df[df['Type'] == 'Strong']

matrices = strong_df['Dataset'].unique()

for mtx in matrices:
    subset = strong_df[strong_df['Dataset'] == mtx].sort_values(by="Ranks")
    
    # Find the baseline time (Rank 1)
    # If Rank 1 is missing, normalize to the smallest rank available
    if not subset.empty:
        baseline_row = subset.iloc[0] 
        base_time = baseline_row['AvgTime']
        base_rank = baseline_row['Ranks']
        
        # Calculate Speedup: S = T_base / T_n * (Rank_base)
        speedup = (base_time / subset['AvgTime']) * base_rank
        
        plt.plot(subset['Ranks'], speedup, marker='o', label=mtx)

# Ideal speedup
max_rank = df['Ranks'].max()
plt.plot([1, max_rank], [1, max_rank], 'k--', label="Ideal Linear Speedup")

plt.legend()
plt.xscale('log', base=2)
plt.yscale('log', base=2)
plt.savefig(f"{OUTPUT_DIR}/strong_scaling_speedup.png")
print(f"Generated {OUTPUT_DIR}/strong_scaling_speedup.png")

# Ideally, Time stays constant as we scale, so Efficiency should stay near 1.0 

plt.figure(figsize=(10, 6))
plt.title("Weak Scaling Efficiency (Scaled Problem Size)", fontsize=14)
plt.xlabel("Number of MPI Ranks", fontsize=12)
plt.ylabel("Efficiency (T1 / Tn)", fontsize=12)
plt.ylim(0, 1.2) 
plt.grid(True, ls="--", alpha=0.7)

weak_df = df[df['Type'] == 'Weak'].sort_values(by="Ranks")

if not weak_df.empty:
    base_time_weak = weak_df.iloc[0]['AvgTime']
    
    efficiency = base_time_weak / weak_df['AvgTime']
    
    plt.plot(weak_df['Ranks'], efficiency, marker='s', color='green', label='Synthetic Matrix')
    
    plt.axhline(y=1.0, color='k', linestyle='--', label="Ideal (Constant Time)")

    plt.legend()
    plt.xscale('log', base=2)
    plt.savefig(f"{OUTPUT_DIR}/weak_scaling_efficiency.png")
    print(f"Generated {OUTPUT_DIR}/weak_scaling_efficiency.png")
else:
    print("Warning: No Weak Scaling data found in CSV.")

plt.figure(figsize=(10, 6))
plt.title("Raw Performance (GFLOPS)", fontsize=14)
plt.xlabel("Number of MPI Ranks", fontsize=12)
plt.ylabel("GFLOPS", fontsize=12)
plt.grid(True, ls="--", alpha=0.7)

for mtx in matrices:
    subset = strong_df[strong_df['Dataset'] == mtx].sort_values(by="Ranks")
    plt.plot(subset['Ranks'], subset['AvgGFLOPS'], marker='o', linestyle='-', label=f"{mtx} (Strong)")

if not weak_df.empty:
    plt.plot(weak_df['Ranks'], weak_df['AvgGFLOPS'], marker='s', linestyle='--', linewidth=2, color='black', label="Synthetic (Weak)")

plt.legend()
plt.xscale('log', base=2)
plt.yscale('log', base=10) # Log scale 
plt.savefig(f"{OUTPUT_DIR}/gflops_performance.png")
print(f"Generated {OUTPUT_DIR}/gflops_performance.png")

plt.close('all')