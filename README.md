# ext-folding
A circuit folding interface embedded in Berkeley's [ABC](https://github.com/berkeley-abc/abc) system.

## Overview
Circuit folding is a process of transforming a combinational circuit _C<sub>c</sub>_ into a sequential circuit _C<sub>s</sub>_, which after time-frame expansion, is functionally equivalent to the original combinational circuit  _C<sub>c</sub>_. It can be done via a functional BDD-based method or a structural AIG-based method. The proposed circuit folding techniques can be useful in testbench generation, sequential synthesis of bounded strategies, time multiplexing in FPGAs, and various applications in logic synthesis.

<img src='example/figures/tm.jpg' width="1000px"/>

If you would like to learn more about this research work, please refer to these [references](#refs).

## How to Compile
This repository contains an [extension module](https://github.com/berkeley-abc/ext-hello-abc) of [ABC](https://github.com/berkeley-abc/abc). Please follow the instructions below to compile[<sup>[1]</sup>](#fn1) this project.
```
git clone git@github.com:berkeley-abc/abc.git           # clone ABC
git clone git@github.com:NTU-ALComLab/ext-folding.git   # clone this repository
ln -s ext-folding/ abc/src/.                            # link this repository to abc/src/
cd abc/ && make                                         # build ABC with the extension module
```

Alternately, we also provide the `Dockerfile` to build the docker image capable of executing our codes.
```
git clone git@github.com:NTU-ALComLab/ext-folding.git   # clone this repository
docker build -t Folding ext-folding/                    # build the docker image
docker run -it Folding                                  # start a container
```

<a class="anchor" id="fn1">[1]</a>: Note that the codes in this project are written in C++11, you may need to set `CXXFLAGS := -std=c++11` in `abc/Makefile`.

## Command Usage
### Circuit Folding Commands

finite state machine (FSM) in [KISS2](https://link.springer.com/content/pdf/bbm%3A978-3-642-36166-1%2F1.pdf) format

[stamina](https://github.com/JackHack96/logic-synthesis)
[MeMin](https://github.com/andreas-abel/MeMin)



### Miscellaneous Commands
refer to the `example/`

## <a class="anchor" id="refs"></a>References
Please refer to the following papers if you are interested in our work. You may cite our papers with the provided BibTex entries.

* [ICCAD 2019 paper](https://po-chun-chien.github.io/publication/2019-11-timeFold):
  ```
  @inproceedings{Chien:ICCAD:2019,
      author      = {Po-Chun Chien and Jie-Hong Roland Jiang},
      title       = {Time-Frame Folding: Back to the Sequentiality},
      booktitle   = {Proceedings of the International Conference of Computer-Aided Design (ICCAD)},
      year        = {2019}
  }
  ```

* [DAC 2020 paper](https://po-chun-chien.github.io/publication/2020-07-timeMux):
  ```
  @inproceedings{Chien:DAC:2020,
      author      = {Po-Chun Chien and Jie-Hong Roland Jiang},
      title       = {Time Multiplexing via Circuit Folding},
      booktitle   = {Proceedings of the Design Automation Conference (DAC)},
      year        = {2020}
  }
  ```

* [Master's Thesis](https://po-chun-chien.github.io/publication/2020-06-thesis):
  ```
  @mastersthesis{Chien:Thesis:2020,
      author  = {Po-Chun Chien},
      title   = {Circuit Folding: From Combinational to Sequential Circuits},
      school  = {National Taiwan University},
      year    = {2020}
  }
  ```

## Contact
If you have any questions or suggestions, feel free to create an [issue](https://github.com/NTU-ALComLab/ext-folding/issues) or contact us through r07943091@ntu.edu.tw.