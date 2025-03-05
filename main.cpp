#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

//TODO properly document this
struct VectorHash {
    size_t operator()(const std::vector<uint8_t> &v) const {
        size_t seed = v.size();
        for (uint8_t i: v) {
            std::hash<uint8_t> hasher;
            seed ^= hasher(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }
};

std::vector<unsigned int> encoding(const std::vector<uint8_t> &input) {
    std::unordered_map<std::vector<uint8_t>, unsigned int, VectorHash> table;
    for (unsigned int i = 0; i <= 255; i++) {
        table[{static_cast<uint8_t>(i)}] = i;
    }

    std::vector<uint8_t> p;
    unsigned int code = 256;
    std::vector<unsigned int> output;

    for (uint8_t c: input) {
        std::vector<uint8_t> pc = p;
        pc.push_back(c);
        if (table.contains(pc)) {
            p = pc;
        } else {
            output.push_back(table[p]);
            table[pc] = code++;
            p = {c};
        }
    }
    if (!p.empty()) {
        output.push_back(table[p]);
    }

    return output;
}

std::vector<uint8_t> decoding(const std::vector<unsigned int> &output) {
    std::unordered_map<unsigned int, std::vector<uint8_t> > table;
    for (unsigned int i = 0; i <= 255; i++) {
        table[i] = {static_cast<uint8_t>(i)};
    }

    if (output.empty()) {
        return {};
    }

    std::vector<uint8_t> decoded;
    std::vector<uint8_t> p = table[output[0]];
    decoded.insert(decoded.end(), p.begin(), p.end());
    unsigned int code = 256;

    for (size_t i = 1; i < output.size(); i++) {
        std::vector<uint8_t> current;
        if (table.contains(output[i])) {
            current = table[output[i]];
        } else if (output[i] == code) {
            current = p;
            current.push_back(p[0]);
        } else {
            return {}; // Error
        }

        decoded.insert(decoded.end(), current.begin(), current.end());
        std::vector<uint8_t> pc = p;
        pc.insert(pc.end(), current.begin(), current.begin() + 1);
        table[code++] = pc;
        p = current;
    }

    return decoded;
}

int main() {
    std::filesystem::path input_filename = "../assets/input1.bin", compressed_filename =
            "../assets/compressed/input1.bin.lzw";

    std::ifstream infile(input_filename, std::ios::binary | std::ios::ate);
    if (!infile.is_open()) {
        std::cerr << "Unable to open file for reading: " << input_filename.string() << std::endl;
        return 1;
    }

    std::streamsize size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    std::vector<uint8_t> file_data(size);
    infile.read(reinterpret_cast<char *>(file_data.data()), size);
    infile.close();

    std::vector<unsigned int> compressed_data = encoding(file_data);

    std::ofstream outfile(compressed_filename, std::ios::binary);
    if (!outfile.is_open()) {
        std::cerr << "Unable to open file for writing: " << compressed_filename.string() << std::endl;
        return 1;
    }
    outfile.write(reinterpret_cast<const char *>(compressed_data.data()),
                  compressed_data.size() * sizeof(unsigned int));
    outfile.close();

    std::ifstream compressed_infile(compressed_filename, std::ios::binary | std::ios::ate);
    if (!compressed_infile.is_open()) {
        std::cerr << "Unable to open compressed file for reading: " << compressed_filename.string() << std::endl;
        return 1;
    }
    std::streamsize compressed_size = compressed_infile.tellg();
    compressed_infile.seekg(0, std::ios::beg);

    std::vector<unsigned int> read_compressed_data(compressed_size / sizeof(unsigned int));
    compressed_infile.read(reinterpret_cast<char *>(read_compressed_data.data()), compressed_size);
    compressed_infile.close();

    std::vector<uint8_t> decompressed_data = decoding(read_compressed_data);

    if (file_data == decompressed_data) {
        std::cout << "Compression & decompression successful" << std::endl;
        std::cout << "Original size: " << file_data.size() << std::endl;
        std::cout << "Compressed size (encoded integers): " << read_compressed_data.size() << std::endl;
    } else {
        std::cerr << "Decompression failed" << std::endl;
    }

    return 0;
}
