#! /bin/bash

module load PE-gnu
module load singularity

IMAGE="/nfs/home/mfasel_alice/mfasel_cc7_alice.simg"
BINDS="-B /home:/home -B /nfs:/nfs -B"

execmd=""
for var in "$@"
do
    if [ "x$(echo $execmd)" == "x"]; then
        execmd=$var
    else
        execmd=$(printf "%s %s" "$execmd" "$var")
    fi 
done
# Export slurm environment to singularity
# We don't know which ones will be needed
# by the job so better export all
export SINGULARITYENV_SLURM_JOBID=$SLURM_JOBID
export SINGULARITYENV_SLURM_JOB_ID=$SLURM_JOB_ID
export SINGULARITYENV_SLURM_ARRAY_JOB_ID=$SLURM_ARRAY_JOB_ID
export SINGULARITYENV_SLURM_ARRAY_TASK_ID=$SLURM_ARRAY_TASK_ID
export SINGULARITYENV_SLURM_ARRAY_TASK_MAX=$SLURM_ARRAY_TASK_MAX
export SINGULARITYENV_SLURM_ARRAY_TASK_MIN=$SLURM_ARRAY_TASK_MIN
export SINGULARITYENV_SLURM_ARRAY_TASK_COUNT=$SLURM_ARRAY_TASK_COUNT
export SINGULARITYENV_SLURM_ARRAY_TASK_STEP=$SLURM_ARRAY_TASK_STEP
export SINGULARITYENV_SLURM_CLUSTER_NAME=$SLURM_CLUSTER_NAME
export SINGULARITYENV_SLURM_CPUS_ON_NODE=$SLURM_CPUS_ON_NODE
export SINGULARITYENV_SLURM_CPUS_PER_GPU=$SLURM_CPUS_PER_GPU
export SINGULARITYENV_SLURM_CPUS_PER_TASK=$SLURM_CPUS_PER_TASK
export SINGULARITYENV_SLURM_CONTAINER=$SLURM_CONTAINER
export SINGULARITYENV_SLURM_DIST_PLANESIZE=$SLURM_DIST_PLANESIZE
export SINGULARITYENV_SLURM_DISTRIBUTION=$SLURM_DISTRIBUTION
export SINGULARITYENV_SLURM_EXPORT_ENV=$SLURM_EXPORT_ENV
export SINGULARITYENV_SLURM_GPU_BIND=$SLURM_GPU_BIND
export SINGULARITYENV_SLURM_GPU_FREQ=$SLURM_GPU_FREQ
export SINGULARITYENV_SLURM_GPUS=$SLURM_GPUS
export SINGULARITYENV_SLURM_GPUS_ON_NODE=$SLURM_GPUS_ON_NODE
export SINGULARITYENV_SLURM_GPUS_PER_NODE=$SLURM_GPUS_PER_NODE
export SINGULARITYENV_SLURM_GPUS_PER_SOCKET=$SLURM_GPUS_PER_SOCKET
export SINGULARITYENV_SLURM_GPUS_PER_TASK=$SLURM_GPUS_PER_TASK
export SINGULARITYENV_SLURM_GTIDS=$SLURM_GTIDS
export SINGULARITYENV_SLURM_HET_SIZE=$SLURM_HET_SIZE
export SINGULARITYENV_SLURM_JOB_ACCOUNT=$SLURM_JOB_ACCOUNT
export SINGULARITYENV_SLURM_JOB_CPUS_PER_NODE=$SLURM_JOB_CPUS_PER_NODE
export SINGULARITYENV_SLURM_JOB_DEPENDENCY=$SLURM_JOB_DEPENDENCY
export SINGULARITYENV_SLURM_JOB_NAME=$SLURM_JOB_NAME
export SINGULARITYENV_SLURM_JOB_NODELIST=$SLURM_JOB_NODELIST
export SINGULARITYENV_SLURM_JOB_NUM_NODES=$SLURM_JOB_NUM_NODES
export SINGULARITYENV_SLURM_JOB_PARTITION=$SLURM_JOB_PARTITION
export SINGULARITYENV_SLURM_JOB_QOS=$SLURM_JOB_QOS
export SINGULARITYENV_SLURM_JOB_RESERVATION=$SLURM_JOB_RESERVATION
export SINGULARITYENV_SLURM_LOCALID=$SLURM_LOCALID
export SINGULARITYENV_SLURM_MEM_PER_CPU=$SLURM_MEM_PER_CPU
export SINGULARITYENV_SLURM_MEM_PER_GPU=$SLURM_MEM_PER_GPU
export SINGULARITYENV_SLURM_MEM_PER_NODE=$SLURM_MEM_PER_NODE
export SINGULARITYENV_SLURM_NNODES=$SLURM_NNODES
export SINGULARITYENV_SLURM_NODE_ALIASES=$SLURM_NODE_ALIASES
export SINGULARITYENV_SLURM_NODEID=$SLURM_NODEID
export SINGULARITYENV_SLURM_NODELIST=$SLURM_NODELIST
export SINGULARITYENV_SLURM_NPROCS=$SLURM_NPROCS
export SINGULARITYENV_SLURM_NTASKS=$SLURM_NTASKS
export SINGULARITYENV_SLURM_NTASKS_PER_CORE=$SLURM_NTASKS_PER_CORE
export SINGULARITYENV_SLURM_NTASKS_PER_GPU=$SLURM_NTASKS_PER_GPU
export SINGULARITYENV_SLURM_NTASKS_PER_NODE=$SLURM_NTASKS_PER_NODE
export SINGULARITYENV_SLURM_NTASKS_PER_SOCKET=$SLURM_NTASKS_PER_SOCKET
export SINGULARITYENV_SLURM_OVERCOMMIT=$SLURM_OVERCOMMIT
export SINGULARITYENV_SLURM_PRIO_PROCESS=$SLURM_PRIO_PROCESS
export SINGULARITYENV_SLURM_PROCID=$SLURM_PROCID
export SINGULARITYENV_SLURM_PROFILE=$SLURM_PROFILE
export SINGULARITYENV_SLURM_RESTART_COUNT=$SLURM_RESTART_COUNT
export SINGULARITYENV_SLURM_SUBMIT_DIR=$SLURM_SUBMIT_DIR
export SINGULARITYENV_SLURM_SUBMIT_HOST=$SLURM_SUBMIT_HOST
export SINGULARITYENV_SLURM_TASK_PID=$SLURM_TASK_PID
export SINGULARITYENV_SLURM_TASKS_PER_NODE=$SLURM_TASKS_PER_NODE
export SINGULARITYENV_SLURM_THREADS_PER_CORE=$SLURM_THREADS_PER_CORE
export SINGULARITYENV_SLURM_TOPOLOGY_ADDR=$SLURM_TOPOLOGY_ADDR
export SINGULARITYENV_SLURM_TOPOLOGY_ADDR_PATTERN=$SLURM_TOPOLOGY_ADDR_PATTERN
export SINGULARITYENV_SLURMD_NODENAME=$SLURMD_NODENAME
# Build containercommand
containercmd=$(printf "singularity exec %s %s %s" "$BINDS" $CONTAINER "$execmd")
eval $containercmd