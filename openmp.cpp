#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>
#include <omp.h>

enum class ThreadCount: uint8_t {
    Single = 1, Twenty = 20, Fourty = 40
};

static auto localThreadCount = ThreadCount::Single;

static int getThreadCount() noexcept {
    return static_cast<int>(localThreadCount);
}

static double randomDouble() {
    static std::random_device random_device{};
    static std::mt19937 generator{random_device()};
    static std::uniform_real_distribution<double> distribution{0.0, 1.0};
    return distribution(generator);
}

class SquareMatrix {
    const size_t matrix_dimension {};
    std::vector<double> data {};

    explicit SquareMatrix(const size_t dimension)
        : matrix_dimension(dimension), data(dimension * dimension) {}

        static SquareMatrix Identity(size_t dim) {
        SquareMatrix I{dim};
        double* data = *I;
        for (size_t i = 0; i < dim; ++i) {
            data[i * dim + i] = 1.0;
        }
        return I;
    }

    static SquareMatrix AllOnes(size_t dim) {
        SquareMatrix O{dim};
        double* data = *O;
        for (size_t i = 0; i < dim * dim; ++i) {
            data[i] = 1.0;
        }
        return O;
    }

    void resize(size_t newSize) {
        this->data.resize(newSize);
    }

    std::vector<double>::const_iterator cbegin() const {
        return data.cbegin();
    }

    std::vector<double>::const_iterator cend() const {
        return data.cend();
    }

public:
    double* operator*() { return data.data(); }
    const double* operator*() const { return data.data(); }

    double operator[](const size_t index) const {
        return data.at(index);
    }

    size_t size() const { return data.size(); }

    std::vector<double> transpose() const & noexcept {
        std::vector<double> transposed_matrix(matrix_dimension * matrix_dimension);
        const auto dim = matrix_dimension;
        const double* src = data.data();
        double* dst = transposed_matrix.data();

        #pragma omp parallel for num_threads(getThreadCount()) schedule(static)
        for (size_t i = 0; i < dim; ++i) {
            for (size_t j = 0; j < dim; ++j) {
                dst[j * dim + i] = src[i * dim + j];
            }
        }
        return transposed_matrix;
    }

    static SquareMatrix fromFileParallel(const size_t matrix_dimension, const std::string &filename) {
        SquareMatrix self{matrix_dimension};
        const auto total_elements = matrix_dimension * matrix_dimension;
        double* raw_data = *self;

        #pragma omp parallel num_threads(getThreadCount())
        {
            int tid = omp_get_thread_num();
            int nthreads = omp_get_num_threads();

            size_t chunk = total_elements / nthreads;
            size_t start = tid * chunk;
            size_t count = (tid == nthreads - 1) ? (total_elements - start) : chunk;

            if (count > 0) {
                std::ifstream fin{filename, std::ios::binary};
                fin.seekg(start * sizeof(double), std::ios::beg);
                fin.read(reinterpret_cast<char*>(raw_data + start), count * sizeof(double));
            }
        }
        return self;
    }

    static SquareMatrix multiplyParallelWithTranspose(const SquareMatrix &A, const SquareMatrix &B) noexcept {
        const auto dim = A.matrix_dimension;
        const auto B_transpose = B.transpose();
        SquareMatrix C{dim};

        double* cData = *C;
        const double* aData = *A;
        const double* bTData = B_transpose.data();

        #pragma omp parallel for num_threads(getThreadCount()) collapse(2) schedule(static)
        for (size_t i = 0; i < dim; ++i) {
            for (size_t j = 0; j < dim; ++j) {
                double accumulator = 0.0;
                const double* rowA = &aData[i * dim];
                const double* rowBT = &bTData[j * dim];

                for (size_t k = 0; k < dim; ++k) {
                    accumulator += rowA[k] * rowBT[k];
                }
                cData[i * dim + j] = accumulator;
            }
        }
        return C;
    }

    static SquareMatrix fromFileSequential(size_t dim, const std::string& fn) {
        SquareMatrix self{dim};
        std::ifstream fin{fn, std::ios::binary};
        if(!fin) { generateFile(dim, fn); fin.open(fn, std::ios::binary); }
        fin.read(reinterpret_cast<char*>(*self), sizeof(double) * dim * dim);
        return self;
    }

    static void generateFile(size_t dim, const std::string& fn) {
        std::ofstream fout{fn, std::ios::binary};
        for (size_t i = 0; i < dim * dim; ++i) {
            double r = randomDouble();
            fout.write(reinterpret_cast<const char*>(&r), sizeof(double));
        }
    }

