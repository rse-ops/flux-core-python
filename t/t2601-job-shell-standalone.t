#!/bin/sh
#
test_description='Test flux-shell in --standalone mode'

. `dirname $0`/sharness.sh

#  Run flux-shell under flux command to get correct paths
FLUX_SHELL="run_timeout 60 flux ${FLUX_BUILD_DIR}/src/shell/flux-shell"

PMI_INFO=${FLUX_BUILD_DIR}/src/common/libpmi/test_pmi_info
KVSTEST=${FLUX_BUILD_DIR}/src/common/libpmi/test_kvstest

unset FLUX_URI

test_expect_success 'flux-shell: generate 1-task jobspec and matching R' '
	flux mini run --dry-run -N1 -n1 echo Hi >j1 &&
	cat >R1 <<-EOT
	{"version": 1, "execution":{ "R_lite":[
		{ "children": { "core": "0" }, "rank": "0" }
        ]}}
	EOT
'
test_expect_success 'flux-shell: run 1-task echo job' '
	${FLUX_SHELL} -v -s -r 0 -j j1 -R R1 0 >echo.out 2>echo.err &&
	grep Hi echo.out
'
test_expect_success 'flux-shell: -v output includes expected task count' '
	grep task_count=1 echo.err
'
test_expect_success 'flux-shell: missing JOBID fails with Usage message' '
	test_must_fail ${FLUX_SHELL} -s -r 0 -j j1 -R R1 2>nojobid.err &&
	grep Usage nojobid.err
'
test_expect_success 'flux-shell: missing -r fails with standalone message' '
	test_must_fail ${FLUX_SHELL} -s -j j1 -R R1 0 2>no_r.err &&
	grep standalone no_r.err
'
test_expect_success 'flux-shell: missing -R fails with standalone message' '
	test_must_fail ${FLUX_SHELL} -s -r 0 -j j1 0 2>no_R.err &&
	grep standalone no_R.err
'
test_expect_success 'flux-shell: missing -j fails with standalone message' '
	test_must_fail ${FLUX_SHELL} -s -r 0 -R R1 0 2>no_j.err &&
	grep standalone no_j.err
'
test_expect_success 'flux-shell: nonexistent jobspec file fails' '
	! ${FLUX_SHELL} -v -s -r 0 -j /noexist -R R1 0 \
		>noexist.out 2>noexist.err &&
	grep "error opening" noexist.err
'
test_expect_success 'flux-shell: malformed jobid fails' '
	! ${FLUX_SHELL} -v -s -r 0 -j j1 -R R1 BADID \
		>badid.out 2>badid.err &&
	grep "jobid" badid.err
'
test_expect_success 'flux-shell: out of range jobid fails' '
	! ${FLUX_SHELL} -v -s -r 0 -j j1 -R R1 18446744073709551616 \
		>badid2.out 2>badid2.err &&
	grep "jobid" badid2.err
'
test_expect_success 'flux-shell: out of range broker rank fails' '
	test_must_fail ${FLUX_SHELL} -s -r 4294967296 -j j1 -R R1 0 2>er.err &&
	grep -i option er.err
'
test_expect_success 'flux-shell: wrong range broker rank fails' '
	test_must_fail ${FLUX_SHELL} -s -r 8 -j j1 -R R1 0 2>wr.err &&
	grep -i fetching wr.err
'
test_expect_success 'flux-shell: unknown argument fails' '
	test_must_fail ${FLUX_SHELL} --FOO -s -r 0 -j j1 -R R1 0 2>alien.err &&
	grep -i unrecognized alien.err
'
test_expect_success 'flux-shell: generate 2-task jobspec and matching R' '
	flux mini run -o cpu-affinity=off --dry-run -N1 -n2 printenv >j2 &&
	cat >R2 <<-EOT
	{"version": 1, "execution":{ "R_lite":[
		{ "children": { "core": "0-1" }, "rank": "0" }
        ]}}
	EOT
'
test_expect_success 'flux-shell: run 2-task printenv job' '
	${FLUX_SHELL} -v -s -r 0 -j j2 -R R2 42 \
		>printenv.out 2>printenv.err
'
test_expect_success 'flux-shell: 0: completed with no error' '
	grep "task 0 complete status=0" printenv.err
'
test_expect_success 'flux-shell: 1: completed with no error' '
	grep "task 1 complete status=0" printenv.err
