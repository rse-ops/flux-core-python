#!/bin/sh
#
test_description='Test flux-shell logging implementation'

. `dirname $0`/sharness.sh

test_under_flux 2 job

FLUX_SHELL="${FLUX_BUILD_DIR}/src/shell/flux-shell"

INITRC_TESTDIR="${SHARNESS_TEST_SRCDIR}/shell/initrc"
INITRC_PLUGINPATH="${SHARNESS_TEST_DIRECTORY}/shell/plugins/.libs"

# test initrc files need to be able to find fluxometer.lua:
export LUA_PATH="$LUA_PATH;${SHARNESS_TEST_DIRECTORY}/?.lua"

test_expect_success 'flux-shell: log: generate 1-task jobspec and matching R' '
	flux mini run --dry-run -N1 -n1 echo Hi >j1 &&
	cat >R1 <<-EOT
	{"version": 1, "execution":{ "R_lite":[
		{ "children": { "core": "0,1" }, "rank": "0" }
        ]}}
	EOT
'
test_expect_success 'flux-shell: run log testing plugin' '
	name=log &&
	cat >${name}.lua <<-EOF &&
	plugin.searchpath = "${INITRC_PLUGINPATH}"
	plugin.load { file = "log.so" }
	EOF
	${FLUX_SHELL} -v -v -s -r 0 -j j1 -R R1 --initrc=${name}.lua 0 \
		> ${name}.log 2>&1 &&
	test_debug "cat ${name}.log"
'
for topic in "shell.init" "shell.exit" \
	     "task.init" "task.exit" \
	     "task.fork" "task.fork"; do
    test_expect_success "$topic: got TRACE level message" "
        grep \"TRACE: log: .*: $topic: trace message\" log.log
    "
    test_expect_success "$topic: got truncated long TRACE level message" "
        grep -q \"TRACE: log: .*: $topic: long message.*0000+$\" log.log
    "
    test_expect_success "$topic: got DEBUG level message" "
        grep \"DEBUG: log: .*: $topic: debug message\" log.log
    "
    test_expect_success "$topic: got INFO level message" "
        grep \" INFO: log: .*: $topic: log message\" log.log
    "
    test_expect_success "$topic: got WARN level message" "
        grep \" WARN: log: .*: $topic: warn message\" log.log
    "
    test_expect_success "$topic: got ERROR level message" "
        grep \"ERROR: log: .*: $topic: error message\" log.log
    "
    test_expect_success "$topic: got log_errn message" "
	grep \"ERROR:.*: $topic: log_errn message: Operation not permitted\" log.log
    "
    test_expect_success "$topic: got log_errno message" "
	grep \"ERROR: log: .*: $topic: log_errno message: Invalid argument\" log.log
    "
done

test_expect_success 'flux-shell: run job with verbose logging to output' '
	flux mini run -o verbose=2 -o initrc=log.lua -n2 -N2 hostname \
		>log-test.output 2>log-test.err
'

for topic in "shell.init" "shell.exit" \
	     "task.init" "task.exit" \
	     "task.fork" "task.fork"; do
    test_expect_success "$topic: got TRACE level message in output eventlog" "
        grep \"flux-shell\[0\]: TRACE: log: $topic: trace message\" log-test.err&&
        grep \"flux-shell\[1\]: TRACE: log: $topic: trace message\" log-test.err
    "
    test_expect_success "$topic: got DEBUG level message in output eventlog" "
        grep \"flux-shell\[0\]: DEBUG: log: $topic: debug message\" log-test.err&&
        grep \"flux-shell\[1\]: DEBUG: log: $topic: debug message\" log-test.err
    "
    test_expect_success "$topic: got INFO level message in output eventlog" "
        grep \"flux-shell\[0\]: log: $topic: log message\" log-test.err &&
        grep \"flux-shell\[1\]: log: $topic: log message\" log-test.err
    "
    test_expect_success "$topic: got WARN level message in output eventlog" "
        grep \"flux-shell\[0\]:  WARN: log: $topic: warn message\" log-test.err &&
        grep \"flux-shell\[1\]:  WARN: log: $topic: warn message\" log-test.err
    "
    test_expect_success "$topic: got ERROR level message in output eventlog" "
        grep \"flux-shell\[0\]: ERROR: log: $topic: error message\" log-test.err &&
        grep \"flux-shell\[1\]: ERROR: log: $topic: error message\" log-test.err
    "
    test_expect_success "$topic: got log_errn message in output eventlog" "
	grep \"flux-shell\[0\]: ERROR: log: $topic: log_errn message: Operation not permitted\" log-test.err &&
	grep \"flux-shell\[1\]: ERROR: log: $topic: log_errn message: Operation not permitted\" log-test.err
    "
    test_expect_success "$topic: got log_errno message in output eventlog" "
	grep \"flux-shell\[0\]: ERROR: log: $topic: log_errno message: Invalid argument\" log-test.err &&
	grep \"flux-shell\[1\]: ERROR: log: $topic: log_errno message: Invalid argument\" log-test.err
    "
