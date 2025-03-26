# Data structures

## Movement Paths

Path are represented as a contigious sequence of PathEntries (PE):

| PE 0  | PE 1  | PE 2  | PE 3  | PE 4  | ...  |
| ----- | ----- | ----- | ----- | ----- | ---- |

Where each PE is a bitset of 3 bits (rqp):

| bit 2 | bit 1 | bit 0 |
| ----- | ----- | ----- |
| r     | q     | p     |


PEs are then packed into sequence of bytes, with right-to-left bit order, with possibility of PEs traversing boundaries of single byte
(i.e. tigthly packed):

```
| byte 0                      | byte 1                        | byte 2
|  7  6  5    4  3  2    1  0 |  7    6  5  4    3  2  1    0 |  7  6   ...
| r0 q0 p0 : r1 q1 p1 : r2 q2 | p2 : r3 q3 p3 : r4 q4 p4 : r5 | q5 p5 : ...
```

TODO: REWRITE TABLES

There are 8 possible movements, excluding (0, 0):
`(-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 0), (0, 1), (1, -1), (1, 0), (1, 1)`
Each next movement cell is encoded using a 3x3 table:

```
|---------|---------|---------|
| r = .   | r = .   | r = .   |
| q = .   | q = .   | q = .   |
| p = .   | p = .   | p = .   |
|---------+---------+---------|
| r = .   | current | r = .   |
| q = .   | player  | q = .   |
| p = .   | cell    | p = .   |
|---------+---------+---------|
| r = .   | r = .   | r = .   |
| q = .   | q = .   | q = .   |
| p = .   | p = .   | p = .   |
|---------|---------|---------|
```

Which gives the following movements translation (coordinates start as usual, in top left corner):

```
|---------|---------|---------|
| PE = .  | PE = .  | PE = .  |
| dX = -1 | dX =  0 | dX =  1 |
| dY = -1 | dY = -1 | dY = -1 |
|---------+---------+---------|
| PE = .  | current | PE = .  |
| dX = -1 | player  | dX =  1 |
| dY =  0 | cell    | dY =  0 |
|---------+---------+---------|
| PE = .  | PE = .  | PE = .  |
| dX = -1 | dX =  0 | dX =  1 |
| dY =  1 | dY =  1 | dY =  1 |
|---------|---------|---------|
```



