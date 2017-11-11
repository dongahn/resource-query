#!/bin/sh

test_description='Test Various Satisfiability On Tiny Machine Configuration'

. $(dirname $0)/sharness.sh

cmp_dir="${SHARNESS_TEST_SRCDIR}/data/commands"
exp_dir="${SHARNESS_TEST_SRCDIR}/data/expected"
grugs="${SHARNESS_TEST_SRCDIR}/data/grugs/tiny.graphml"
query="${SHARNESS_TEST_SRCDIR}/../resource-query"

#
# Selection Policy -- High ID first (-P high)
#     The resource vertex with higher ID is preferred among its kind
#     (e.g., node0 is preferred over node1 if available)
#

cmds001="${cmp_dir}/test002.satisfiability.cmds"
test001_desc="36 core (satisfiable) cores and 37 cores (unsatisfiable) (pol=hi)"
test_expect_success "${test001_desc}" '
    ${query} -G ${grugs} -S CA -P high < ${cmds001} > 001.R.out &&
    test_cmp 001.R.out ${exp_dir}/tiny2/001.R.out
'

test_done

