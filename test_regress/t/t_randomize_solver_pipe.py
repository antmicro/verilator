#!/usr/bin/env python3
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of either the GNU Lesser General Public License Version 3
# or the Perl Artistic License Version 2.0.
# SPDX-FileCopyrightText: 2026 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import os

import vltest_bootstrap

test.scenarios('vlt')
test.top_filename = "t/t_randomize_solver_fault.v"

if not test.have_solver:
    test.skip("No constraint solver installed")

test.compile()

# Every scenario acts on the first reply of its kind, so the counts do not
# depend on how many replies a particular solver sends per randomize() call.
# once=True acts one time in the whole run; the restarted solver then serves
# every later call. once=False acts again in every solver, so the runtime gives
# up and disables randomization.
runs = [
    ('die_at', 10, 3),  # solver exits with a model reply pending
    ('die_status_at', 4, 1),  # solver exits with a soft constraint status pending
    ('die_status_at', 7, 2),  # solver exits between the status and the model read
    ('mute_at', 3, 2),  # solver stays running but stops answering
    ('garbage_at', 2, 1),  # solver answers, but not with an S-expression
    # These land on a reply the UniGen2 cell enumerator reads
    ('unknown_var', 5, 2),  # reply names a variable that was not asked for
    ('pair_paren_all', 10, 3),  # a (name value) pair lost its opening paren
    ('bad_value_all', 10, 3),  # a value with no base
    ('extra_pair_all', 14, 3),  # one pair more than was asked for
]

for mode, once, npass in runs:
    tag = mode + ('_once' if once else '_always')
    logfile = test.obj_dir + '/sim_' + tag + '.log'
    latch = test.obj_dir + '/' + tag + '.latch'
    if os.path.exists(latch):
        os.unlink(latch)
    test.execute(logfile=logfile,
                 run_env='VERILATOR_SOLVER="' + test.t_dir + '/randomize_solver_tamper.py" ' +
                 'TAMPER=' + mode + ' TAMPER_AT=1 ' +
                 ('TAMPER_ONCE="' + latch + '" ' if once else ''))
    test.file_grep(logfile, r'NPASS=(\d+)', npass)
    test.file_grep(logfile, r'Solver died or replied unreadably')

test.file_grep(test.obj_dir + '/sim_garbage_at_always.log', r'Solver failed repeatedly')

# A solver that never starts is not restarted, so it is never reported as dead
logfile = test.obj_dir + '/sim_nosolver.log'
test.execute(logfile=logfile, run_env='VERILATOR_SOLVER=someimaginarysolver ')
test.file_grep(logfile, r'NPASS=(\d+)', 0)
test.file_grep(logfile, r'Unable to communicate with SAT solver')
test.file_grep_not(logfile, r'Solver died')

# One rejected command must not count against the solver: the reply was
# complete, so the session stays and later calls keep working
logfile = test.obj_dir + '/sim_err_once.log'
latch = test.obj_dir + '/err_once.latch'
if os.path.exists(latch):
    os.unlink(latch)
test.execute(logfile=logfile,
             run_env='VERILATOR_SOLVER="' + test.t_dir + '/randomize_solver_tamper.py" ' +
             'TAMPER=err_once TAMPER_AT=2 TAMPER_ONCE="' + latch + '" ')
test.file_grep(logfile, r'NPASS=(\d+)', 11)
test.file_grep_not(logfile, r'Solver failed repeatedly')
test.file_grep_not(logfile, r'Solver died')

test.passes()
