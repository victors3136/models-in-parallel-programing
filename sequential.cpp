#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <random>

double randomDouble() {
    static std::random_device random_device{};
    static std::mt19937 generator{random_device()};
    static std::uniform_real_distribution distribution{0.0, 1.0};
    return distribution(generator);
}

bool isBinaryFile(const std::string &filename) {
    if (filename.length() >= 4) {
        return filename.compare(filename.length() - 4, 4, ".bin") == 0;
    }
    return false;
}

class SquareMatrix : protected std::vector<double> {
    explicit SquareMatrix(const size_t dimension) : matrix_dimension(dimension) {
    }

    const size_t matrix_dimension{};

    static SquareMatrix multiplySequential(const SquareMatrix &first, const SquareMatrix &second) {
        if (first.matrix_dimension != second.matrix_dimension) {
            throw std::runtime_error{
                "Invalid multiplication between square matrices of size "
                + std::to_string(first.matrix_dimension) + " and "
                + std::to_string(second.matrix_dimension)
            };
        }
        const auto dim = first.matrix_dimension;
        SquareMatrix result{dim};
        result.assign(dim * dim, 0.0);
        for (auto i = 0ul; i < dim; ++i) {
            for (auto k = 0ul; k < dim; ++k) {
                for (auto j = 0ul; j < dim; ++j) {
                    result[i * dim + j] += first[i * dim + k] * second[k * dim + j];
                }
            }
        }
        return result;
    }

    static SquareMatrix multiplySequentialWithTranspose(
        const SquareMatrix &A,
        const SquareMatrix &B) {
        const size_t dim = A.matrix_dimension;

        SquareMatrix B_transpose{dim};
        B_transpose.assign(dim * dim, 0.0);

        for (auto i = 0ull; i < dim; ++i) {
            for (auto j = 0ull; j < dim; ++j) {
                B_transpose[j * dim + i] = B[i * dim + j];
            }
        }

        SquareMatrix C{dim};
        C.assign(dim * dim, 0.0);

        for (auto i = 0ull; i < dim; ++i) {
            const auto a_row = A.cbegin() + i * dim;
            for (auto j = 0ull; j < dim; ++j) {
                const auto b_row = B_transpose.cbegin() + j * dim;

                C[i * dim + j] = std::transform_reduce(
                    a_row,
                    a_row + dim,
                    b_row,
                    0.0
                );
            }
        }

        return C;
    }

    static void generateTxtFile(
        const size_t matrix_dimension,
        const std::string &filename) {
        std::ofstream fout{filename};
        for (auto i = 0ul; i < matrix_dimension * matrix_dimension; ++i) {
            fout << randomDouble() << " ";
        }
        fout.close();
    }

    static void generateBinFile(
        const size_t matrix_dimension,
        const std::string &filename) {
        std::ofstream fout{filename, std::ios::binary};
        for (auto i = 0ul; i < matrix_dimension * matrix_dimension; ++i) {
            const auto random_double = randomDouble();
            fout.write(reinterpret_cast<const char *>(&random_double), sizeof(random_double));
        }
        fout.close();
    }

    static SquareMatrix fromTxtFile(
        const size_t matrix_dimension,
        const std::string &filename) {
        SquareMatrix self{matrix_dimension};
        const auto matrix_size = matrix_dimension * matrix_dimension;
        self.resize(matrix_size);
        std::ifstream fin{filename};
        if (!fin) {
            generateTxtFile(matrix_dimension, filename);
            fin.open(filename);
            if (!fin) {
                throw std::runtime_error("Unable to open " + filename);
            }
        }
        for (auto i = 0ul; i < matrix_size; ++i) {
            if (!(fin >> self[i])) { throw std::runtime_error("Invalid input: File does not have enough elements"); }
        }
        fin.close();
        return self;
    }

    static SquareMatrix fromBinaryFile(
        const size_t matrix_dimension,
        const std::string &filename) {
        SquareMatrix self{matrix_dimension};
        const auto matrix_size = matrix_dimension * matrix_dimension;
        self.resize(matrix_size);
        std::ifstream fin{filename, std::ios::binary};
        if (!fin) {
            generateBinFile(matrix_dimension, filename);
            fin.open(filename, std::ios::binary);
            if (!fin) {
                throw std::runtime_error("Unable to open " + filename);
            }
        }
        fin.read(reinterpret_cast<char *>(self.data()), sizeof(double) * matrix_size);
        fin.close();
        return self;
    }

public:
    static SquareMatrix fromFile(
        const size_t matrix_dimension,
        const std::string &filename) {
        return isBinaryFile(filename)
                   ? fromBinaryFile(matrix_dimension, filename)
                   : fromTxtFile(matrix_dimension, filename);
    }

    SquareMatrix operator*(const SquareMatrix &other) const noexcept {
        return multiplySequentialWithTranspose(*this, other);
    }

    void writeTo(const std::string &filename) const {
        if (isBinaryFile(filename)) {
            std::ofstream fout{filename, std::ios::binary};
            fout.write(reinterpret_cast<const char *>(data()), sizeof(double) * matrix_dimension * matrix_dimension);
            fout.close();
        } else {
            std::ofstream fout{filename};
            for (auto i = this->cbegin(); i < this->cend(); ++i) {
                fout << *i << " ";
            }
            fout.close();
        }
    }
};

int main(const int argc, const char *argv[]) {
    if (argc < 5) {
        std::cerr << "usage: <matrix-dimension> <input-file-A> <input-file-b> <output-file>";
        return 1;
    }

    const auto matrix_size = std::atol(argv[1]);
    const auto start_t = std::chrono::high_resolution_clock::now();
    const auto read_start_t = std::chrono::high_resolution_clock::now();
    const auto A = SquareMatrix::fromFile(matrix_size, argv[2]);
    const auto B = SquareMatrix::fromFile(matrix_size, argv[3]);
    const auto read_end_t = std::chrono::high_resolution_clock::now();
    const auto work_start_t = std::chrono::high_resolution_clock::now();
    const auto C = A * B;
    const auto work_end_t = std::chrono::high_resolution_clock::now();
    const auto write_start_t = std::chrono::high_resolution_clock::now();
    C.writeTo(argv[4]);
    const auto write_end_t = std::chrono::high_resolution_clock::now();
    const auto end_t = std::chrono::high_resolution_clock::now();
    const auto read_t = std::chrono::duration<double>(read_end_t - read_start_t).count();
    const auto work_t = std::chrono::duration<double>(work_end_t - work_start_t).count();
    const auto write_t = std::chrono::duration<double>(write_end_t - write_start_t).count();
    const auto total_t = std::chrono::duration<double>(end_t - start_t).count();
    std::cout << std::fixed << std::setw(12) << std::setprecision(10)
            << matrix_size << "\t" << read_t
            << "\t" << work_t << "\t"
            << write_t << "\t" << total_t
            << std::endl;
    return 0;
}