    bool operator==(const SquareMatrix &other) const noexcept {
        if (size() != other.size()) return false;
        for (size_t i = 0; i < size(); ++i) {
            if (std::abs(data[i] - other.data[i]) > 1e-10) return false;
        }
        return true;
    }

    bool operator!=(const SquareMatrix &other) const { return !(other == *this); }

    static void testIdentity() {
        size_t dim = 10;
        SquareMatrix A{dim};
        for(size_t i = 0; i < dim * dim; ++i) (*A)[i] = randomDouble();

        SquareMatrix I = SquareMatrix::Identity(dim);
        SquareMatrix result = A * I;

        assert(result == A && "Matrix * Identity must equal itself");
    }

    static void testScaling() {
        size_t dim = 4;
        SquareMatrix Ones = SquareMatrix::AllOnes(dim);
        SquareMatrix result = Ones * Ones;

        for(size_t i = 0; i < dim * dim; ++i) {
            assert(std::abs(result[i] - static_cast<double>(dim)) < 1e-10
                   && "Each element in Ones*Ones must equal the dimension");
        }
    }

    static void testParallelConsistency() {
        size_t dim = 64;
        SquareMatrix A{dim}, B{dim};
        for(size_t i = 0; i < dim * dim; ++i) {
            (*A)[i] = randomDouble();
            (*B)[i] = randomDouble();
        }

        SquareMatrix resSeq = SquareMatrix::multiplySequentialWithTranspose(A, B);
        SquareMatrix resPar = SquareMatrix::multiplyParallelWithTranspose(A, B);

        assert(resSeq == resPar && "Parallel and Sequential results must match exactly");
    }
        static SquareMatrix multiplySequentialWithTranspose(
        const SquareMatrix &A,
        const SquareMatrix &B) noexcept {
        const auto dim = A.matrix_dimension;

        const auto B_transpose = B.transpose();

        SquareMatrix C{dim};
        auto cData = *C;
        for (auto i = 0ull; i < dim; ++i) {
            const auto a_row = A.cbegin() + i * dim;
            for (auto j = 0ull; j < dim; ++j) {
                const auto b_row = B_transpose.cbegin() + j * dim;
                double s = 0.0;
                for (auto k = 0ull; k < dim; ++k) {
                    s += a_row[k] * b_row[k];
                }
                cData[i * dim + j] = s;
            }
        }
        return C;
    }


    SquareMatrix operator*(const SquareMatrix &other) const noexcept {
        return multiplyParallelWithTranspose(*this, other);
    }

    void writeTo(const std::string &filename) const {
        std::ofstream fout{filename, std::ios::binary};
        fout.write(reinterpret_cast<const char *>(data.data()), sizeof(double) * data.size());
    }
};

int main(int argc, char** argv) {
    localThreadCount = ThreadCount::Fourty;
    if (argc < 5) {
        std::cerr << "usage: <matrix-dimension> <input-file-A> <input-file-b> <output-file>";
        return 1;
    }
    SquareMatrix::testIdentity();
    SquareMatrix::testScaling();
    SquareMatrix::testParallelConsistency();

    const auto matrix_size = std::atol(argv[1]);
    const auto start_t = std::chrono::high_resolution_clock::now();
    const auto read_start_t = std::chrono::high_resolution_clock::now();
    const auto A = SquareMatrix::fromFileSequential(matrix_size, argv[2]);
    const auto B = SquareMatrix::fromFileSequential(matrix_size, argv[3]);
    const auto read_end_t = std::chrono::high_resolution_clock::now();
    const auto par_work_start_t = std::chrono::high_resolution_clock::now();
    const auto C_par = SquareMatrix::multiplyParallelWithTranspose(A, B);
    const auto par_work_end_t = std::chrono::high_resolution_clock::now();
    const auto write_start_t = std::chrono::high_resolution_clock::now();
    C_par.writeTo(argv[4]);
    const auto write_end_t = std::chrono::high_resolution_clock::now();
    const auto end_t = std::chrono::high_resolution_clock::now();
    const auto read_t = std::chrono::duration<double>(read_end_t - read_start_t).count();
    const auto par_work_t = std::chrono::duration<double>(par_work_end_t - par_work_start_t).count();
    const auto write_t = std::chrono::duration<double>(write_end_t - write_start_t).count();
    const auto total_t = std::chrono::duration<double>(end_t - start_t).count();
    std::cout << std::fixed << std::setw(12) << std::setprecision(10)
            << matrix_size << "\t" << read_t
            << "\t" << par_work_t << "\t"
            << write_t << "\t" << total_t
            << std::endl;

    return 0;
}
