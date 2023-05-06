#!/bin/sh

test_description='Test alloc-bypass job manager plugin'

. $(dirname $0)/sharness.sh

test_under_flux 2 job

flux version | grep -q libflux-security && test_set_prereq FLUX_SECURITY

flux setattr log-stderr-level 1

submit_as_alternate_user()
{
        FAKE_USERID=42
        test_debug "echo running flux run $@ as userid $FAKE_USERID"
        flux run --dry-run "$@" | \
	  flux python ${SHARNESS_TEST_SRCDIR}/scripts/sign-as.py $FAKE_USERID \
            >job.signed
        FLUX_HANDLE_USERID=$FAKE_USERID \
          flux job submit --flags=signed job.signed
}

test_expect_success 'alloc-bypass: start a job using all resources' '
	SLEEPID=$(flux submit --wait-event=start \
	            -n $(flux resource list -s up -no {ncores}) \
	            sleep 300)
'
test_expect_success 'alloc-bypass: load alloc-bypass plugin' '
	flux jobtap load alloc-bypass.so
'
test_expect_success 'alloc-bypass: works' '
	flux resource list &&
	run_timeout 15 \
	    flux run \
	        --setattr=system.alloc-bypass.R="$(flux kvs get resource.R)" \
	        -o per-resource.type=node hostname
'
test_expect_success 'alloc-bypass: works with per-resource.type=core' '
	flux run \
	    --setattr=system.alloc-bypass.R="$(flux kvs get resource.R)" \
	    -o per-resource.type=core hostname
'
test_expect_success 'alloc-bypass: works with jobid' '
	flux run \
	    --setattr=system.alloc-bypass.R="$(flux job info $SLEEPID R)" \
	    -o per-resource.type=node flux getattr rank
'
test_expect_success FLUX_SECURITY 'alloc-bypass: guest user not allowed' '
	test_must_fail submit_as_alternate_user \
	  --setattr=system.alloc-bypass.R="$(flux kvs get resource.R)" \
          hostname
'
test_expect_success 'alloc-bypass; invalid alloc-bypass.R type fails' '
	test_must_fail flux run \
	    --setattr=system.alloc-bypass.R=1 \
	    -o per-resource.type=node hostname
'
test_expect_success 'alloc-bypass: invalid alloc-bypass.R object fails' '
	test_must_fail flux run \
	    --setattr=system.alloc-bypass.R="{\"version\": 1}" \
	    -o per-resource.type=node hostname
'
test_expect_success 'alloc-bypass: handles exception before alloc event' '
	jobid=$(flux submit \
	        --setattr=system.alloc-bypass.R="$(flux kvs get resource.R)" \
		--dependency=afterok:$SLEEPID \
		-o per-resource.type=node hostname) &&
	flux job wait-event -vt 15 $jobid dependency-add &&
	flux cancel $jobid &&
	test_must_fail flux job attach -vEX $jobid
'
test_expect_success 'alloc-bypass: kill sleep job' '
	flux cancel --all &&
	flux job wait-event $SLEEPID clean
'
test_expect_success 'alloc-bypass: submit an alloc-bypass job' '
	flux submit -vvv --wait-event=start --job-name=bypass \
		--setattr=alloc-bypass.R="$(flux R encode -r 0)" \
		-n 1 \
		sleep 300
'
test_expect_success 'alloc-bypass: a full system job can still be run' '
	run_timeout 15 \
	  flux run -n $(flux resource list -s up -no {ncores}) hostname
'
test_expect_success 'kill bypass job' '
	flux pkill bypass
'
test_done
