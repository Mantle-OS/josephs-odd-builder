# JobYaml design into

## Why

```text
-------------------------------------------------------------------------------
BaseObject serialization benchmarks
-------------------------------------------------------------------------------
/home/jmills/git/opensource/josephs-odd-builder/tests/job_core/test_job_base_obj.cpp:1372
...............................................................................

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Binary Serialization (toBinary)                100            17      895.9 us 
                                        532.648 ns    524.712 ns    542.359 ns 
                                        44.4925 ns    38.5869 ns    54.8399 ns 
                                                                               
Binary Deserialization (fromBinary)            100            14        931 us 
                                        660.295 ns    651.354 ns    679.227 ns 
                                        63.4041 ns    37.8049 ns    119.473 ns 
                                                                               
JSON Serialization (toJson)                    100             2     1.2218 ms 
                                         6.2665 us    6.16326 us    6.52123 us 
                                        777.769 ns    412.069 ns    1.59608 us 
                                                                               
JSON Deserialization (fromJson)                100             4     1.0456 ms 
                                        2.60518 us    2.56865 us    2.70873 us 
                                        285.898 ns    120.487 ns    615.662 ns 
                                                                               
YAML Serialization (toYaml)                    100             1    24.9236 ms 
                                        246.507 us    245.714 us    248.832 us 
                                        6.35173 us    2.75842 us    13.8118 us 
                                                                               
YAML Deserialization (fromYaml)                100             1     6.4292 ms 
                                        64.8714 us    64.4342 us    66.1041 us 
                                        3.39735 us    1.45848 us    7.17524 us 
                                                                               

```

I ran so many tests and it is yamlcpp there is nothing I can do about that. 


The current `BaseObject` YAML implementation is backed by `yaml-cpp`.

Repeated profiling and benchmarking showed that the large gap between the binary/JSON paths and YAML is not coming from C++26 reflection, `BaseObject` member traversal, or JOB's type conversion layer. The dominant cost is inside the `yaml-cpp` representation and emission/parsing path itself.

At this point there is no useful optimization left to make in `job_core` that would materially change these numbers while continuing to use the same YAML backend.

This creates two choices:

1. Accept YAML as an unusually expensive persistence format inside JOB.
2. Replace the YAML backend.

`job_yaml` exists to test the second option.

The goal is not initially to reproduce `yaml-cpp` feature-for-feature. The first goal is to determine what YAML parsing and emission look like when designed around the facilities JOB already has today:

* C++26 reflection
* compile-time concepts
* annotations
* direct reflected object traversal
* non-owning `std::string_view` scanning
* small explicit parser state
* caller-owned fast paths
* JOB contracts for internal invariants
* direct lowering into final C++ members

The recent `JobIni` work demonstrated that a text format does not necessarily require an intermediate runtime object model between the source text and the destination C++ type.

Conceptually, the current YAML path resembles:

```text
C++ object
    ↓
reflection
    ↓
yaml-cpp node/object model
    ↓
yaml-cpp emitter
    ↓
YAML
```

and in the opposite direction:

```text
YAML
    ↓
yaml-cpp parser
    ↓
yaml-cpp node/object model
    ↓
JOB conversion
    ↓
reflected C++ object
```

`job_yaml` will investigate a different architecture:

```text
YAML
    ↓
JOB grammar / structural parser
    ↓
reflected member dispatch
    ↓
destination-specific conversion
    ↓
C++ object
```

and:

```text
C++ object
    ↓
reflection
    ↓
direct YAML emission
```

However, a dynamic node representation must not be required by the primary reflected serialization path.

The design question is therefore larger than simply writing a faster YAML parser:

> **How much YAML machinery is still necessary when the final C++ type already describes the structure we are trying to construct?**

If the resulting implementation is correct and materially faster, `job_yaml` will replace the YAML implementation used by `BaseObject`, allowing `job_core` to remove its `yaml-cpp` dependency.

Once the JOB YAML users have been migrated, the final goal is to remove `yaml-cpp` from the project entirely.

