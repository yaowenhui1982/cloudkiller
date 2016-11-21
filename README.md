# cloudkiller
the critical issues in cloud environment；

## Thread hang
We always use multiple threads programs in cloud environment. You may want all the threads behave as the same when there is some failoure about other soft or hard ware. Recently, we have encounter some of the threads in on process hang there but other threads running normally. The reason is that disk hang occured when the cold code fragment is not loaded into the memory, so the hot path run normally.
To handle this isuue, please read source code in thread/hang/;
