#!/bin/bash

# /*
# Original Credits: (Thanks for making it publicly available!)
# https://github.com/casperIITB/ChampSim
# https://github.com/ChampSim/ChampSim

# Adapted By:
# Akash Maji (akashmaji@iisc.ac.in)
# https://github.com/akashmaji946/ChampSim-IISc
# */


if [ "$#" -lt 4 ]; then
    echo "Illegal number of parameters. Choose 4."
    echo "Usage: ./run_cloudsuite_iisc.sh [BINARY] [N_WARM] [N_SIM] [TRACE] [OPTION]"
    exit 1
fi

# look for traces here
TRACE_DIR=$PWD/dpc3_traces

BINARY=${1}
N_WARM=${2}
N_SIM=${3}
TRACE=${4}
OPTION=${5}

# Sanity check, yeah of course
if [ -z $TRACE_DIR ] || [ ! -d "$TRACE_DIR" ] ; then
    echo "[ERROR] Cannot find a trace directory: $TRACE_DIR"
    exit 1
fi

if [ ! -f "bin/$BINARY" ] ; then
    echo "[ERROR] Cannot find a ChampSim binary: bin/$BINARY"
    exit 1
fi

re='^[0-9]+$'
if ! [[ $N_WARM =~ $re ]] || [ -z $N_WARM ] ; then
    echo "[ERROR]: Number of warmup instructions is NOT a number" >&2;
    exit 1
fi

re='^[0-9]+$'
if ! [[ $N_SIM =~ $re ]] || [ -z $N_SIM ] ; then
    echo "[ERROR]: Number of simulation instructions is NOT a number" >&2;
    exit 1
fi

if [ ! -f "$TRACE_DIR/$TRACE" ] ; then
    echo "[ERROR] Cannot find a trace file: $TRACE_DIR/$TRACE"
    exit 1
fi

mkdir -p results_cloudsuite_akashmaji${N_SIM}M
(./bin/${BINARY} -warmup_instructions ${N_WARM}000000 -simulation_instructions ${N_SIM}000000 -c -traces ${TRACE_DIR}/${TRACE}) &> results_cloudsuite_akashmaji${N_SIM}M/${TRACE}-${BINARY}${OPTION}.txt

# Build 4 predictors as:
# ./build_champsim_iisc.sh bimodal no no no next_line lru 1
# ./build_champsim_iisc.sh gshare no no no next_line lru 1
# ./build_champsim_iisc.sh perceptron no no no next_line lru 1
# ./build_champsim_iisc.sh tage no no no next_line lru 1

# Run 3 traces with bimodal
# sudo ./run_cloudsuite_iisc.sh bimodal-no-no-no-next_line-lru-1core 10 500 trace1.xz
# sudo ./run_cloudsuite_iisc.sh bimodal-no-no-no-next_line-lru-1core 10 500 trace2.xz
# sudo ./run_cloudsuite_iisc.sh bimodal-no-no-no-next_line-lru-1core 10 500 trace3.xz

# Run 3 traces with gshare
# sudo ./run_cloudsuite_iisc.sh gshare-no-no-no-next_line-lru-1core 10 500 trace1.xz
# sudo ./run_cloudsuite_iisc.sh gshare-no-no-no-next_line-lru-1core 10 500 trace2.xz
# sudo ./run_cloudsuite_iisc.sh gshare-no-no-no-next_line-lru-1core 10 500 trace3.xz

# Run 3 traces with perceptron
# sudo ./run_cloudsuite_iisc.sh perceptron-no-no-no-next_line-lru-1core 10 500 trace1.xz
# sudo ./run_cloudsuite_iisc.sh perceptron-no-no-no-next_line-lru-1core 10 500 trace2.xz
# sudo ./run_cloudsuite_iisc.sh perceptron-no-no-no-next_line-lru-1core 10 500 trace3.xz

# Run 3 traces with tage
# sudo ./run_cloudsuite_iisc.sh tage-no-no-no-next_line-lru-1core 10 500 trace1.xz
# sudo ./run_cloudsuite_iisc.sh tage-no-no-no-next_line-lru-1core 10 500 trace2.xz
# sudo ./run_cloudsuite_iisc.sh tage-no-no-no-next_line-lru-1core 10 500 trace3.xz