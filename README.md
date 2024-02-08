<h1>Parallelizing bloom filter algorithm with OpenMP</h1>

The bloom filter algorithm is the technique I've selected to use in this project to achieve the string searching objective. The Bloom filter algorithm is a probabilistic data structure that saves space and may be used to determine if an element is a part of a set. However, because of its probabilistic nature, there is a tradeoff for its efficiency, and it is possible that it will yield a false positive result.False positives arise when it shows that a string may exist but does not really exist.

Variables to take note of:
\
M = initial size of the bit array
\
K = number of hash functions
\
N = number of strings being inserted

The bloom filter algorithm essentially consists of an empty bloom, which is a bit array of m bits with all bits set to 0.In addition, a fixed amount of K number of hash functions should be defined to calculate the hashes for a given string. With the provided hash values, we are able to flip the bit to 1 based on the index in the bloom filter. For example, if we were to hash the string “parallel” 3 times, and receive the value “1,3,7”, the bits for the indices will be flipped to 1. Now, if we were to hash the string and it returns the hash value of “1,3,7” as well. Because "parallel" has already set the bits as the indices, the bloom filter method will assert that it is present, resulting in a false positive. Through the technique of regulating false positive rate we may reduce the likelihood of a false positive by adjusting the size of the bloom filter.

<h2>Development of the Parallel String Mathcing algorithm</h2>

Pseudocode for the parallise portion
<p align="center">
<img width="390" alt="Screenshot 2023-09-26 at 01 24 54" src="https://github.com/jinyangjy/Bloom_filter_algorithm_with_OpenMP/assets/107976566/915a50c5-290c-490b-bf8a-bbd7dee9e25a">
</p>
The primary objective of parallelization is to concurrently process the numerous text files provided, consequently that it attains a reduced computing time. In this particular approach, each text file is denoted as an independent workload, with the objective being the allocation of these workloads among numerous threads. In this approach, data parallelism is accomplished by partitioning the task of processing several text files into separate threads, which can be executed concurrently to exploit parallelism.

In order to analyse the given directive, a flow chart will be employed as a visual representation to illustrate the procedure.

#pragma omp parallel for reduction (+:total_unique_words) reduction(+:total_optimization_time)

<p align="center">
<img width="185" alt="Screenshot 2023-09-26 at 01 28 07" src="https://github.com/jinyangjy/Bloom_filter_algorithm_with_OpenMP/assets/107976566/9370838e-8a8a-49ad-9fba-8f50d90a9a5b">
</p>
Throughout the parallel region, OpenMP is responsible for dynamically creating an array of threads. The total number of threads generated is contingent upon various factors, including the availability of CPU cores. Within each iteration of the loop, OpenMP utilises a process allocation mechanism to distribute the available threads for the purpose of processing the designated files. In this particular case, the attainment of data parallelism is accomplished through parallel processing of each individual text file. To illustrate an example, let us visualise a hypothetical situation where three files are given, and the OpenMP framework is set to generate a group of four threads. Each thread will be tasked with the responsibility of processing and executing separate computations on the specified file. The computational process involves multiple stages, such as the first step of reading a text file, determining the optimum size for a bit array based on the array of unique words, processing the data, and ultimately aggregating the end result. The reduction clauses utilised for the variables total_unique_words and total_optimization_time in OpenMP direct the aggregation of values over all created threads. Similarly, our methodology ensures an ongoing state of thread safety, hence ensuring precise computation of the accumulated outcomes while mitigating the possibility of a race condition.
