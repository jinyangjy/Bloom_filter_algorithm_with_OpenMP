#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <omp.h>

#define MAX_STRING_LEN 100
#define MAX_FP_RATE 0.05
#define NUM_HASH_FUNCTIONS 4

/*
Function Description: calculates the optimum bit array 'm' based on the number
of unique words 'n' and the desired false positive rate 'max_fp_rate'. It will
iteratively calculate the false positive rate unit it falls below the desired
maximum fp '0.05'.
*/
int calc_optimum_bitArraySize(int n, double max_fp_rate) {
    int k = NUM_HASH_FUNCTIONS;
    double m = 1.0;
    double current_false_positive_rate = 1.0;

    while (current_false_positive_rate > max_fp_rate) {
        m++;
        current_false_positive_rate = pow(1 - pow(1 - (1.0 / m), k * n), k);
    }
    return (int)(-n * log(max_fp_rate) / ((log(2) * log(2))));
}

/*
Function Description: The division method hash is used to hash each  unique word 
before inserting the value into the bloom filter. It will initialize a 'hash'
variable to zero and iterates through the chracacters in the word. Through this
process, each ASCII Values of the characters in the word are summed up, then it
will me modulo-ed to fit the result within the range of the bit array size.
*/
int division_method_hash(const char *word, int m) {
    unsigned long hash = 0;
    size_t len = strlen(word);
    for (int i = 0; i < len; i++) {
        hash += (unsigned long)word[i];
    }
    return (int)(hash % m);
}

/*
Function Description: checks if a given 'string' exists within an array of strings
'strings_arr' up to a specific number of strings 'num_strings' that needs to be 
checked. It comparses the strings each string to check for potential duplications.
If its a duplicate: 1, if not 0.
*/
int duplicated_strings(const char *string, char **strings_arr, int num_strings) {
    for (int i = 0; i < num_strings; i++) {
        if (strcmp(strings_arr[i], string) == 0) {
            return 1;
        }
    }
    return 0;
}

/*
Function Description: Reads strings from text file specified by a 'filename' and stores 
unique words in a dynamically allocated array. It also keeps track of the number of strings
read 'num_strings', the total number of strings read across the file 'total_strings', and 
the total time taken for reading_and counting the unique words 'total_read_time'. While 
reading from the file using fscanf, it checks if the read strings is duplicated using the
duplicated_strings function. If the string's nont a duplicate, it dynamically allocates
memory for the new string and adds it to the strings_arr. 

***strings_arr: creates an array of strings, where each string is a pointer to an array of
characters. Used to pass address of the array of strings between functions so that the array
can be dynamically allocated and modified.
*/
int read_strings_from_file(const char *filename, char ***strings_arr, int *num_strings, int *total_strings, double *local_read_time) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("There's an error opening this text file");
        return 0;
    }
    *strings_arr = NULL;
    *num_strings = 0;
    char buffer[MAX_STRING_LEN];  // temporary buffer to read strings

    double start_time, end_time;

    start_time = omp_get_wtime();

    while (fscanf(file, "%s", buffer) != EOF) {
        (*total_strings)++;
        if (!duplicated_strings(buffer, *strings_arr, *num_strings)) {
            // if the string is not a duplicated string, add it to the array of strings.
            (*num_strings)++;
            /*
            strings_arr: dereferenced to access the pointer to the array strings.
            realloc dynamically resizes the array of strings. It returns a pointer to the
            newly allocated memory block, and *strings_arr is updated to point to this
            new memory block.
            */
            *strings_arr = (char **)realloc(*strings_arr, (*num_strings) * sizeof(char *));
            if (*strings_arr == NULL) {
                perror("Memory allocation has failed");
                fclose(file);
                return 0;
            }
            /*
            *strings_arr is used to access the array of strings, whereas (*num_strings) - 1
            is used to index the last element in the array of strings. 'strdup' is used to 
            duplicate the contents of buffer and allocate memory for the new string. The 
            pointer to this newly allocated memory is stored in the array of strings.
            */
            (*strings_arr)[(*num_strings) - 1] = strdup(buffer);
            if ((*strings_arr)[(*num_strings) - 1] == NULL) {
                perror("Memory allocation has failed");
                fclose(file);
                return 0;
            }
        }
    }
    fclose(file);

    end_time = omp_get_wtime();
    *local_read_time = end_time - start_time; // Store the local read time

    return 1;
}

