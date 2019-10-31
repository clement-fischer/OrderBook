# Central Limit Order Book

## Design Considerations

The order book is essentially implemented using two maps, with price as keys and sets of limit orders as values. Limit orders implement `operator<` so that items in the set are sorted by lowest timestamps first, then by lowest order ID in case of equality. Additionally, limit orders are also stored in a map, with order ID as keys, for fast order lookup.

As a central limit order book would typically be serving multiple requests at the same time, the structures are protected by shared mutexes, which allow concurrent read access, but exclusive write access.

For memory allocations, no `new` statement is used, objects stored on the heap are all in the structures previously defined.

## Setup

From root folder:

```Shell
$ cd build
$ cmake ..
$ make
$ # Optional. Run the unit tests.
$ ctest --verbose
```

Developed using the g++ compiler, version 7.4.0.

## Run

After completing the setup steps, from root folder:

```Shell
$ cd bin
$ ./order_book
```

## Test

To run the unit tests without `ctest`, after completing the setup steps and from root folder:

```Shell
$ cd bin
$ ./order_book_test
```

## Possible improvements

Synchronization between the two structures, with different mutexes, led to a more complicated implementation that initially intended. A simpler synchronization, with one mutex, might be an improvement to reduce code complexity at the cost of performance. If instead higher performance is targetted, more work should be done on concurrency-safety, adding recursive locks in some functions could help, at the cost of a higher implementation complexity.

Dynamic allocations could also help improving on performance, at the cost of paying extra attention for deallocation. This would also enable to remove copies of limit orders between the two structures, but a comparison class would need to be implemented to sort the pointers not by their value but instead by the value with they point to, adding a bit of complexity again.

Finally although there are a few comments in the difficult parts, more documentation in the code would be a clear improvement.