'
test_expect_success 'flux-shell: 0: FLUX_TASK_LOCAL_ID, TASK_RANK set' '
	grep FLUX_TASK_LOCAL_ID=0 printenv.out &&
	grep FLUX_TASK_RANK=0 printenv.out
'
test_expect_success 'flux-shell: 1: FLUX_TASK_LOCAL_ID, TASK_RANK set' '
	grep FLUX_TASK_LOCAL_ID=1 printenv.out &&
	grep FLUX_TASK_RANK=1 printenv.out
'
test_expect_success 'flux-shell: FLUX_JOB_SIZE, JOB_NNODES, JOB_ID set' '
	grep FLUX_JOB_SIZE=2 printenv.out &&
	grep FLUX_JOB_NNODES=1 printenv.out &&
	(grep FLUX_JOB_ID=$(flux job id --to=f58 42) printenv.out ||
	 grep FLUX_JOB_ID=42 printenv.out)
'
test_expect_success 'flux-shell: FLUX_URI is not set by shell' '
	test_must_fail grep FLUX_URI printenv.out
'
test_expect_success 'flux-shell: 0: PMI_RANK set' '
	grep PMI_RANK=0 printenv.out
'
test_expect_success 'flux-shell: 1: PMI_RANK set' '
	grep PMI_RANK=1 printenv.out
'
test_expect_success 'flux-shell: PMI_SIZE, PMI_FD set' '
	grep PMI_SIZE=2 printenv.out &&
	grep PMI_FD= printenv.out
'
test_expect_success 'flux-shell: generate 8-task bash exit rank job' '
	flux mini run --dry-run -o cpu-affinity=off \
	   -N1 -n8 bash -c "exit \$FLUX_TASK_RANK" >j8 &&
	cat >R8 <<-EOT
	{"version": 1, "execution":{ "R_lite":[
		{ "children": { "core": "0-7" }, "rank": "0" }
        ]}}
	EOT
'
test_expect_success 'flux-shell: environ in jobspec is set for task' '
	flux mini run --dry-run --env=ENVTEST=foo printenv >je &&
	${FLUX_SHELL} -v -s -r 0 -j je -R R1 42 \
		>printenv2.out 2>printenv2.err &&
	grep ENVTEST=foo printenv2.out
'
test_expect_success 'flux-shell: shell PMI works' '
	flux mini run --dry-run -o cpu-affinity=off \
	  -N1 -n8 ${PMI_INFO} >j8pmi &&
	${FLUX_SHELL} -v -s -r 0 -j j8pmi -R R8 51 \
		>pmi_info.out 2>pmi_info.err
'
test_expect_success 'flux-shell: shell PMI exports clique info' '
	flux mini run --dry-run -N1 -n8 \
		${PMI_INFO} -c >j8pmi_clique &&
	${FLUX_SHELL} -v -s -r 0 -j j8pmi_clique -R R8 51 \
		>pmi_clique.out 2>pmi_clique.err &&
	COUNT=$(grep "clique=0,1,2,3,4,5,6,7" pmi_clique.out | wc -l) &&
	test ${COUNT} -eq 8
'
test_expect_success 'flux-shell: shell PMI KVS works' '
	flux mini run --dry-run -o cpu-affinity=off \
	  -N1 -n8 ${KVSTEST} > j8kvs &&
	${FLUX_SHELL} -v -s -r 0 -j j8kvs -R R8 52 \
		>kvstest.out 2>kvstest.err
'
test_expect_success NO_ASAN 'flux-shell: shell can launch flux' '
	flux mini run --dry-run -o cpu-affinity=off \
	    -N1 -n8 flux start flux getattr size >j8flux &&
	${FLUX_SHELL} -vv -s -r 0 -j j8flux -R R8 39 \
		>flux.out 2>flux.err &&
	grep -x "0: 8" flux.out
'

test_expect_success 'flux-shell: shell exits with highest task exit value' '
	test_expect_code 7 ${FLUX_SHELL} -v -s -r 0 -j j8 -R R8 69 \
		>exit.out 2>exit.err
'

test_expect_success 'flux-shell: shell forwards signals to tasks' '
	flux mini run --dry-run -n1 bash -c "kill \$PPID; sleep 10" > j9 &&
	test_expect_code  $((128+15)) \
		${FLUX_SHELL} -v -s -r 0 -j j9 -R R8 69 \
			>sigterm.out 2>sigterm.err &&
	grep "forwarding signal 15" sigterm.err
'

test_done
