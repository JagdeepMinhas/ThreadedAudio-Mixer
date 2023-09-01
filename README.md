# ThreadedAudio-Mixer
Great! It sounds like you've completed your project. Here's a suggestion for a name and a `README.md` file to help you get started:

## Project Name: "ThreadedAudio-Mixer"

### Description

"ThreadedAudio-Mixer" is a multi-threaded program designed to process 16-bit integer input from multiple channels, apply optional low-pass and amplification filters, and formulate a final mixed output. The program reads input data from various files, processes them concurrently using multiple threads, and generates a combined output according to the specified filter and amplification parameters.

### Features

- **Multi-Channel Processing:** Process data from multiple channels simultaneously.
- **Dynamic Thread Allocation:** Create and manage threads dynamically based on the number of input channels.
- **Low-Pass Filter:** Apply a low-pass filter to smooth sample values (adjustable alpha parameter).
- **Amplification:** Amplify sample values by a specified factor (adjustable beta parameter).
- **Output Handling:** Save the processed data to an output file, rounding values to the nearest integer.
- **Global and Local Checkpointing:** Choose between global and local checkpointing strategies for synchronized thread execution.
- **Robust Input Handling:** Robustly handle input data, including buffer size management and format validation.

### Usage

#### Building the Program

Compile the program using your preferred C/C++ compiler:

```bash
gcc -o myChannels myChannels.c -lpthread
```

#### Running the Program

To execute the program, use the following command-line arguments:

```bash
./myChannels <buffer_size> <num_threads> <metadata_file_path> <lock_config> <global_checkpointing> <output_file_path>
```

- `<buffer_size>`: Size of the buffer in bytes when reading a file.
- `<num_threads>`: Number of threads to use for processing channels.
- `<metadata_file_path>`: Absolute path of the metadata file containing channel information.
- `<lock_config>`: Lock configuration (1 for global lock, 2 for granular locks, 3 for compare_and_swap).
- `<global_checkpointing>`: Flag (0 or 1) to enable global checkpointing.
- `<output_file_path>`: Path to the output file for storing processed data.

### Example Usage

```bash
./myChannels 1024 4 metadata.txt 2 0 output.txt
```

### Input Data Format

#### Metadata File

The metadata file should contain channel information in the following format:

```
<number_of_input_files>
<channel_file1_path>
<channel_file1_low_pass_filter_value>
<channel_file1_amplification>
<channel_file2_path>
<channel_file2_low_pass_filter_value>
<channel_file2_amplification>
...
```

#### Channel Files

Each channel file should contain a list of samples, with one sample per line. Each sample should be a 16-bit integer greater than 0.

### Output

The program processes the input channels, applies filters and amplification, and stores the mixed output in the specified output file.

### Error Handling

The program includes error handling for various scenarios, such as invalid input data, buffer overflows, and file read errors.

### Authors

Jagdeep Sngh

