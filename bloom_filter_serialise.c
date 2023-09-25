#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>


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

**strings_arr: double pointer representing an array of strings. Each element of strings_arr
is a pointer to a string.
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
int read_strings_from_file(const char *filename, char ***strings_arr, int *num_strings, int *total_strings, double *total_read_time) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("There's an error opening this text file");
        return 0;
    }
    *strings_arr = NULL;
    *num_strings = 0;
    char buffer[MAX_STRING_LEN];  // temporary buffer to read strings

    clock_t start_time, end_time;
    double process_time;

    start_time = clock();

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

    end_time = clock();
    process_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    *total_read_time += process_time; // Accumulate the read time
    printf("\nReading the file and counting the number of unique strings, Process time (seconds): %1f\n", process_time);

    return 1;
}

/*
Approach description: the main function is the entry point of the program, where all the
processing occurs. It is defined with an array of filenames for text files to be processed 
and dynamically determines the number of files without hardcoding it. It will then iterate
through each text files, while utilising the read_strings_from_file to read and count the 
unique words in each file. (Bloom filter starts) It will then calculate the optimal size of 
the bit array 'm' and the false positive rate for the bloom filter. It will then create the 
optimal sized bit array and insert the hash values of the unique strings using the 
division_method_hash function.It also includes a tester to check if a string exist within the
bloom filter. Lastly it frees up the memory allocated for the file's strings.
*/
int main() {
    const char *filenames[] = {"MOBY_DICK.txt", "LITTLE_WOMEN.txt", "SHAKESPEARE.txt"}; // Add more filenames as needed
    int num_files = sizeof(filenames) / sizeof(filenames[0]);

    char **strings_arr = NULL;
    int num_strings = 0;
    int total_strings = 0;
    int m = 0;
    double false_positive_rate;
    clock_t start_optimization_time, end_optimization_time, total_start_time, total_end_time;
    double optimization_process_time, total_process_time;
    double total_read_time = 0.0; // Track total time for reading and counting unique words
    double total_optimization_time = 0.0; // Track total time for optimization and insertion
    int total_unique_words = 0; // Track total unique words across all files

    total_start_time = clock();

    for (int i = 0; i < num_files; i++) {
        //passing address by reference from the read_strings_from_file function
        if (read_strings_from_file(filenames[i], &strings_arr, &num_strings, &total_strings, &total_read_time)) {
            total_unique_words += num_strings; // Accumulate unique words count

            printf("Initial bit array size based on the number of unique words in %s: %d\n", filenames[i], num_strings);
            int n = num_strings;

            start_optimization_time = clock();
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
                return 1;
            }

            for (int i = 0; i < num_strings; i++) {
                for (int j = 0; j < NUM_HASH_FUNCTIONS; j++) {
                    int hash = division_method_hash(strings_arr[i], m);
                    bit_array[hash] = 1;
                }
            }
            end_optimization_time = clock();
            optimization_process_time = ((double)(end_optimization_time - start_optimization_time)) / CLOCKS_PER_SEC;
            total_optimization_time += optimization_process_time; // Accumulate the optimization time
            printf("Total time for optimization and insertion (seconds): %lf\n", optimization_process_time);

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
                printf("The string '%s' is potentially in the bloom filter.\n", query);
            } 
            
            else {
                printf("The string '%s' does not exist in the bloom filter.\n", query);
            }

            // Free memory allocated for this file's strings
            // strings_arr[i] is used to access individual strings in the array of strings
            //dereferenced to get the pointer to the specific string to be freed.
            for (int i = 0; i < num_strings; i++) {
                free(strings_arr[i]);
            }
            free(strings_arr);
            free(bit_array);
        } 
        
        else {
            printf("Error reading strings from the text file %s\n", filenames[i]);
        }
    }

    total_end_time = clock();

    printf("\nOptimal bit array size based on calculations: %d\n", m);
    printf("Total unique strings from all files: %d\n", total_unique_words);
    printf("Total time for reading and counting unique words (seconds): %lf\n", total_read_time);
    printf("Total time for optimization and insertion (seconds): %lf\n", total_optimization_time);
    total_process_time = ((double)(total_end_time - total_start_time)) / CLOCKS_PER_SEC;
    printf("Total Process time (seconds): %lf\n\n", total_process_time);

    return 0;
}

