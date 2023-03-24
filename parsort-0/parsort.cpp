#include <iostream>
#include <fstream>
#include <vector>
#include <omp.h>
#include <algorithm>
#include <chrono>

std::vector<std::uint64_t> load_file_contents (const std::string& path) { 

  std::vector<std::uint64_t> vec{};
  std::ifstream file{path};

  if (!file.is_open()) {
    // handle the error if the file could not be opened
    std::cerr << "Error: could not open file." << std::endl;
    return {};
  }

  // read the numbers from the file
  std::uint64_t num;
  while (file >> num) {
    vec.push_back(num);
  }

  return vec;
}

void parallel_mergesort(std::vector<std::uint64_t>& vec, int left, int right) {
  // serial sections
	if (right <= left) { return; }
  auto middle = (right + left)/2;

  // make two tasks, one for each half
  #pragma omp task
  parallel_mergesort(vec, left, middle);
  #pragma omp task
  parallel_mergesort(vec, middle+1, right);

  // sync the tasks
  #pragma omp taskwait

  // merge the two sorted halves
  auto begin = vec.begin();
  std::inplace_merge(begin + left, begin + middle + 1, begin + right + 1); 
}

void parallel_sort(std::vector<uint64_t>& vec, int num_threads) {
  // spawn big boi thread army
  // let only one thread start the forking process
  #pragma omp parallel num_threads(num_threads)
  {
    #pragma omp single nowait 
    parallel_mergesort(vec, 0, vec.size() - 1);
  }
}

int main() {
  auto numbers = load_file_contents("test-files/input.txt");  

  auto start_time = std::chrono::high_resolution_clock::now();
  parallel_sort(numbers, 2);
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
  std::cout << "Merge Sort " << duration_us.count() << std::endl;
  for (auto e: numbers) { std::cout << e << " "; }
  return 0;
}