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
public:
    explicit Multiplier(const size_t size) : size(size) {
        first.resize(size * size);
        second.resize(size * size);
        output.assign(size * size, 0.0);
    }

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

    virtual bool readA(const std::string &) =0;

    virtual bool readB(const std::string &) =0;

    bool read(const std::string &filenameA, const std::string &filenameB) {
        return readA(filenameA) && readB(filenameB);
    }

    virtual bool write(const std::string &) =0;

public:
    virtual ~Multiplier() = default;

    void measure(const std::string &infileA, const std::string &infileB, const std::string &outfile) {
        std::cout << "Measuring duration for " << infileA << " and " << infileB << "\n";

        const auto readStart = std::chrono::high_resolution_clock::now();
        if (!read(infileA, infileB)) {
            std::cout << "Error reading " << infileA << " or " << infileB << "\n";
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
public:
    explicit TextMultiplier(const size_t size)
        : Multiplier(size) {
    }

private:
    bool readA(const std::string &filename) override {
        std::ifstream fin(filename);
        if (!fin) {
            std::cout << "Can't open " << filename << std::endl;
            return false;
        }
        for (auto i = 0; i < size * size; ++i) {
            fin >> first[i];
        }
        return true;
    }

    bool readB(const std::string &filename) override {
        std::ifstream fin(filename);
        if (!fin) {
            std::cout << "Can't open " << filename << std::endl;
            return false;
        }
        for (auto i = 0; i < size * size; ++i) {
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
public:
    explicit BinaryMultiplier(const size_t size)
        : Multiplier(size) {
    }

private:
    bool readA(const std::string &filename) override {
        std::cout << filename << "\n";
        std::ifstream fin(filename, std::ios::binary);
        if (!fin) {
            std::cout << "Can't open " << filename << std::endl;
            return false;
        }
        fin.read(reinterpret_cast<char *>(first.data()), sizeof(double) * size * size);
        return true;
    }

    bool readB(const std::string &filename) override {
        std::cout << filename << "\n";
        std::ifstream fin(filename, std::ios::binary);
        if (!fin) {
            std::cout << "Can't open " << filename << std::endl;
            return false;
        }
        fin.read(reinterpret_cast<char *>(first.data()), sizeof(double) * size * size);
        return true;
    }

    bool write(const std::string &filename) override {
        std::ofstream fout(filename, std::ios::binary);
        if (!fout) {
            std::cout << "Can't open " << filename << std::endl;
            return false;
        }
        fout.write(reinterpret_cast<char *>(output.data()), sizeof(double) * size * size);
        return true;
    }
};

int main() {
    const std::string data_dir{"./data/"};
    for (const size_t &dimension: {5, 10, 50, 100, 500, 1'000, 5'000}) {
        const auto file = data_dir + std::to_string(dimension);
        const std::string infileA_txt{file + "A.txt"};
        const std::string infileA_bin{file + "A.bin"};
        const std::string infileB_txt{file + "B.txt"};
        const std::string infileB_bin{file + "B.bin"};
        const std::string outfile_txt{file + ".txt.out"};
        const std::string outfile_bin{file + ".bin.out"};
        TextMultiplier tex{dimension};
        BinaryMultiplier bin{dimension};
        std::cout << "Text with " << dimension << " dimensions\n";
        tex.measure(infileA_txt, infileB_txt, outfile_txt);
        std::cout << "Binary with " << dimension << " dimensions\n";
        bin.measure(infileA_bin, infileB_bin, outfile_bin);
    }
    return 0;
}
