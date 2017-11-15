#!/bin/sh

test_description='Test reservations of jobs of varying geometries and durations'

. $(dirname $0)/sharness.sh

cmd_dir="${SHARNESS_TEST_SRCDIR}/data/commands/reservation"
exp_dir="${SHARNESS_TEST_SRCDIR}/data/expected/reservation"
grugs="${SHARNESS_TEST_SRCDIR}/data/grugs/resv_test.graphml"
query="${SHARNESS_TEST_SRCDIR}/../resource-query"

#
# Selection Policy -- High ID first (-P high)
#     The resource vertex with higher ID is preferred among its kind
#     (e.g., node1 is preferred over node0 if available)
#

cmds001="${cmd_dir}/test005.reservation.cmds"
test001_desc="allocate or reserve 17 jobs (pol=hi)"
test_expect_success "${test001_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds001} > 001.R.out &&
    test_cmp 001.R.out ${exp_dir}/001.R.out
'

#
# Selection Policy -- High ID first (-P low)
#     The resource vertex with higher ID is preferred among its kind
#     (e.g., node0 is preferred over node1 if available)
#

cmds002="${cmd_dir}/test005.reservation.cmds"
test002_desc="allocate or reserve 17 jobs (pol=low)"
test_expect_success "${test002_desc}" '
    ${query} -G ${grugs} -S CA -P low < ${cmds002} > 002.R.out &&
    test_cmp 002.R.out ${exp_dir}/002.R.out
'

test_done
