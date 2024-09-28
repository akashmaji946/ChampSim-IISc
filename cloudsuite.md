### Running the 3 cloudsuite traces with ChampSim-IISc
- Trace1: nutch_phase2_core1.trace.xz   
- Trace2: cloud9_phase1_core1.trace.xz  
- Trace3: classification_phase0_core3.trace.xz


#### Build 4 predictors as:
```
# ./build_champsim_iisc.sh bimodal no no no next_line lru 1
# ./build_champsim_iisc.sh gshare no no no next_line lru 1
# ./build_champsim_iisc.sh perceptron no no no next_line lru 1
# ./build_champsim_iisc.sh tage no no no next_line lru 1
```

#### Run 3 traces with bimodal:
```
# sudo ./run_cloudsuite_iisc.sh bimodal-no-no-no-next_line-lru-1core 10 500 trace1.xz
# sudo ./run_cloudsuite_iisc.sh bimodal-no-no-no-next_line-lru-1core 10 500 trace2.xz
# sudo ./run_cloudsuite_iisc.sh bimodal-no-no-no-next_line-lru-1core 10 500 trace3.xz
```

#### Run 3 traces with gshare:
```
# sudo ./run_cloudsuite_iisc.sh gshare-no-no-no-next_line-lru-1core 10 500 trace1.xz
# sudo ./run_cloudsuite_iisc.sh gshare-no-no-no-next_line-lru-1core 10 500 trace2.xz
# sudo ./run_cloudsuite_iisc.sh gshare-no-no-no-next_line-lru-1core 10 500 trace3.xz
```

#### Run 3 traces with perceptron:
```
# sudo ./run_cloudsuite_iisc.sh perceptron-no-no-no-next_line-lru-1core 10 500 trace1.xz
# sudo ./run_cloudsuite_iisc.sh perceptron-no-no-no-next_line-lru-1core 10 500 trace2.xz
# sudo ./run_cloudsuite_iisc.sh perceptron-no-no-no-next_line-lru-1core 10 500 trace3.xz
```

#### Run 3 traces with tage:
```
# sudo ./run_cloudsuite_iisc.sh tage-no-no-no-next_line-lru-1core 10 500 trace1.xz
# sudo ./run_cloudsuite_iisc.sh tage-no-no-no-next_line-lru-1core 10 500 trace2.xz
# sudo ./run_cloudsuite_iisc.sh tage-no-no-no-next_line-lru-1core 10 500 trace3.xz
```