done

test_expect_success 'flux-shell: run job with normal logging level' '
	flux mini run -o initrc=log.lua -n2 -N2 hostname \
		>log-test.output 2>log-test.err
'

for topic in "shell.init" "shell.exit" \
	     "task.init" "task.exit" \
	     "task.fork" "task.fork"; do
    test_expect_success "$topic: no TRACE level messages in output eventlog" "
        test_must_fail grep TRACE log-test.err
    "
    test_expect_success "$topic: no DEBUG level messages in output eventlog" "
	test_must_fail grep DEBUG log-test.err
    "
    test_expect_success "$topic: got INFO level message in output eventlog" "
        grep \"flux-shell\[0\]: log: $topic: log message\" log-test.err &&
        grep \"flux-shell\[1\]: log: $topic: log message\" log-test.err
    "
    test_expect_success "$topic: got WARN level message in output eventlog" "
        grep \"flux-shell\[0\]:  WARN: log: $topic: warn message\" log-test.err &&
        grep \"flux-shell\[1\]:  WARN: log: $topic: warn message\" log-test.err
    "
    test_expect_success "$topic: got ERROR level message in output eventlog" "
        grep \"flux-shell\[0\]: ERROR: log: $topic: error message\" log-test.err &&
        grep \"flux-shell\[1\]: ERROR: log: $topic: error message\" log-test.err
    "
    test_expect_success "$topic: got log_errn message in output eventlog" "
	grep \"flux-shell\[0\]: ERROR: log: $topic: log_errn message: Operation not permitted\" log-test.err &&
	grep \"flux-shell\[1\]: ERROR: log: $topic: log_errn message: Operation not permitted\" log-test.err
    "
    test_expect_success "$topic: got log_errno message in output eventlog" "
	grep \"flux-shell\[0\]: ERROR: log: $topic: log_errno message: Invalid argument\" log-test.err &&
	grep \"flux-shell\[1\]: ERROR: log: $topic: log_errno message: Invalid argument\" log-test.err
    "
done

test_expect_success 'flux-shell: missing command logs fatal error' '
	test_expect_code 127 flux mini run nosuchcommand 2>missing.err &&
	grep "flux-shell\[0\]: FATAL: task 0.*: start failed" missing.err &&
	grep "job.exception type=exec severity=0 task 0.*: start failed" missing.err &&
        grep "No such file or directory" missing.err
'

test_expect_success 'flux-shell: illegal command logs fatal error' '
	mkdir adirectory &&
	test_expect_code 126 flux mini run ./adirectory 2>illegal.err &&
	grep "flux-shell\[0\]: FATAL: task 0.*: start failed" illegal.err &&
	grep "job.exception type=exec severity=0 task 0.*: start failed" illegal.err &&
	grep "Permission denied" illegal.err
'

test_expect_success 'flux-shell: bad cwd emits message, but completes' '
	flux mini run --setattr="system.cwd=/foo" pwd 2>badcwd.err &&
        grep "Could not change dir to /foo: No such file or directory" badcwd.err
'

test_expect_success 'job-exec: set kill-timeout to low value for testing' '
	flux module reload job-exec kill-timeout=0.25
'

dump_job_output_eventlog() {
    flux job eventlog -p guest.output $(sed -n 's/^jobid: //p' fatal-$1.out)
}

