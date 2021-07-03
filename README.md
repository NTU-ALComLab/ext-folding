# ext-folding
A circuit folding interface embedded in Berkeley's [ABC](https://github.com/berkeley-abc/abc) system.

## Overview
Circuit folding is a technique

The proposed circuit folding techniques can be useful in testbench generation, sequential synthesis of bounded strategies, and various applications in logic synthesis.

add image

## How to Compile
```
git clone git@github.com:berkeley-abc/abc.git
git clone link-to-this-project
ln -s ext-folding/ abc/src/.
```

```
cd abc/
make
cd ../
```
## Command Usage
### Circuit Folding Commands
### Miscellaneous Commands
refer to the `example/`

## Citation
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
If you have any questions or suggestions, feel free to [create an issue](TODO) or contact us through r07943091@ntu.edu.tw.