#include <iostream>
#include <fstream>
#include <vector>
#include <omp.h>
#include <algorithm>

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
  if ((right-left) <= 0) {
    return;
  }

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
  std::inplace_merge(begin + left, begin + middle, begin + right); // still using as a crutch
}

void parallel_sort(std::vector<uint64_t>& vec, int num_threads) {
  // limit the number of threads
  // omp_set_num_threads(num_procs);

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
  std::cout << numbers.size();

  // sort the numbers
  // sort 

  // print timed results
  return 0;
}