/*
Approach Description: The main function initially is executed by a single thread. Whereas inside
the main function, where the it loops over the num_files, the OpenMP parallelization kicks off. 
OpenMP automatically distributes the iterations of the loop among multiple threads, which can 
execute the loop concurrently. Each thread will be processing a different file independently. 
Threads execute the file reading and count, the bloom filter constructing (bit array optimizatio,
and the insertion) with the query checks concurrently for their respective files. After the iterations
are completed, the reduction clauses ensure that each thread's contribution to the 'total_unique_words'
and the 'total_optimization_time are correctly combined into their global values. This parallization
approach allowed me to process the multiple text files concurrenty. 

*/
int main() {
    const char *filenames[] = {"MOBY_DICK.txt", "LITTLE_WOMEN.txt", "SHAKESPEARE.txt"};
    int num_files = sizeof(filenames) / sizeof(filenames[0]);
    
    double total_optimization_time = 0.0;
    int total_unique_words = 0;
    int m = 0; // Added variable declaration for m

    double total_start_time, total_end_time; // Added double variables
    
    total_optimization_time = 0.0;
    total_unique_words = 0;

    total_start_time = omp_get_wtime(); // Measure the start time before entering the loop

    // Parallelize the file reading and processing loop
    /*
    total_unique_words and total_optimization_time should be treated as private within each thread
    and the combined (reduced) into their global values after the loop is done. This allows threads
    to update these variables independently without causing race conditions.
    */
    #pragma omp parallel for reduction(+:total_unique_words) reduction(+:total_optimization_time)
    for (int i = 0; i < num_files; i++) {
        char **strings_arr = NULL;
        int num_strings = 0;
        int total_strings = 0;
        double false_positive_rate = 0.0;

        // Flag to indicate if an error occurred in this file
        int error = 0;

        double local_read_time = 0.0;
        double local_optimization_time = 0.0;
        //passing address by reference from the read_strings_from_file function
        if (read_strings_from_file(filenames[i], &strings_arr, &num_strings, &total_strings, &local_read_time)) {
            total_unique_words += num_strings;

            printf("Initial bit array size based on the number of unique words in %s: %d\n", filenames[i], num_strings);
            int n = num_strings;

            double start_optimization_time = omp_get_wtime(); // Start measuring optimization time
            m = calc_optimum_bitArraySize(n, MAX_FP_RATE);
            false_positive_rate = pow(1 - pow(1 - (1.0 / m), NUM_HASH_FUNCTIONS * n), NUM_HASH_FUNCTIONS);
            printf("False Positive Rate: %f\n", false_positive_rate);
	    
	    	/*
	    	*bit_array is used to create a dynamically allocated array of integers.
	    	'calloc' will then allocate memory for an array of 'm' integers, and *bit_array
	    	points to the start of this allocated memory block.
	    	*/
            int *bit_array = (int *)calloc(m, sizeof(int));
            if (bit_array == NULL) {
                perror("Memory allocation has failed");
                error = 1; // Set error flag
            }

            if (!error) {
                for (int i = 0; i < num_strings; i++) {
                    for (int j = 0; j < NUM_HASH_FUNCTIONS; j++) {
                        int hash = division_method_hash(strings_arr[i], m);
                        bit_array[hash] = 1;
                    }
                }
                const char *query = "geohash";
                int is_present = 1;
                for (int j = 0; j < NUM_HASH_FUNCTIONS; j++) {
                    int hash = division_method_hash(query, m);
                    if (bit_array[hash] == 0) {
                        is_present = 0;
                        break;
                    }
                }
                if (is_present) {
                    printf("The string '%s' is potentially in the bloom filter.\n\n", query);
                } else {
                    printf("The string '%s' does not exist in the bloom filter.\n\n", query);
                }
            }
            double end_optimization_time = omp_get_wtime(); // End measuring optimization time

            // Free memory allocated for this file's strings
            // strings_arr[i] is used to access individual strings in the array of strings
            //dereferenced to get the pointer to the specific string to be freed.
            for (int i = 0; i < num_strings; i++) {
                free(strings_arr[i]);
            }
            free(strings_arr);
            free(bit_array);

            // Accumulate local optimization time
            local_optimization_time = end_optimization_time - start_optimization_time;
        } 
        
        else {
            printf("Error reading strings from the text file %s\n", filenames[i]);
            error = 1; // Set error flag
        }

        // Accumulate local read time
        /*
        local_optimization_time is performed atomic-cally without race conditions by multiple
        threads. Each thread calculates its local optimization time, and this directive 
        ensures that the values are safely accumulated into the total_optimization_time.  
        */
        #pragma omp atomic
        total_optimization_time += local_optimization_time;
    }
    
    total_end_time = omp_get_wtime(); // Measure the end time after the loop completes

    // Calculate and print total process time
    double total_process_time = total_end_time - total_start_time; 
    double total_read_time = total_process_time - total_optimization_time;
    printf("Optimal bit array size based on calculations: %d\n", m);
    printf("Total unique strings from all files: %d\n", total_unique_words);
    printf("Total time for reading and counting unique words (seconds): %lf\n", total_read_time);
    printf("Total time for optimization and insertion (seconds): %lf\n", total_optimization_time);
    printf("Total Process time (seconds): %lf\n\n", total_process_time);

    return 0;
}