test_expect_success 'flux-shell: fatal error in shell.init works' '
	site=shell.init &&
	test_when_finished "test_debug \"dump_job_output_eventlog $site\"" &&
	test_must_fail_or_be_terminated \
		flux mini run -vvv -o verbose=2 -n2 -N2 \
			-o initrc=log.lua \
			-o log-fatal-error=$site hostname \
		>fatal-${site}.out 2>&1 &&
	test_debug "cat fatal-${site}.out" &&
	id=$(sed -n "s/jobid: //p" fatal-${site}.out) &&
	test_must_fail_or_be_terminated flux job attach -XEl ${id} &&
	test_might_fail \
		grep "flux-shell\[1\]: FATAL: log: log-fatal-error requested!" \
			fatal-${site}.out &&
	dump_job_output_eventlog $site | grep log-fatal-error
'
test_expect_success 'flux-shell: fatal error in shell.exit works' '
	site=shell.exit &&
	echo "running mini run" &&
	test_must_fail_or_be_terminated \
		flux mini run -vvv -n2 -N2 \
			-o verbose=2 \
			-o initrc=log.lua \
                	-o log-fatal-error=$site hostname &&
	test $? -eq 0 &&
	echo "mini run done"
'
test_expect_success 'flux-shell: fatal error in task.init works' '
	site=task.init &&
	test_when_finished "test_debug \"dump_job_output_eventlog $site\"" &&
	test_must_fail_or_be_terminated \
		flux mini run -vvv -n2 -N2 \
                        -o verbose=2 \
			-o initrc=log.lua \
                	-o log-fatal-error=$site hostname \
		>fatal-${site}.out 2>&1 &&
	test_debug "cat fatal-${site}.out" &&
	id=$(sed -n "s/jobid: //p" fatal-${site}.out) &&
	test_must_fail_or_be_terminated flux job attach -XEl ${id} &&
	test_might_fail \
	    grep "log-fatal-error requested!" fatal-${site}.out &&
	dump_job_output_eventlog $site | grep log-fatal-error

'
test_expect_success 'flux-shell: fatal error in task.fork works' '
	site=task.fork &&
	test_when_finished "test_debug \"dump_job_output_eventlog $site\"" &&
	test_must_fail_or_be_terminated \
		flux mini run -v -n2 -N2 \
			-o initrc=log.lua \
                	-o log-fatal-error=$site hostname \
		>fatal-${site}.out 2>&1 &&
	id=$(sed -n "s/jobid: //p" fatal-${site}.out) &&
	test_must_fail_or_be_terminated flux job attach -XEl ${id} &&
	test_might_fail \
	    grep "flux-shell\[1\]: FATAL: log: log-fatal-error requested!" \
		fatal-${site}.out &&
	dump_job_output_eventlog $site | grep log-fatal-error

'
test_expect_success 'flux-shell: fatal error in task.exec works' '
	site=task.exec &&
	test_when_finished "test_debug \"dump_job_output_eventlog $site\"" &&
	test_must_fail_or_be_terminated \
		flux mini run -v -n2 -N2 \
			-o initrc=log.lua \
                	-o log-fatal-error=$site hostname \
		>fatal-${site}.out 2>&1 &&
	test_debug "cat fatal-${site}.out" &&
	id=$(sed -n "s/jobid: //p" fatal-${site}.out) &&
	test_must_fail_or_be_terminated flux job attach -XEl ${id} &&
	test_might_fail \
	    grep "flux-shell\[1\]: FATAL: log: log-fatal-error requested!" \
		fatal-${site}.out &&
	dump_job_output_eventlog $site | grep log-fatal-error
'
test_expect_success 'flux-shell: stdout/err from task.exec works' '
	cat <<-EOF >task.exec.print.lua &&
	plugin.register {
	  name = "stdout-test",
	    handlers = {
	    {
	      topic = "task.exec",
	      fn = function ()
	        io.stderr:write("this is stderr\n")
	        io.stdout:write("this is stdout\n")
	      end
	    }
	  }
	}
	EOF
	flux mini run -o initrc=task.exec.print.lua hostname \
	  >print.out 2>print.err &&
	grep "^this is stderr" print.err &&
	grep "^this is stdout" print.out
'
test_done
