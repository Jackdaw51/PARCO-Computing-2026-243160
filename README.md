# How to run

> **IMPORTANT NOTE:**
> These instructions are for **hpc2**.
>
> If running on **hpc3**:
> * Use `shortcpuQ` instead of `short_cpuQ`.
> * You may need to change the module loading in `script.pbs`.
> * You may need to change the resources in `script.pbs` as they might be handled differently

1.  Connect to the Unitn cluster.
2.  Clone the repository:
    ```bash
    git clone [https://github.com/Jackdaw51/PARCO-Computing-2026-243160](https://github.com/Jackdaw51/PARCO-Computing-2026-243160)
    ```
3.  Navigate into the directory:
    ```bash
    cd PARCO-Computing-2026-243160
    ```
4.  Submit the job:
    ```bash
    qsub -q short_cpuQ script.pbs
    ```
5.  The result files will be generated in `results_averaged.csv`.
6.  Download the results. Optional: Use python3 to generate graphs with `plot.py`.