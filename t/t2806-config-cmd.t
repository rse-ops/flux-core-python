#!/bin/sh

test_description='Test flux config get'

. $(dirname $0)/sharness.sh

mkdir config
cat <<EOF >config/config.toml
[foo]
a = 42
b = "meep"
c = "50s"
d = 3.14
e = false
f = [ "fubar", "barfu" ]
[foo.bar]
a = true
EOF

test_under_flux 1 minimal -o,--config-path=$(pwd)/config

runas_guest() {
        local userid=$(($(id -u)+1))
        FLUX_HANDLE_USERID=$userid FLUX_HANDLE_ROLEMASK=0x2 "$@"
}

test_expect_success 'flux-config with no args fails with message' '
	test_must_fail flux config 2>noargs.out &&
	grep "missing subcommand" noargs.out
'
test_expect_success 'flux-config get --help prints usage' '
	flux config get --help 2>get_help.out &&
	grep -i "query broker configuration values" get_help.out
'
test_expect_success HAVE_JQ 'flux-config dumps entire object' '
	echo 42 >foo.a.exp &&
	flux config get | jq -r .foo.a >foo.a.out &&
	test_cmp foo.a.exp foo.a.out
'
test_expect_success HAVE_JQ 'flux-config get dumps table' '
	flux config get foo | jq -r .a >foo.a2.out &&
	test_cmp foo.a.exp foo.a2.out
'
test_expect_success 'flux-config get dumps integer table member' '
	flux config get foo.a >foo.a3.out &&
	test_cmp foo.a.exp foo.a3.out
'
test_expect_success 'flux-config get --type=integer dumps int table member' '
	flux config get --type=integer foo.a >foo.a4.out &&
	test_cmp foo.a.exp foo.a4.out
'
test_expect_success 'flux-config get --type=string dumps string table member' '
	echo "meep" >foo.b.exp &&
	flux config get --type=string foo.b >foo.b.out &&
	test_cmp foo.b.exp foo.b.out
'
test_expect_success 'flux-config get --type=fsd dumps fsd table member' '
	echo "50s" >foo.c.exp &&
	flux config get --type=fsd foo.c >foo.c.out &&
	test_cmp foo.c.exp foo.c.out
'
test_expect_success 'flux-config get --type=fsd-integer works' '
	echo "50" >foo.c_fsd_int.exp &&
	flux config get --type=fsd-integer foo.c >foo.c_fsd_int.out &&
	test_cmp foo.c_fsd_int.exp foo.c_fsd_int.out
'
test_expect_success 'flux-config get --type=fsd-real works' '
	echo "50.0" >foo.c_fsd_real.exp &&
	flux config get --type=fsd-real foo.c | cut -c1-4 >foo.c_fsd_real.out &&
	test_cmp foo.c_fsd_real.exp foo.c_fsd_real.out
'
test_expect_success 'flux-config get --type=fsd fails on non-fsd string' '
	test_must_fail flux config get --type=fsd foo.b 2>badfsd.err &&
	grep "does not have the requested type" badfsd.err
'
test_expect_success 'flux-config get --type=real dumps real table member' '
	echo "3.14" >foo.d.exp &&
	flux config get --type=real foo.d | cut -c1-4 >foo.d.out &&
	test_cmp foo.d.exp foo.d.out
'
test_expect_success 'flux-config get --type=boolean dumps bool table member' '
	echo "false" >foo.e.exp &&
	flux config get --type=boolean foo.e >foo.e.out &&
	test_cmp foo.d.exp foo.d.out
'
test_expect_success HAVE_JQ 'flux-config get --type=array dumps array' '
	echo barfu >foo.f1.exp &&
	flux config get --type=array foo.f | jq -r -e ".[1]" >foo.f1.out &&
	test_cmp foo.f1.exp foo.f1.out
'
test_expect_success HAVE_JQ 'flux-config get --type=object dumps table' '
	echo true >foo.bar.a.exp &&
	flux config get --type=object foo.bar | jq -e ".a" >foo.bar.a.out &&
	test_cmp foo.bar.a.exp foo.bar.a.out
