# DroidPerf

DroidPerf is a lightweight, object-centric memory profiler for ART, which associates memory
inefficiencies with objects used by Android apps. With such object-level information, DroidPerf is able to guide locality optimization on memory layouts, access patterns, and allocation patterns.


## Contents

-   [Installation](#installation)
-   [Usage](#usage)
-   [Client tools](#client-tools)
-   [Supported Platforms](#support-platforms)
-   [License](#license)

## Installation



### Build

In order to build you'll need the following packages:

- gcc (>= 4.8)
- [cmake](https://cmake.org/download/) (>= 3.4)
- Android SDK (>= 33)
- Android NDK (>= 21)


Follow these steps to get source code and build DroidPerf:

1. Clone this repository to your local machine.
2. Run the shell script file *cross.sh*.

## Usage

### Linux

To run DroidPerf, one needs to use the following command:

#### 1 Set the global environment variable

```console
$ export drrun=/path/to/DroidPerf/build/bin64/drrun
```

#### 2 Run client tool

-   **x86_64**

```console
$ $drrun -t <client tool> -- <application> [apllication args]
```

e.g. The command below will start the client **drcctlib_instr_statistics_clean_call** to analyze **echo _"Hello World!"_**.

```console
$ $drrun -t drcctlib_instr_statistics_clean_call -- echo "Hello World!"
```

-   **aarch64**

```console
$ $drrun -unsafe_build_ldstex -t <client tool> -- <application> [apllication args]
```

## Client tools

### Internal client tools list

| Name                                 | Features                                                                    | Status  |
| ------------------------------------ | --------------------------------------------------------------------------- | ------- |
| drcctlib_cct_only_clean_call         | A tool that collects call path on each instruction.                         | release |
| drcctlib_instr_statistics_clean_call | A instruction counting tool that counts each instruction.                   | release |
| drcctlib_reuse_distance_client_cache | A reuse distance measurement tool.                                          | release |
| drcctlib_cct_only                    | (Code cache mode)A tool that collects call path on each instruction.        | beta    |
| drcctlib_instr_statistics            | (Code cache mode) A instruction counting tool that counts each instruction. | beta    |
| drcctlib_reuse_distance              | (Code cache mode) A reuse distance measurement tool.                        | beta    |


## Support Platforms

The following platforms pass our tests.

### Linux

| CPU                               | Systems       | Architecture |
| --------------------------------- | ------------- | ------------ |
| Intel(R) Xeon(R) CPU E5-2699 v3   | Ubuntu 18.04  | x86_64       |
| Intel(R) Xeon(R) CPU E5-2650 v4   | Ubuntu 14.04  | x86_64       |
| Intel(R) Xeon(R) CPU E7-4830 v4   | Red Hat 4.8.3 | x86_64       |
| Arm Cortex A53(Raspberry pi 3 b+) | Ubuntu 18.04  | aarch64      |
| Arm Cortex-A57(Jetson Nano)       | Ubuntu 18.04  | aarch64      |
| ThunderX2 99xx                    | Ubuntu 20.04  | aarch64      |
| AWS Graviton1                     | Ubuntu 18.04  | aarch64      |
| AWS Graviton2                     | Ubuntu 18.04  | aarch64      |
| Fujitsu A64FX                     | CentOS 8.1    | aarch64      |


## License

DroidPerf is released under the [MIT License](http://www.opensource.org/licenses/MIT).

## Related Publication
1. Milind Chabbi, Xu Liu, and John Mellor-Crummey. 2014. Call Paths for Pin Tools. In Proceedings of Annual IEEE/ACM International Symposium on Code Generation and Optimization (CGO '14). ACM, New York, NY, USA, , Pages 76 , 11 pages. DOI=http://dx.doi.org/10.1145/2544137.2544164
2. Milind Chabbi, Wim Lavrijsen, Wibe de Jong, Koushik Sen, John Mellor-Crummey, and Costin Iancu. 2015. Barrier elision for production parallel programs. In Proceedings of the 20th ACM SIGPLAN Symposium on Principles and Practice of Parallel Programming (PPoPP 2015). ACM, New York, NY, USA, 109-119. DOI=http://dx.doi.org/10.1145/2688500.2688502
3. Milind Chabbi and John Mellor-Crummey. 2012. DeadSpy: a tool to pinpoint program inefficiencies. In Proceedings of the Tenth International Symposium on Code Generation and Optimization (CGO '12). ACM, New York, NY, USA, 124-134. DOI=http://dx.doi.org/10.1145/2259016.2259033
4. Shasha Wen, Xu Liu, Milind Chabbi, "Runtime Value Numbering: A Profiling Technique to Pinpoint Redundant Computations"  The 24th International Conference on Parallel Architectures and Compilation Techniques (PACT'15), Oct 18-21, 2015, San Francisco, California, USA
5. Shasha Wen, Milind Chabbi, and Xu Liu. 2017. REDSPY: Exploring Value Locality in Software. In Proceedings of the Twenty-Second International Conference on Architectural Support for Programming Languages and Operating Systems (ASPLOS '17). ACM, New York, NY, USA, 47-61. DOI: https://doi.org/10.1145/3037697.3037729
6. Pengfei Su, Shasha Wen, Hailong Yang, Milind Chabbi, and Xu Liu. 2019. Redundant Loads: A Software Inefficiency Indicator. In Proceedings of the 41st International Conference on Software Engineering (ICSE '19). IEEE Press, Piscataway, NJ, USA, 982-993. DOI: https://doi.org/10.1109/ICSE.2019.00103
7. Jialiang Tan, Shuyin Jiao, Milind Chabbi, Xu Liu. What Every Scientific Programmer Should Know About Compiler Optimizations? The 34th ACM International Conference on Supercomputing (ICS'20), Jun 29 - Jul 2, 2020, Barcelona, Spain.
8. Xin You, Hailong Yang, Zhongzhi Luan, Depei Qian, Xu Liu. Zerospy: Exploring the Software Inefficiencies with Redundant Zeros, The International Conference for High Performance Computing, Networking, Storage and Analysis (SC'20), Nov 15-20, 2020, Atlanta, GA, USA.
9. Qidong Zhao, Xu Liu, Milind Chabbi. DroidPerf: A Fine-grained Call Path Profiler for ARM-based Clusters, The International Conference for High Performance Computing, Networking, Storage and Analysis (SC'20), Nov 15-20, 2020, Atlanta, GA, USA. 