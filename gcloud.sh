#!/bin/bash

# You probably want to have your keychain already set up before running this
# script, or you'll have to enter your SSH key password 4x.

set -e

INSTANCE_NAME=parallel-sort-bench
ZONE=us-west1-a

function build {
  bazelisk build -c opt --cxxopt=-march=icelake-server --cxxopt=-DENABLE_INTEL_X86_SIMD_SORT "$@"
}

function start {
  gcloud beta compute instances create ${INSTANCE_NAME} \
    --zone ${ZONE} \
    --provisioning-model=SPOT \
    --instance-termination-action=DELETE \
    --min-cpu-platform='Intel Ice Lake' \
    --image=ubuntu-minimal-2210-kinetic-amd64-v20230214 \
    --image-project=ubuntu-os-cloud \
    --machine-type=n2-standard-8 \
    --max-run-duration=10m
}

function delete {
  gcloud compute instances delete ${INSTANCE_NAME} -q --zone ${ZONE}
}

function ssh {
  gcloud compute ssh ${INSTANCE_NAME} --zone ${ZONE} -- "$@"
}

function run-bench {
  build :bench
  gcloud compute scp bazel-bin/bench ${INSTANCE_NAME}:/tmp --zone ${ZONE}
  ssh bash -c "/tmp/bench --benchmark_counters_tabular=true; echo done, deleting /tmp/bench; rm -f /tmp/bench"
}

function run-test {
  build :test
  gcloud compute scp bazel-bin/test ${INSTANCE_NAME}:/tmp --zone ${ZONE}
  ssh bash -c "/tmp/test; rm -f /tmp/test"
}

if [[ $# -eq 0 ]]; then
  build :bench :test  ## Build first
  start
  run-test
  run-bench
  delete
else
  for cmd in "$@"; do
    "${cmd}"
  done
fi
