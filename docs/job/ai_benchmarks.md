# Benchmarks can be done by running the tests with 


Benchmarks have been performed on the following hardware. 

* Ryzen 5900
* Ryzen 5950X


To run benchmarks yourself. Just build the tests and run them. One can also set the 

```cmake 
JOB_TEST_BENCHMARKS=ON
```

They are on by default. 

**If you have 64MB of L3 cache. We suggest turning this on.**

```cmake
JOB_BLOCK_SIZE_256=ON
```


```text
-------------------------------------------------------------------------------
[00:13:45.688] [INFO ] XOR Solved at Gen 7 Score: 0.95317066
-------------------------------------------------------------------------------
Evolution: Throughput Benchmarks
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Evolution Step (Pop=4096, Tiny Net)            100             1    909.367 ms 
                                         8.4686 ms    8.35148 ms    8.64155 ms 
                                        721.207 us    542.217 us    998.638 us 
                                                                               
Evolution Step (Pop=512, Wide Net                                              
[256])                                         100             1    131.435 ms 
                                        1.41745 ms    1.39212 ms    1.47704 ms 
                                        187.933 us     100.06 us    380.858 us 
                                                                               

-------------------------------------------------------------------------------
SparseMoE: Throughput Benchmark (LLaMA-Small Scale)
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
MoE Forward (128x1024, 8 Experts,                                              
Top2)                                          100             1    314.768 ms 
                                        3.23846 ms    3.14652 ms    3.33291 ms 
                                        475.631 us    415.692 us    570.124 us 
                                                                               

-------------------------------------------------------------------------------
ESCoach: infra tax vs raw eval
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Raw parallel eval (Pop=1024, 1KB)              100             1     3.2156 ms 
                                        32.9132 us     31.951 us    33.8231 us 
                                         4.7594 us    4.00439 us    5.72125 us 
                                                                               

-------------------------------------------------------------------------------
ESCoach: High-Throughput Population Benchmark
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
ES Generation (Pop=1024, 1KB                                                   
Genome)                                        100             1    46.2502 ms 
                                        459.277 us     455.01 us    476.437 us 
                                        37.7799 us    7.81896 us      87.97 us 
                                                                               

-------------------------------------------------------------------------------
Barnes-Hut Adapter: Scaling Benchmark
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
BH N=1024                                      100             1    141.034 ms 
                                         1.4206 ms    1.40871 ms    1.43517 ms 
                                        66.7604 us    54.2307 us    95.7314 us 
                                                                               
BH N=4096                                      100             1     832.25 ms 
                                        6.84639 ms    6.79052 ms    6.90375 ms 
                                         288.03 us    245.117 us     353.46 us 
                                                                               
BH N=16384                                     100             1     7.03986 s 
                                        52.0229 ms     50.332 ms    53.6868 ms 
                                        8.59933 ms    7.52977 ms    9.98992 ms 
                                                                               

-------------------------------------------------------------------------------
Verlet Adapter: Throughput (N-Body O(N^2))
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Verlet Step (N=1024)                           100             1    188.237 ms 
                                        1.87563 ms    1.85774 ms    1.89372 ms 
                                        91.6625 us    84.7132 us    99.7455 us 
                                                                               

-------------------------------------------------------------------------------
FMM Adapter: Scaling Benchmark (O(N) Proof)
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
FMM N=1024                                     100             1    84.3746 ms 
                                        911.541 us    892.504 us    933.072 us 
                                        103.327 us     89.626 us    128.581 us 
                                                                               
FMM N=4096                                     100             1    313.375 ms 
                                        3.09963 ms    3.03533 ms    3.17352 ms 
                                        352.511 us    305.704 us     400.56 us 
                                                                               

-------------------------------------------------------------------------------
Attention: Scaling Benchmark (Seq=4096)
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
FMM Attention (O(N)) - Long Context            100             1    14.9247 ms 
                                        127.979 us    123.869 us    135.283 us 
                                        27.3147 us    18.6545 us    46.2315 us 
                                                                               
LowRank Attention (O(N)) - Long                                                
Context                                        100             1    11.1785 ms 
                                        116.697 us    112.125 us    133.986 us 
                                        40.0836 us    8.59577 us    92.6201 us 
                                                                               
Dense Attention - Long Context                 100             1    10.2559 ms 
                                        111.081 us    106.405 us    124.801 us 
                                        37.4612 us    15.6952 us    80.9947 us 
                                                                               

-------------------------------------------------------------------------------
MLP: Benchmark Standard (GPT-3 Medium Sized ReLU)
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
MLP Forward (ReLU)                             100             1    454.279 ms 
                                        4.52647 ms    4.44033 ms    4.62277 ms 
                                        465.688 us    394.953 us    566.209 us 
                                                                               

-------------------------------------------------------------------------------
MLP: Benchmark Gated (Swish)
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Gated MLP Forward (Swish)                      100             1     5.14306 s 
                                        53.6363 ms     53.066 ms    54.1441 ms 
                                        2.73343 ms    2.37217 ms     3.1833 ms 
                                                                               

-------------------------------------------------------------------------------
MLP: The Show down
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Naive                                          100             1    234.635 ms 
                                        2.33256 ms    2.32936 ms    2.34131 ms 
                                        24.8691 us    11.4922 us    52.4912 us 
                                                                               
ReLU (The Speed Demon)                         100             1    15.0708 ms 
                                        143.315 us    141.541 us    145.148 us 
                                        9.16135 us    8.07593 us    10.6273 us 
                                                                               
GELU (The Transformer Standard)                100             1    13.1651 ms 
                                        122.144 us    119.611 us    125.839 us 
                                        15.4297 us    11.5558 us    22.9439 us 
                                                                               
Swish (The LLaMA Style)                        100             1    12.5172 ms 
                                        123.054 us    121.402 us    125.329 us 
                                        9.78447 us    7.58433 us    13.0679 us 
                                                                               

-------------------------------------------------------------------------------
FlashAttention Benchmark
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
FlashAttention Forward (N=2048)                100             1    350.006 ms 
                                        3.53733 ms    3.51608 ms    3.58513 ms 
                                        152.922 us    80.8832 us     279.46 us 
                                                                               

-------------------------------------------------------------------------------
The Showdown FlashAttention Benchmark
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
FlashAttention Forward (N=512 dim=                                             
64)                                            100             1    29.4009 ms 
                                        278.389 us    265.275 us    300.394 us 
                                        84.4989 us    59.5646 us    149.273 us 
                                                                               
Naive Attention (N=512 dim=64)                 100             1    800.778 ms 
                                        7.99347 ms    7.99026 ms    7.99733 ms 
                                        17.8658 us    14.7672 us    22.9753 us 
                                                                            
-------------------------------------------------------------------------------
sgemm: Parallel Scaling (Single vs Multi-Thread)
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Serial AVX + Tiling SGEMM (m=512 K=                                            
1024 N=1024)                                   100             1     1.95924 s 
                                        19.7823 ms    19.7406 ms    19.8471 ms 
                                        261.403 us    188.369 us    397.901 us 
                                                                               
Parallel(8) + Tiling + AVX SGEMM                                               
(m=512 K=1024 N=1024)                          100             1    392.888 ms 
                                        3.31847 ms    3.23281 ms    3.47924 ms 
                                        580.572 us    355.025 us    903.117 us 
                                                                               

-------------------------------------------------------------------------------
SGEMM Showdown: Naive vs Optimized
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Naive Implementation (Scalar loops)            100             1     7.97374 s 
                                        79.2474 ms    79.1369 ms    79.4353 ms 
                                         717.82 us    487.359 us    1.19325 ms 
                                                                               
Optimized Implementation (AVX2 +                                               
Tiling)                                        100             1    59.4885 ms 
                                        594.413 us    593.008 us    595.979 us 
                                        7.52109 us    6.43432 us    9.64894 us 
                                                                               
-------------------------------------------------------------------------------
sgemm: Matrix Object vs Raw Overhead
-------------------------------------------------------------------------------

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Raw Pointer SGEMM (1024^3)                     100             1     3.95851 s 
                                        39.7727 ms     39.697 ms     39.869 ms 
                                        432.864 us    355.033 us    561.785 us 
                                                                               
Matrix Object SGEMM (1024^3)                   100             1     3.94029 s 
                                        39.5906 ms    39.5428 ms    39.6557 ms 
                                        282.418 us    219.408 us    380.043 us 
                                                                               
-------------------------------------------------------------------------------
Unix Socket: AI Genome Transfer
-------------------------------------------------------------------------------
benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Unix Socket Roundtrip (Kernel                                                  
Latency)                                       100             1     6.3857 ms 
                                        60.8287 us    59.4286 us    61.8381 us 
                                        5.94757 us    3.62628 us    9.74954 us 

-------------------------------------------------------------------------------
AI Genome IPC: Zero-Copy Serialization
-------------------------------------------------------------------------------

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
Genome Roundtrip (Serialize+                                                   
Transfer+Deserialize)                          100            74     1.8944 ms 
                                        251.311 ns    250.154 ns    254.138 ns 
                                        8.54616 ns    2.31529 ns    15.3059 ns 
                                                                               
-------------------------------------------------------------------------------
Activation performance: bulk ReLU
-------------------------------------------------------------------------------

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
ReLU loop (direct)                             100             1     5.6625 ms 
                                        57.7433 us    56.8401 us    58.8898 us 
                                        5.16175 us    4.30196 us    5.93951 us 
                                                                               

-------------------------------------------------------------------------------
GDN performance: small vs medium channels
-------------------------------------------------------------------------------

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
GDN C=16                                       100           351     1.8603 ms 
                                        52.7592 ns    52.5216 ns    53.5071 ns 
                                        1.95183 ns   0.640072 ns    4.29273 ns 
                                                                               

-------------------------------------------------------------------------------
Activation Benchmark: Scalar vs Vector
-------------------------------------------------------------------------------

benchmark name                       samples       iterations    est run time
                                     mean          low mean      high mean
                                     std dev       low std dev   high std dev
-------------------------------------------------------------------------------
ReLU (Scalar Loop)                             100             1     3.4579 ms 
                                        37.6283 us    36.5562 us    38.8573 us 
                                        5.87785 us    5.30112 us    6.25188 us 
                                                                               
ReLU (Vectorized Dispatch)                     100             1     3.1671 ms 
                                        39.2472 us    37.4317 us    41.5396 us 
                                        10.4144 us    8.57953 us    13.5171 us 
                                                                               
Swish (Scalar Loop)                            100             1     667.32 ms 
                                        6.67987 ms     6.6739 ms    6.68645 ms 
                                        32.0892 us    27.9022 us    37.7292 us 
                                                                               
Swish (Vectorized (By the painter))            100             1     4.4404 ms 
                                        41.7903 us    39.4427 us    46.0667 us 
                                        15.6273 us    10.3278 us    28.6193 us 

```
