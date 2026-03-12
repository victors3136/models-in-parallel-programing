#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <string>

using Matrix = std::vector<double>;

constexpr size_t index(const size_t i, const size_t j, const size_t n) noexcept {
    return i * n + j;
}

class Multiplier {
protected:
    Matrix first{}, second{}, output{};
    size_t size{};

    void multiplySequential() {
        for (auto i = 0; i < size; ++i) {
            for (auto k = 0; k < size; ++k) {
                const auto a = first[index(i, k, size)];
                for (auto j = 0; j < size; ++j) {
                    output[index(i, j, size)] += a * second[index(k, j, size)];
                }
            }
        }
    }

    virtual bool read(const std::string &) =0;

    virtual bool write(const std::string &) =0;

public:
    virtual ~Multiplier() = default;

    void measure(const std::string &infile, const std::string &outfile) {
        std::cout << "Measuring duration for " << infile << "\n";

        const auto readStart = std::chrono::high_resolution_clock::now();
        if (!read(infile)) {
            std::cout << "Error reading " << infile << "\n";
            return;
        }
        const auto readEnd = std::chrono::high_resolution_clock::now();
        const auto readDuration = std::chrono::duration<double>(readEnd - readStart).count();

        std::cout << "N : " << size << ":\n";
        std::cout << "Read : " << readDuration << " seconds\n";

        const auto workStart = std::chrono::high_resolution_clock::now();
        multiplySequential();
        const auto workEnd = std::chrono::high_resolution_clock::now();
        const auto workDuration = std::chrono::duration<double>(workEnd - workStart).count();

        std::cout << "Work : " << workDuration << " seconds\n";

        const auto writeStart = std::chrono::high_resolution_clock::now();
        if (!write(outfile)) {
            std::cout << "Error reading " << outfile << "\n";
            return;
        }
        const auto writeEnd = std::chrono::high_resolution_clock::now();
        const auto writeDuration = std::chrono::duration<double>(writeEnd - writeStart).count();

        std::cout << "Write : " << writeDuration << " seconds\n";
    }
};

class TextMultiplier : public Multiplier {
    bool read(const std::string &filename) override {
        std::ifstream fin(filename);
        if (!fin) {
            std::cout << "Can't open " << filename << std::endl;
            return false;
        }
        fin >> size;
        const auto elementCount = size * size;
        first.resize(elementCount);
        second.resize(elementCount);
        output.assign(elementCount, 0.0);
        for (auto i = 0; i < elementCount; ++i) {
            fin >> first[i];
        }
        for (auto i = 0; i < elementCount; ++i) {
            fin >> second[i];
        }

        return true;
    }

    bool write(const std::string &filename) override {
        std::ofstream fout(filename);
        if (!fout) {
            std::cout << "Can't open " << filename << std::endl;
            return false;
        }

        fout << size << "\n";
        for (auto i = 0; i < size; ++i) {
            for (auto j = 0; j < size; ++j) {
                fout << output[index(i, j, size)] << " ";
            }
        }
        fout << "\n";

        return true;
    }
};

class BinaryMultiplier : public Multiplier {
    bool read(const std::string &filename) override {
        std::cout << filename << "\n";
        std::ifstream fin(filename, std::ios::binary);
        if (!fin) {
            std::cout << "Can't open " << filename << std::endl;
            return false;
        }

        fin.read(reinterpret_cast<char *>(&size), sizeof(size_t));
        const auto elementCount = size * size;
        first.resize(elementCount);
        second.resize(elementCount);
        output.assign(elementCount, 0.0);
        fin.read(reinterpret_cast<char *>(first.data()), sizeof(double) * elementCount);
        fin.read(reinterpret_cast<char *>(second.data()), sizeof(double) * elementCount);

        return true;
    }

    bool write(const std::string &filename) override {
        std::ofstream fout(filename, std::ios::binary);
        if (!fout) {
            std::cout << "Can't open " << filename << std::endl;
            return false;
        }

        fout.write(reinterpret_cast<char *>(&size), sizeof(size_t));
        fout.write(reinterpret_cast<char *>(output.data()), sizeof(double) * size * size);

        return true;
    }
};

int main() {
    const std::string data_dir{"./data/"};
    for (const auto &dimension: {5, 10, 50, 100, 500, 1'000, 5'000}) {
        const auto file = data_dir + std::to_string(dimension);
        const std::string infile_txt{file + ".txt"};
        const std::string infile_bin{file + ".bin"};
        const std::string outfile_txt{infile_txt + ".out"};
        const std::string outfile_bin{infile_bin + ".out"};
        TextMultiplier tex{};
        BinaryMultiplier bin{};
        tex.measure(infile_txt, outfile_txt);
        bin.measure(infile_bin, outfile_bin);
    }
    return 0;
}
