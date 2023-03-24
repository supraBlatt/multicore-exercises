#include <iostream>
#include <fstream>
#include <vector>

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


int main() {
  auto numbers = load_file_contents("test-files/input.txt");  
  std::cout << numbers.size();

  // sort the numbers
  // sort 

  // print timed results
  return 0;
}