#include <iostream>
#include <fstream>
#include <vector>
#include <omp.h>
#include <algorithm>
#include <chrono>
#define THRESHOLD 1000

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

void mergesort(std::vector<std::uint64_t>& vec, int left, int right) {
  // serial sections
  if (right <= left) { return; }
  auto middle = (right + left)/2;

  mergesort(vec, left, middle);
  mergesort(vec, middle+1, right);

  // merge the two sorted halves
  auto begin = vec.begin();
  std::inplace_merge(begin + left, begin + middle + 1, begin + right + 1); 
}

void parallel_mergesort(std::vector<std::uint64_t>& vec, int left, int right) {
  // serial sections
  if ((right - left) <= THRESHOLD) { mergesort(vec, left, right); return; }
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

int main(int argc, char** argv) {
  // used as 'parsort [number of processors to use] [input file to sort]'
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " [num_processors] [input_file_path]" << std::endl;
    return 1;
  }
  
  // parse the command-line arguments
  int num_threads = std::stoi(argv[1]);
  std::string input_file_name{argv[2]};

  auto numbers = load_file_contents(input_file_name);  

  // time the sort. spoiler: it's pretty bad
  auto start_time = std::chrono::high_resolution_clock::now();
  parallel_sort(numbers, num_threads);
  auto end_time = std::chrono::high_resolution_clock::now();
  auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);

  // print [sort name] [time] \n [sorted array]
  std::cout << "MergeSort: " << duration_us.count() << std::endl;
  for (auto e: numbers) { std::cout << e << " "; }
  return 0;
}