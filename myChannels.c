/**
 * Author- Jagdeep Singh/ Gurkirat Singh
 * Date - July 13, 2023
 * Program- This program reads input from different files and writes them to ouput with number of threds using various synchronisation and  locking configutrations based on the user input
 * */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdatomic.h>

// Struct for  each thread
typedef struct
{
    int total_num_threads;
    int thread_indexex;
    int buffer_size;
    double *alpha;
    double *beta;
    FILE **files;      // array of multiple input files
    int numberOfFiles; // number of files
    double *results;   // dynamic array to store results
} ThreadParams;

// Global variables
int thread_index = 0;
int global_checkpointing = 0;
double *arr;
pthread_mutex_t lock;
pthread_mutex_t *granular_locks;
int temp_index = 0;
FILE *output_File;
int locks_config = 0;
int *array_for_threads;
int *fininshed_thread_counter;
atomic_int cas_lock = 0;

// function to write to ouyput file based on the lock configuration
void write_to_output(double number, int tid)
{
    int roundResult = (int)ceil(number);
    if (locks_config == 1)
    {
        pthread_mutex_lock(&lock);
        fprintf(output_File, "%d\n", roundResult);
        pthread_mutex_unlock(&lock);
    }
    else if (locks_config == 2)
    {
        pthread_mutex_lock(&granular_locks[tid]);
        fprintf(output_File, "%d\n", roundResult);
        pthread_mutex_unlock(&granular_locks[tid]);
    }
    else if (locks_config == 3)
    {

        int expected = 0;
        while (!atomic_compare_exchange_weak(&cas_lock, &expected, 1))
        {
            expected = 0;
            // Spin or yield
        }

        fprintf(output_File, "%d\n", roundResult);

        atomic_store(&cas_lock, 0);
    }
}
// This function manipulate the read data based on various lock config and global synchronisation 
void *threadFunction(void *arg)
{
    ThreadParams *params = (ThreadParams *)arg;

    pthread_t thread_id = pthread_self();
    printf("Thread ID: %lu\n", thread_id);

    // Initialize first_byte and end_flags arrays
    int *first_byte = malloc(params->numberOfFiles * sizeof(int));
    int *end_flags = malloc(params->numberOfFiles * sizeof(int));
    for (int i = 0; i < params->numberOfFiles; i++)
    {
        first_byte[i] = 1;
        end_flags[i] = 0;
    }

    char **buffers = malloc(params->numberOfFiles * sizeof(char *) * 50);
    double *sample_values = malloc(params->numberOfFiles * sizeof(double) * 50);
    double *new_sample_values = malloc(params->numberOfFiles * sizeof(double) * 50);
    double *previous_sample_values = malloc(params->numberOfFiles * sizeof(double) * 50);
    double *temp_values = malloc(params->numberOfFiles * sizeof(double) * 50);

    for (int i = 0; i < params->numberOfFiles; i++)
    {
        buffers[i] = malloc((params->buffer_size + 100) * sizeof(char));
    }

    int counter = -1;

    // Main loop to read buffers from files
    while (1)
    {

        if (global_checkpointing == 1)
        {

            while (params->thread_indexex != thread_index)
                ;
        }

        // Check if all files have reached the end
        int all_files_ended = 1;
        for (int i = 0; i < params->numberOfFiles; i++)
        {

            if (!end_flags[i])
            {
                all_files_ended = 0;
                break;
            }
        }

        if (all_files_ended)
        {
            fininshed_thread_counter[params->thread_indexex] = 1;

            int all_files_finished = 0;

            for (int k = 0; k < params->total_num_threads; k++)
            {
                if (fininshed_thread_counter[k] == 0)
                {
                    break;
                }
                if (k == params->total_num_threads - 1)
                {
                    all_files_finished = 1;
                }
            }

            if (all_files_finished == 1)
            {

                break;
            }

            int i = params->thread_indexex;
            if (i == params->total_num_threads - 1)
            {
                i = 0;
            }
            else
            {
                i++;
            }
            for (int j = i; j < params->total_num_threads; j++)
            {
                if (fininshed_thread_counter[j] != 1)
                {
                    thread_index = j;
                    break;
                }
                if (j == params->total_num_threads - 1)
                {
                    j = 0;
                }
            }
            break;
        }

        counter = counter + 1;
        // Loop through each file and read one buffer
        for (int i = 0; i < params->numberOfFiles; i++)
        {

            if (end_flags[i])
            {
                continue; // Skip if the file has reached the end
            }

            FILE *input_file = params->files[i];
            char *buffer = buffers[i];
            int count = 0;
            int ch;
            while ((ch = fgetc(input_file)) != EOF)
            {
                if (count)
                    if (ch == '\r')
                    {
                        continue;
                    }

                if (isprint(ch) || ch == '\n')
                {
                    buffer[count++] = ch;

                    if (count == params->buffer_size)
                    {
                        buffer[count] = '\0';

                        // Process the buffer
                        char temp[256]; // Adjust the size as per your requirements
                        int tempIndex = 0;

                        for (int i = 0; buffer[i] != '\0'; i++)
                        {
                            if (buffer[i] == '\n')
                            {
                                continue;
                            }

                            temp[tempIndex] = buffer[i];
                            tempIndex++;
                        }

                        temp[tempIndex] = '\0'; // Null-terminate the temporary string

                        sample_values[i] = strtod(temp, NULL); //

                        if (first_byte[i])
                        {
                            new_sample_values[i] = sample_values[i];
                            first_byte[i] = 0;
                            previous_sample_values[i] = new_sample_values[i];
                        }
                        else
                        {
                            new_sample_values[i] = params->alpha[i] * sample_values[i] + (1 - params->alpha[i]) * previous_sample_values[i];
                            previous_sample_values[i] = new_sample_values[i];
                        }
                        temp_values[i] = new_sample_values[i] * params->beta[i];

                        printf("New sample value: %.3lf\n", temp_values[i]);

                        // Add temp_values to results array
                        params->results[counter] += temp_values[i];

                        arr[temp_index] += temp_values[i];

                        count = 0;
                        for (int i = 0; i < params->buffer_size; i++)
                        {
                            buffer[i] = '\n';
                        }
                        break; // Exit the temp_indexer while loop to read from the next file
                    }
                }
            }

            if (ch == EOF)
            {
                if (count == 0)
                {
                    printf("end of file.....\n");
                    // Set end_flags[i] to true if the file has reached the end
                    end_flags[i] = 1;
                    // Close the file
                    fclose(input_file);
                }
                else
                {
                    printf("end of file.....\n");
                    buffer[count] = '\0';

                    // Process the buffer
                    char temp[256]; // Adjust the size as per your requirements
                    int tempIndex = 0;

                    for (int i = 0; buffer[i] != '\0'; i++)
                    {
                        if (buffer[i] == '\n')
                        {
                            continue;
                        }

                        temp[tempIndex] = buffer[i];
                        tempIndex++;
                    }

                    temp[tempIndex] = '\0'; // Null-terminate the temporary string

                    sample_values[i] = strtod(temp, NULL); //

                    if (first_byte[i])
                    {
                        new_sample_values[i] = sample_values[i];
                        first_byte[i] = 0;
                    }
                    else
                    {
                        new_sample_values[i] = params->alpha[i] * sample_values[i] + (1 - params->alpha[i]) * previous_sample_values[i];
                    }
                    temp_values[i] = new_sample_values[i] * params->beta[i];

                    printf("New sample value: %.3lf\n", temp_values[i]);

                    // Add temp_values to results array
                    params->results[counter] += temp_values[i];

                    arr[temp_index] += temp_values[i];

                    count = 0;

                    // Set end_flags[i] to true if the file has reached the end
                    end_flags[i] = 1;

                    // Close the file
                    fclose(input_file);
                }
            }
            for (int i = 0; i < params->numberOfFiles; i++)
            {

                if (!end_flags[i])
                {
                    all_files_ended = 0;
                    break;
                }
            }

            if (global_checkpointing == 1)
            {
                int i = params->thread_indexex;
                if (i == params->total_num_threads - 1)
                {
                    i = 0;
                    printf("Result %d: %.3lf\n", temp_index, arr[temp_index]);
                    write_to_output(arr[temp_index], params->thread_indexex);

                    arr[temp_index] = 0;

                    temp_index = temp_index + 1;
                }
                else
                {
                    i++;
                }
                for (int j = i; j < params->total_num_threads; j++)
                {
                    if (fininshed_thread_counter[j] != 1)
                    {
                        thread_index = j;
                        break;
                    }
                    if (j == params->total_num_threads - 1)
                    {
                        j = 0;
                        printf("Result %d: %.3lf\n", temp_index, arr[temp_index]);
                        write_to_output(arr[temp_index], params->thread_indexex);

                        arr[temp_index] = 0;
                        temp_index = temp_index + 1;
                    }
                    if ((params->thread_indexex) == j)
                    {
                        break;
                    }
                }
            }
        }
    }

    // Print the results array
    if (global_checkpointing == 0)
        for (int i = 0; i <= counter; i++)
        {
            printf("Result %d: %.3lf\n", i, params->results[i]);
            write_to_output(params->results[i], params->thread_indexex);
        }

    // Cleanup allocated memory
    free(first_byte);
    free(end_flags);
    for (int i = 0; i < params->numberOfFiles; i++)
    {
        free(buffers[i]);
    }
    free(buffers);
    free(sample_values);
    free(previous_sample_values);
    free(new_sample_values);
    free(temp_values);
    array_for_threads[params->thread_indexex] = 1;
    pthread_exit(NULL);
}
//Struct for program params
typedef struct
{
    int buffer_size;
    int num_threads;
    char *metadata_file_path;
    int lock_config;
    int global_check;
    char *output_file_path;
} ProgramParams;

