# A Simple Guide to Circuit Folding

## Time-frame Folding
```
read $PATH_FOLD/example/sample_circuits/s27.blif    # read s27 benchmark circuit
frames -i -F 3                                      # time-frame unfolding (expansion)
time_fold                                           # time-frame folding
memin                                               # FSM minimization
read_kiss                                           # encode FSM into logic circuit
print_stats                                         # print info.
```
<img src="figures/s27_fsm.png" width="450px"/> <img src="figures/s27_fsm.m.png" width="350px"/>

## Functional Circuit Folding
```
read $PATH_FOLD/example/sample_circuits/03-adder.blif
```
<img src="figures/adder_fsm.png" width="500px"/> <img src="figures/adder_fsm.m.png" width="200px"/>
## Structural Circuit Folding
```
```