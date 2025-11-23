# PARCO-Computing-2026-243160
#How to run
1. Connect to the cluster in unitn
2. Once inside, enter into a interactive session using `qsub -I -q short_cpuQ`
3. When you are inside the node run `lscpu` to make sure that the cpus match the one written in the paper
4. Run `bash run.sh` for the OpenMP implementation and `bash run1.sh` for my implementation
5. The result files will be inside either `output` or `output_my_impl`
6. If you have `python3` installed you can also run the script to get the graphs `python3 <chosenscript>`
