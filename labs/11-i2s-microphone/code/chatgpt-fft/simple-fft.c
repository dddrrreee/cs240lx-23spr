// gcc -O2 -Wall simple-fft.c -o simple-fft -lm
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define SAMPLE_RATE 32768
#define DURATION .5 
#define ERROR 10
#define AMPLITUDE 0.9
#define NOISE_AMPLITUDE 0.1

// Function to perform the FFT
void fft(int N, double* x, double* y)
{
    // Base case: N = 1
    if (N == 1)
        return;

    double even_x[N / 2];
    double even_y[N / 2];
    double odd_x[N / 2];
    double odd_y[N / 2];

    // Split the samples into even and odd parts
    for (int i = 0; i < N / 2; i++)
    {
        even_x[i] = x[2 * i];
        even_y[i] = y[2 * i];
        odd_x[i] = x[2 * i + 1];
        odd_y[i] = y[2 * i + 1];
    }

    // Recursive calls
    fft(N / 2, even_x, even_y);
    fft(N / 2, odd_x, odd_y);

    // Combine the results
    for (int k = 0; k < N / 2; k++)
    {
        double angle = -2.0 * M_PI * k / N;
        double w_real = cos(angle);
        double w_imag = sin(angle);

        double t_real = w_real * odd_x[k] - w_imag * odd_y[k];
        double t_imag = w_real * odd_y[k] + w_imag * odd_x[k];

        x[k] = even_x[k] + t_real;
        y[k] = even_y[k] + t_imag;
        x[k + N / 2] = even_x[k] - t_real;
        y[k + N / 2] = even_y[k] - t_imag;
    }
}

// Function to generate a synthetic signal
void generateSignal(int N, double sample_rate, double frequency, double* signal)
{
    double time_step = 1.0 / sample_rate;

    for (int i = 0; i < N; i++)
    {
        double time = i * time_step;
        signal[i] = sin(2.0 * M_PI * frequency * time);
    }
}

typedef struct {
    double frequency;
    double magnitude;
} FrequencyMagnitude;

int compare(const void* a, const void* b) {
    FrequencyMagnitude* fa = (FrequencyMagnitude*)a;
    FrequencyMagnitude* fb = (FrequencyMagnitude*)b;
    if (fa->magnitude < fb->magnitude)
        return 1;
    if (fa->magnitude > fb->magnitude)
        return -1;
    return 0;
}

// returns the top
double print_top_10_results(int N, double* real, double* imag, double sample_rate, double test_frequency, int verbose_p) {
    FrequencyMagnitude results[N/2];
    printf("--------------------------------------------------------\n");
    for (int i = 0; i < N/2; i++) {
        double magnitude = sqrt(real[i] * real[i] + imag[i] * imag[i]);
        double frequency = i * sample_rate / N;
        results[i].frequency = frequency;
        results[i].magnitude = magnitude;
#if 0
        if(i < 100)
            printf("Frequency: %.2fHz, Magnitude: %.2f\n", results[i].frequency, results[i].magnitude);
#endif
    }

    printf("Test Frequency: %.2fHz\n", test_frequency);

    qsort(results, N/2, sizeof(FrequencyMagnitude), compare);

    if(verbose_p) {
    printf("Top 10 Frequencies and Magnitudes:\n");
    for (int i = 0; i < 10; i++) {
        printf("Frequency: %.2fHz, Magnitude: %.2f\n", results[i].frequency, results[i].magnitude);
    }
    }

    return results[0].frequency;
}

double generate_noise() {
    return (2.0 * ((double)rand() / RAND_MAX) - 1.0) * NOISE_AMPLITUDE;
}

double* generate_test_signal(double frequency, double duration, int* num_samples) {
    // srand(time(NULL)); // Seed the random number generator

    *num_samples = SAMPLE_RATE * duration;
    double* signal = (double*)malloc(*num_samples * sizeof(double));
    if (signal == NULL) {
        printf("Memory allocation failed.\n");
        return NULL;
    }

    double angular_frequency = 2.0 * M_PI * frequency;
    double dt = 1.0 / SAMPLE_RATE;

    for (int i = 0; i < *num_samples; i++) {
        double t = i * dt;
        double noise = generate_noise();
        signal[i] = AMPLITUDE * sin(angular_frequency * t) + noise;
    }

    return signal;
}

double *generate_test_signal2(double targetFrequency, double duration, int *num_samples) {

    *num_samples = SAMPLE_RATE * duration;
    double* signal = (double*)malloc(*num_samples * sizeof(double));


    double timeStep = 1.0 / SAMPLE_RATE; // Time between samples
    double angularFrequency = 2 * M_PI * targetFrequency; // Angular frequency of the target frequency
    double phaseIncrement = angularFrequency * timeStep; // Phase increment per sample

    double phase = 0.0;
    for (int i = 0; i < *num_samples; i++) {
        signal[i] = sin(phase); // Generate a sinusoidal signal
        phase += phaseIncrement;
        while(phase > 2 * M_PI) {
            phase -= 2 * M_PI; // Wrap phase to stay within [0, 2*pi)
        }
    }
    return signal;
}

static void check_result(char *name, double test_frequency, 
    double *real, double *imag, unsigned N, int verbose_p) 
{
    // Print the top 10 results along with the test frequency
        double got = 
            print_top_10_results(N, real, imag, SAMPLE_RATE, test_frequency, verbose_p);

        double diff = fabs(got - test_frequency);

        printf("%s: samples=%d: expect top result to be about %f, is %f (diff=%f)\n", name, N, test_frequency, got, diff);

        if(diff > ERROR) {
            printf("too much error: got %f, expected about %f\n", 
                got,test_frequency);
            exit(1);
        }
}

double *vec_new(unsigned N) {
    double *v = malloc(N * sizeof(double));
    for(unsigned i = 0; i < N; i++)
        v[i] = 0.0;
    return v;
}

double *vec_dup(double *input, unsigned N) {
    double *v = vec_new(N);
    for(unsigned i = 0; i < N; i++)
        v[i] = input[i];
    return v;
}

void print_freq_bins(double freq, double* real, double* imag, unsigned N) {
    double sample_rate = SAMPLE_RATE;
    unsigned bin_i = (freq * N) / sample_rate;

    assert(bin_i > 10);

    printf("about to print 20 bins\n");
    for(int i = bin_i - 10; i < bin_i+10; i++) {
        double magnitude = sqrt(real[i] * real[i] + imag[i] * imag[i]);
        double frequency = i * sample_rate / N;
        printf("Frequency: %.2fHz, Magnitude: %.2f\n", 
            frequency, magnitude);
    }
}

int main() {
    double freq = 80.0;
    int N;

    double *signal = generate_test_signal2(freq, DURATION, &N);
    
    double* real = vec_dup(signal,N);
    double *imag = vec_new(N);

    fft(N, real, imag);
    check_result("raw", freq, real,imag,N,0);

    return 0;
}

