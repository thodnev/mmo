# MMO TODO

## Pathfinding

Higher-order tasks:
- [x] Implement pathfinding algo in raw form;
- [x] Refactor as a separate module, encapsulating and abstracting out the details;
- [x] Find optimal distance metric coefficients:
      - It should be coarse enough not to be overfit (otherwise, bizarre paths);
      - Still be computed efficiently. It's a hot piece of code;
- [ ] Cleanup code;
- [ ] Improve performance;
- [ ] Make able to return partial paths (if flag is passed);
- [ ] Implement map tiling (not that easy);

Concrete tasks:
- [ ] Hot inlinable raw map get by coordinates, to eliminate redundant checks;
- [ ] More optimal `heapq`, better suited for the task;
- [ ] Try applying squircle distance check, in addition to bbox boundaries.
      Time it.
- [ ] Optimize `visited` initialization.
      Try `std::fill`, `std::vector::assign`, loop, `std::memset` (*careful*).
      Find out which one is faster;
- [ ] Optimize `visited` reconstruction;
      Now it is slow AF compared to other lookup internals;
- [ ] Profile it. Find and eliminate bottlenecks;

- [ ] Refactor test/demo code
      - [ ] Wrap test code as a separate executable util, taking all input
            params as args, with results output to the stdout;
      - [ ] Refactor raw path visualization Python script to call the util,
            make public.
- [ ] OR (*better*) make a Python binding. Maybe ship as a separate package
      to benefit from open source collective participation;