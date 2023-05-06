###############################################################
# Copyright 2019 Lawrence Livermore National Security, LLC
# (c.f. AUTHORS, NOTICE.LLNS, COPYING)
#
# This file is part of the Flux resource manager framework.
# For details, see https://github.com/flux-framework.
#
# SPDX-License-Identifier: LGPL-3.0
###############################################################

# Usage: flux python submit-wait.py njobs
#
# Submit njobs jobs in a loop, then job.wait() njobs times
#

import flux
from flux import job
from flux.job import JobspecV1
import sys

if len(sys.argv) != 2:
    njobs = 10
else:
    njobs = int(sys.argv[1])

# Open connection to broker
h = flux.Flux()

# Submit njobs test jobs (half will fail)
jobspec = JobspecV1.from_command(["/bin/true"])
jobspec_fail = JobspecV1.from_command(["/bin/false"])
for i in range(njobs):
    if i < njobs / 2:
        jobid = job.submit(h, jobspec, waitable=True)
        print("submit: {} /bin/true".format(jobid))
    else:
        jobid = job.submit(h, jobspec_fail, waitable=True)
        print("submit: {} /bin/false".format(jobid))


# Wait for njobs jobs
for i in range(njobs):
    result = job.wait(h)
    if result.success:
        print("wait: {} Success".format(result.jobid))
    else:
        print("wait: {} Error: {}".format(result.jobid, result.errstr))

# vim: tabstop=4 shiftwidth=4 expandtab
