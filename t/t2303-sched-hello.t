#!/bin/sh

test_description='test sched hello

Ensure that a job that the scheduler rejects during hello processing
receives a fatal exception.
'

# Append --logfile option if FLUX_TESTS_LOGFILE is set in environment:
test -n "$FLUX_TESTS_LOGFILE" && set -- "$@" --logfile
. $(dirname $0)/sharness.sh

test_under_flux 1 job

test_expect_success 'start a long-running job' '
	jobid=$(flux submit -n1 -t1h sleep 3600)
'
test_expect_success 'unload scheduler' '
	flux module remove sched-simple
'
test_expect_success 'exclude the job node from configuration' '
	flux config load <<-EOT
	[resource]
	exclude = "0"
	EOT
'
test_expect_success 'increase broker stderr log level' '
	flux setattr log-stderr-level 6
'
test_expect_success 'load scheduler' '
	flux module load sched-simple
'
test_expect_success 'job receives exception' '
	flux job wait-event -t 30 ${jobid} exception
'
test_expect_success 'job receives clean event' '
	flux job wait-event -v -t 30 ${jobid} clean
'
test_expect_success 'decrease broker stderr log level' '
	flux setattr log-stderr-level 5
'

test_done
