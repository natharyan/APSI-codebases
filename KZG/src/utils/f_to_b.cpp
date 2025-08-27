#include <fstream>
#include <vector>

std::vector<unsigned char> readFileToBytes(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    return std::vector<unsigned char>(std::istreambuf_iterator<char>(file), {});
}