//Struct for input file
typedef struct
{
    char *input_file_path;
    double alpha;
    double beta;
} InputFile;

// Main function for the function to rad from file and allocate memory
int main(int argc, char *argv[])
{

    if (argc < 7)
    {
        printf("Insufficient number of arguments.\n");
        printf("Usage: ./myChannels <buffer_size> <num_threads> <metadata_file_path> <lock_config> <global_checkpointingbal_checkpointing> <output_file_path>\n");
        return 1;
    }

    ProgramParams params;

    params.buffer_size = atoi(argv[1]);
    params.num_threads = atoi(argv[2]);
    params.metadata_file_path = argv[3];
    params.lock_config = atoi(argv[4]);
    params.global_check = atoi(argv[5]);
    params.output_file_path = argv[6];

    if (params.lock_config != 1 && params.lock_config != 2 && params.lock_config != 3)
    {
        printf("Invalid value for lock_config. Expected values: 1, 2, or 3.\n");
        return 1;
    }

    if (params.global_check != 0 && params.global_check != 1)
    {
        printf("Invalid value for global_checkpointing. Expected values: 0 or 1.\n");
        return 1;
    }

    locks_config = params.lock_config;
    global_checkpointing = params.global_check;

    // open the output file
    output_File = fopen(argv[6], "w");
    if (output_File == NULL)
    {
        printf("Failed to open the output file.\n");
        return 1;
    }

    // open the metadata file
    FILE *metadata_file = fopen(params.metadata_file_path, "r");
    if (metadata_file == NULL)
    {
        fclose(output_File);
        printf("Failed to open metadata file.\n");
        return 1;
    }

    fininshed_thread_counter = malloc(sizeof(int) * params.num_threads);
    for (int i = 0; i < params.num_threads; i++)
    {
        fininshed_thread_counter[i] = 0;
    }

    // locks 
    pthread_mutex_init(&lock, NULL);
    granular_locks = malloc(sizeof(pthread_mutex_t) * params.num_threads);
    for (int t = 0; t < params.num_threads; t++)
    {
        pthread_mutex_init(&granular_locks[t], NULL);
    }

    array_for_threads = malloc(sizeof(int) * params.num_threads); // thsi is global thread checker
    for (int t = 0; t < params.num_threads; t++)
    {
        array_for_threads[t] = 0;
    }

    int num_input_files;
    fscanf(metadata_file, "%d", &num_input_files);

    InputFile *input_files = malloc(num_input_files * sizeof(InputFile));
    if (input_files == NULL)
    {
        printf("Failed to allocate memory for input files.\n");
        free(input_files);
        free(granular_locks);
        // Free fininshed_thread_counter
        free(fininshed_thread_counter);
        pthread_mutex_destroy(&lock);
        for (int t = 0; t < params.num_threads; t++)
        {
            pthread_mutex_destroy(&granular_locks[t]);
        }
        // Free array_for_threads
        free(array_for_threads);
        fclose(metadata_file);
        fclose(output_File);
        return 1;
    }

    for (int i = 0; i < num_input_files; i++)
    {
        char *input_file_path = NULL;
        double alpha = 1.0;
        double beta = 1.0;

        fscanf(metadata_file, "%ms %lf %lf", &input_file_path, &alpha, &beta);

        input_files[i].input_file_path = input_file_path;
        input_files[i].alpha = alpha;
        input_files[i].beta = beta;
    }
    fclose(metadata_file);

    if (num_input_files % params.num_threads != 0)
    {
        printf("Number of input files must be a multiple of the number of threads.\n");
        free(input_files);
        return 1;
    }

    int files_per_thread = num_input_files / params.num_threads;
    pthread_t *threads = malloc(params.num_threads * sizeof(pthread_t));
    ThreadParams *thread_params = malloc(params.num_threads * sizeof(ThreadParams));

    arr = malloc(sizeof(double) * 100);
    for (int p = 0; p < 100; p++)
    {
        arr[p] = 0;
    }

    int file_thread_indexex = 0;
    // assigning threads to assign files 
    for (int i = 0; i < params.num_threads; i++)
    {
        thread_params[i].buffer_size = params.buffer_size;
        thread_params[i].alpha = malloc(files_per_thread * sizeof(double));
        thread_params[i].beta = malloc(files_per_thread * sizeof(double));
        thread_params[i].numberOfFiles = files_per_thread;
        thread_params[i].results = malloc(200 * sizeof(double));
        thread_params[i].files = malloc(files_per_thread * sizeof(FILE *));
        thread_params[i].thread_indexex = i;
        thread_params[i].total_num_threads = params.num_threads;

        for (int j = 0; j < files_per_thread; j++)
        {
            thread_params[i].results[j] = 0;
        }

        if (thread_params[i].results == NULL || thread_params[i].files == NULL)
        {
            printf("Failed to allocate memory for thread parameters.\n");
            // Handle error and cleanup
            return (1);
        }

        for (int j = 0; j < files_per_thread; j++)
        {
            FILE *input_file = fopen(input_files[file_thread_indexex].input_file_path, "r");
            if (input_file == NULL)
            {
                printf("Failed to open input file: %s\n", input_files[file_thread_indexex].input_file_path);
                // Handle error and cleanup
                return (1);
            }
            thread_params[i].files[j] = input_file;

            thread_params[i].alpha[j] = input_files[file_thread_indexex].alpha;
            thread_params[i].beta[j] = input_files[file_thread_indexex].beta;

            file_thread_indexex++;
        }

        if (pthread_create(&threads[i], NULL, threadFunction, &thread_params[i]) != 0)
        {
            printf("Failed to create thread %d.\n", i);
            // Handle error and cleanup
            return (1);
        }
    }

    for (int i = 0; i < params.num_threads; i++)
    {
        pthread_join(threads[i], NULL);
    }
    

    // Collect the results from all threads
    int total_results = num_input_files;
    double *all_results = malloc(total_results * sizeof(double));
    if (all_results == NULL)
    {
        printf("Failed to allocate memory for all results.\n");
        // Handle error and cleanup
        return (1);
    }


    pthread_mutex_destroy(&lock);
    for (int t = 0; t < params.num_threads; t++)
    {
        pthread_mutex_destroy(&granular_locks[t]);
    }

    // Free thread_params, thread_params[i].alpha, thread_params[i].beta, thread_params[i].results, and thread_params[i].files for each thread
    for (int i = 0; i < params.num_threads; i++)
    {
        free(thread_params[i].alpha);
        free(thread_params[i].beta);
        free(thread_params[i].results);
        free(thread_params[i].files);
    }
    // Free input_files and each input file path
    for (int i = 0; i < num_input_files; i++)
    {
        free(input_files[i].input_file_path);
    }

    free(granular_locks);
    fclose(output_File);
    free(threads);
    free(thread_params);
    free(all_results);
    free(input_files);

    // Free fininshed_thread_counter
    free(fininshed_thread_counter);

    // Free array_for_threads
    free(array_for_threads);

    // Free arr
    free(arr);
    return 0;
}