#!/bin/sh

test_description='Test kvs eventlog get|append'

. `dirname $0`/kvs/kvs-helper.sh

. `dirname $0`/sharness.sh

test_under_flux 4 kvs

waitfile=${SHARNESS_TEST_SRCDIR}/scripts/waitfile.lua

test_expect_success 'flux kvs eventlog append --timestamp works' '
	flux kvs eventlog append --timestamp=1 test.a name {\"data\":\"foo\"} &&
	flux kvs eventlog get --unformatted test.a >get_a.out &&
        echo "{\"timestamp\":1.0,\"name\":\"name\",\"context\":{\"data\":\"foo\"}}" \
             >get_a.exp &&
        flux kvs get test.a &&
        test_cmp get_a.exp get_a.out
'

test_expect_success 'flux kvs eventlog append works w/o context' '
	flux kvs eventlog append test.b foo &&
	flux kvs eventlog get test.b >get_b.out &&
	grep foo get_b.out
'

test_expect_success 'flux kvs eventlog append works w/ context' '
	flux kvs eventlog append test.c foo {\"data\":\"bar\"} &&
	flux kvs eventlog get test.c >get_c.out &&
	grep -q foo get_c.out &&
        grep -q "\"data\":\"bar\"" get_c.out
'

test_expect_success 'flux kvs eventlog get --watch --count=N works' '
	flux kvs eventlog append --timestamp=42 test.d foo &&
	flux kvs eventlog append --timestamp=43 test.d bar &&
	run_timeout 2 flux kvs eventlog \
		get --watch --unformatted --count=2 test.d >get_d.out &&
	echo "{\"timestamp\":42.0,\"name\":\"foo\"}" >get_d.exp &&
	echo "{\"timestamp\":43.0,\"name\":\"bar\"}" >>get_d.exp &&
        test_cmp get_d.exp get_d.out
'

test_expect_success NO_CHAIN_LINT 'flux kvs eventlog get --watch returns append order' '
	flux kvs eventlog append --timestamp=1 test.e foo "{\"data\":\"bar\"}" &&
	echo "{\"timestamp\":1.0,\"name\":\"foo\",\"context\":{\"data\":\"bar\"}}" >get_e.exp
	flux kvs eventlog get --unformatted --watch --count=20 test.e >get_e.out &
	pid=$! &&
	for i in $(seq 2 20); do \
		flux kvs eventlog append --timestamp=$i test.e foo "{\"data\":\"bar\"}"; \
    	        echo "{\"timestamp\":$i.0,\"name\":\"foo\",\"context\":{\"data\":\"bar\"}}" >>get_e.exp
	done &&
	wait $pid &&
	test_cmp get_e.exp get_e.out
'

test_expect_success NO_CHAIN_LINT 'flux kvs eventlog get --waitcreate works' '
        test_must_fail flux kvs eventlog get --unformatted test.f &&
	flux kvs eventlog get --unformatted --waitcreate test.f >get_f.out &
	pid=$! &&
        wait_watcherscount_nonzero primary &&
	flux kvs eventlog append --timestamp=1 test.f foo "{\"data\":\"bar\"}" &&
	echo "{\"timestamp\":1.0,\"name\":\"foo\",\"context\":{\"data\":\"bar\"}}" >get_f.exp
	wait $pid &&
	test_cmp get_f.exp get_f.out
'

test_expect_success 'flux kvs eventlog append and work on alternate namespaces' '
        flux kvs namespace create EVENTLOGTESTNS &&
        flux kvs eventlog append test.ns main &&
        flux kvs eventlog append --namespace=EVENTLOGTESTNS test.ns guest &&
        flux kvs eventlog get test.ns > get_f1.out &&
        grep main get_f1.out &&
        flux kvs eventlog get --namespace=EVENTLOGTESTNS test.ns > get_f2.out &&
        grep guest get_f2.out
'

test_expect_success 'flux kvs eventlog wait-event detects eventlog with event' '
	flux kvs eventlog append --timestamp=42 test.wait_event.a foo &&
	flux kvs eventlog append --timestamp=43 test.wait_event.a bar &&
        flux kvs eventlog wait-event --unformatted test.wait_event.a foo > wait_event_a1.out &&
        grep foo wait_event_a1.out &&
        test_must_fail grep bar wait_event_a1.out &&
        flux kvs eventlog wait-event --unformatted test.wait_event.a bar > wait_event_a2.out &&
        test_must_fail grep foo wait_event_a2.out &&
        grep bar wait_event_a2.out
'

test_expect_success 'flux kvs eventlog wait-event outputs more events with -v' '
	flux kvs eventlog append --timestamp=42 test.wait_event.b foo &&
	flux kvs eventlog append --timestamp=43 test.wait_event.b bar &&
        flux kvs eventlog wait-event --unformatted -v test.wait_event.b foo > wait_event_b1.out &&
        grep foo wait_event_b1.out &&
        test_must_fail grep bar wait_event_b1.out &&
        flux kvs eventlog wait-event --unformatted -v test.wait_event.b bar > wait_event_b2.out &&
        grep foo wait_event_b2.out &&
        grep bar wait_event_b2.out
'

test_expect_success 'flux kvs eventlog wait-event doesnt output events with -q' '
	flux kvs eventlog append --timestamp=42 test.wait_event.c foo &&
	flux kvs eventlog append --timestamp=43 test.wait_event.c bar &&
        flux kvs eventlog wait-event --unformatted -q test.wait_event.c foo > wait_event_c1.out &&
        test_must_fail grep foo wait_event_c1.out &&
        test_must_fail grep bar wait_event_c1.out &&
        flux kvs eventlog wait-event --unformatted -q test.wait_event.c bar > wait_event_c2.out &&
        test_must_fail grep foo wait_event_c2.out &&
        test_must_fail grep bar wait_event_c2.out
'

test_expect_success 'flux kvs eventlog wait-event fails on eventlog without event' '
	flux kvs eventlog append --timestamp=42 test.wait_event.d foo &&
	flux kvs eventlog append --timestamp=43 test.wait_event.d bar &&
        test_expect_code 137 run_timeout 0.1 flux kvs eventlog wait-event test.wait_event.d foobar
'

test_expect_success 'flux kvs eventlog wait-event fails on non-existent eventlog' '
        test_must_fail flux kvs eventlog wait-event test.wait_event.e foo
'

test_expect_success NO_CHAIN_LINT 'flux kvs eventlog wait-event --waitcreate works' '
        test_must_fail flux kvs eventlog get --unformatted test.wait_event.f &&
	flux kvs eventlog wait-event --waitcreate --unformatted test.wait_event.f foo >wait_event_f.out &
	pid=$! &&
        wait_watcherscount_nonzero primary &&
	flux kvs eventlog append --timestamp=1 test.wait_event.f foo "{\"data\":\"bar\"}" &&
	echo "{\"timestamp\":1.0,\"name\":\"foo\",\"context\":{\"data\":\"bar\"}}" >wait_event_f.exp
	wait $pid &&
	test_cmp wait_event_f.exp wait_event_f.out
'

test_expect_success NO_CHAIN_LINT 'flux kvs eventlog wait-event --timeout works' '
	flux kvs eventlog append --timestamp=42 test.wait_event.g foo &&
	flux kvs eventlog append --timestamp=43 test.wait_event.g bar &&
	test_must_fail flux kvs eventlog wait-event --timeout=0.1 test.wait_event.g baz
'

#
# corner case tests
#

test_expect_success 'flux kvs eventlog get fails on bad input' '
	test_must_fail flux kvs eventlog get
'

test_expect_success 'flux kvs eventlog append fails on bad input' '
	test_must_fail flux kvs eventlog append
'

test_expect_success 'flux kvs eventlog append fails with invalid context' '
	test_must_fail flux kvs eventlog append test.bad.context foo not_a_object
'

test_expect_success 'flux kvs eventlog wait-event fails on bad input' '
	test_must_fail flux kvs eventlog wait-event
'

test_done
