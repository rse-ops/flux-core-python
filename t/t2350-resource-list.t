#!/bin/sh

test_description='flux-resource list tests'

# Append --logfile option if FLUX_TESTS_LOGFILE is set in environment:
test -n "$FLUX_TESTS_LOGFILE" && set -- "$@" --logfile
. $(dirname $0)/sharness.sh

# Use a static format to avoid breaking output if default flux-resource list
#  format ever changes
FORMAT="{state:>10} {properties:<10} {nnodes:>6} {ncores:>8} {ngpus:>8}"

for input in ${SHARNESS_TEST_SRCDIR}/flux-resource/list/*.json; do
    testname=$(basename ${input%%.json}) &&
    base=${input%%.json} &&
    name=$(basename $base) &&
    test_expect_success "flux-resource list input check: $testname" '
        flux resource list -o "$FORMAT" \
            --from-stdin < $input > $name.output 2>&1 &&
        test_debug "cat $name.output" &&
        test_cmp ${base}.expected $name.output
    '
    test_expect_success "flux-resource info input check: $testname" '
        flux resource info --from-stdin < $input > ${name}-info.output 2>&1 &&
        test_debug "cat ${name}-info.output" &&
        test_cmp ${base}-info.expected ${name}-info.output
    '
done

test_expect_success 'create test input with properties' '
	cat <<-EOF >properties-test.in
	{
	  "all": {
	    "version": 1,
	    "execution": {
	      "R_lite": [
	        {
	          "rank": "0-3",
	          "children": {
	            "core": "0-3"
	          }
	        }
	      ],
	      "starttime": 0,
	      "expiration": 0,
	      "nodelist": [
	        "asp,asp,asp,asp"
	      ],
	      "properties": {
	        "foo": "2-3",
	        "xx": "1-2"
	      }
	    }
	  },
	  "allocated": {
	    "version": 1,
	    "execution": {
	      "R_lite": [
	        {
	          "rank": "1",
	          "children": {
	            "core": "0"
	          }
	        }
	      ],
	      "starttime": 0,
	      "expiration": 0,
	      "nodelist": [
	        "asp"
	      ],
	      "properties": {
	        "xx": "1"
	      }
	    }
	  },
	  "down": {
	    "version": 1,
	    "execution": {
	      "R_lite": [
	        {
	          "rank": "1",
	          "children": {
	            "core": "0-3"
	          }
	        }
	      ],
	      "starttime": 0,
	      "expiration": 0,
	      "nodelist": [
	        "asp"
	      ],
	      "properties": {
	        "xx": "1"
	      }
	    }
	  }
	}
	EOF
'

test_expect_success 'flux resource list -no {properties} works' '
	flux resource list -no {properties} \
		--from-stdin <properties-test.in >properties.out &&
	test $(cat properties.out) = "xx,foo"
'
test_expect_success 'flux resource list -no {properties:>4.4+} works' '
	flux resource list -no "{properties:>5.5+}" \
		--from-stdin <properties-test.in >properties-trunc.out &&
	test_debug "cat properties-trunc.out" &&
	test $(cat properties-trunc.out) = "xx,f+" &&
	flux resource list -no "{properties:>5.5h+}" \
		--from-stdin <properties-test.in >properties-trunc+h.out &&
	test_debug "cat properties-trunc+h.out" &&
	test $(cat properties-trunc.out) = "xx,f+"
'
test_expect_success 'flux resource list -o rlist works' '
	flux resource list -o rlist \
		--from-stdin <properties-test.in >rlist.out &&
	test_debug "cat rlist.out" &&
	grep -i list rlist.out
'
test_done