'
test_expect_success 'flux-config get dumps subtable value' '
	flux config get --type=boolean foo.bar.a >foo.bar.a2.out &&
	test_cmp foo.bar.a.exp foo.bar.a2.out
'
test_expect_success 'flux-config get wrong --type fails' '
	test_must_fail flux config get --type=string foo.a 2>foo.a.type.err &&
	grep "does not have the requested type" foo.a.type.err
'
test_expect_success 'flux-config get unknown --type fails' '
	test_must_fail flux config get --type=oops foo.a 2>unknown.err &&
	grep "Unknown type: oops" unknown.err
'
test_expect_success 'flux-config get --default dumps integer table member' '
	flux config get --default=2 foo.a >foo.a5.out &&
	test_cmp foo.a.exp foo.a5.out
'
test_expect_success 'flux-config get fails on non-existent key' '
	test_must_fail flux config get nokey 2>nokey.err &&
	grep "is not set" nokey.err
'
test_expect_success 'flux-config get --default works on non-existent key' '
	echo 42 >def.nokey.exp &&
	flux config get --default=42 nokey >def.nokey.out &&
	test_cmp def.nokey.exp def.nokey.out
'

test_expect_success 'flux-config reload works' '
	rm -f config/config.toml &&
	flux config reload
'
test_expect_success 'flux-config get returns empty object if no config' '
	echo "{}" >empty.exp &&
	flux config get >empty.out &&
	test_cmp empty.exp empty.out
'
test_expect_success 'flux-config load handles TOML input' '
	echo "test.y.z=42" >load.toml &&
	flux config load <load.toml &&
	flux config get >load.json
'
test_expect_success HAVE_JQ 'flux-config get returns loaded JSON' '
	jq -e ".test.y.z == 42" load.json
'
test_expect_success 'flux-config load handles empty input' '
	flux config load </dev/null &&
	flux config get >empty.json
'
test_expect_success HAVE_JQ 'now the config is empty' '
	jq -e ". == {}" empty.json
'
test_expect_success 'flux-config load handles JSON input' '
	flux config load <load.json &&
	flux config get >load2.json
'
test_expect_success HAVE_JQ 'flux-config get returns loaded JSON' '
	jq -e ".test.y.z == 42" load2.json
'
test_expect_success 'flux-config load handles TOML directory' '
	mkdir -p conf.d &&
	cat >conf.d/test.toml <<-EOT &&
	test.a.b.c = 999
	EOT
	flux config load conf.d &&
	flux config get >fromfiles.json
'
test_expect_success HAVE_JQ 'flux-config get returns loaded JSON' '
	jq -e ".test.a.b.c == 999" fromfiles.json
'
test_expect_success 'flux-config load PATH fails on invalid TOML' '
	cat >conf.d/test.toml <<-EOT &&
	this is not toml
	EOT
	test_must_fail flux config load conf.d
'
test_expect_success 'flux-config load fails on invalid TOML' '
	echo "[whoops" >bad.input &&
	test_must_fail flux config load <bad.input
'
test_expect_success 'flux-config builtin requires key name' '
	test_must_fail flux config builtin
'
test_expect_success 'flux-config builtin fails on unknown key' '
	test_must_fail flux config builtin unknown 2>bi_unknown.err &&
	grep "unknown is invalid" bi_unknown.err
'
test_expect_success 'flux-config builtin works on known key' '
	flux config builtin rc1_path
'
test_expect_success 'flux-config get works as guest' '
	runas_guest flux config get >obj
'
test_expect_success 'flux-config load fails as guest' '
	test_must_fail runas_guest flux config load <obj
'
test_expect_success 'flux-config reload fails as guest' '
	test_must_fail runas_guest flux config reload
'
test_expect_success 'flux-config builtin works as guest' '
	runas_guest flux config builtin rc1_path
'

test_done
