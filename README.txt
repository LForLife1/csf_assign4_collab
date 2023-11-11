CONTRIBUTIONS

Liam did majority of the coding, while Trevor did majority of
testing/reporting. Both contributed to all parts in some way,
however.

REPORT

The test results are below the explanation.

In the results, we found that as the threshold decreased, so did
the time it takes for the process to fully execute. This is because
by decreasing the threshold, it increased the amount of forks in the
code before resorting to sequential sort. For example, the first
test with a threshold of 2097152 means that the data of size 16M
would be run only among 8 processes as 16M divided by 2097152 is 8.
Following this logic, each subsequent test doubles the number of
parallel processes accounting for the drastic reduction in time.

This effect, however, stops after around 64 processes. This is
because the computer only has so many cpu cores to utilize. After
using them all up, adding more instances cannot utilize any more
processing power.


**********************
**** TEST RESULTS ****
**********************

test 16M 2097152
    real    0m0.387s
    user    0m0.372s
    sys     0m0.013s

test 16M 1048576
    real    0m0.239s
    user    0m0.402s
    sys     0m0.026s

test 16M 524288
    real    0m0.184s
    user    0m0.446s
    sys     0m0.052s

test 16M 262144
    real    0m0.157s
    user    0m0.524s
    sys     0m0.045s

test 16M 131072
    real    0m0.151s
    user    0m0.540s
    sys     0m0.067s

test 16M 65536
    real    0m0.154s
    user    0m0.538s
    sys     0m0.100s

test 16M 32768
    real    0m0.174s
    user    0m0.598s
    sys     0m0.101s

test 16M 16384
    real    0m0.169s
    user    0m0.577s
    sys     0m0.169s