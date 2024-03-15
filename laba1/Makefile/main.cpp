#include <iostream>
#include <cmath>
#include <vector>

#define PI 3.14159265358979323846

int main() {
    //const int arraySize = 10000000;
    const int size = 10000000; // Number of elements in the array

#ifdef DOUBLE
    std::cout << "double" << std::endl;
    std::vector<double> array(size);
#else
    std::cout << "float" << std::endl;
    std::vector<float> array(size);
#endif

    // Filling the array with sine values
    for (int i = 0; i < size; ++i) {
        double angle = (2 * PI * i) / size; // Calculate the angle for each element
        array[i] = std::sin(angle); // Store the sine value in the array
    }

    // Calculating the sum of the array elements
    double sum = 0.0;
    for (int i = 0; i < size; ++i) {
        sum += array[i];
    }

    // Output the sum to the terminal
    std::cout << "Sum: " << sum << std::endl;

    return 0;
}