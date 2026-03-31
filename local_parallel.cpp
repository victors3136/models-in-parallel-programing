#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

enum class ThreadCount: uint8_t {
    Single = 1, Twenty = 20, Fourty = 40
};

static auto localThreadCount = ThreadCount::Single;

static size_t getThreadCount()noexcept {
    return static_cast<size_t>(localThreadCount);
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
    explicit SquareMatrix(const size_t dimension) : matrix_dimension(dimension), data(dimension * dimension) {
    }


    std::vector<double> transpose() const & noexcept {
        std::vector<double> transposed_matrix(matrix_dimension * matrix_dimension);
        for (auto i = 0ull; i < matrix_dimension; ++i) {
            for (auto j = 0ull; j < matrix_dimension; ++j) {
                transposed_matrix[j * matrix_dimension + i] = (*this)[i * matrix_dimension + j];
            }
        }
        return transposed_matrix;
    }
    static void generateFile(
        const size_t matrix_dimension,
        const std::string &filename) {
        std::ofstream fout{filename, std::ios::binary};
        for (auto i = 0ull; i < matrix_dimension * matrix_dimension; ++i) {
            const auto random_double = randomDouble();
            fout.write(reinterpret_cast<const char *>(&random_double), sizeof(random_double));
        }
        fout.close();
    }

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

    double* operator*() {
        return this->data.data();
    }
    const double* operator*() const {
        return this->data.data();
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
    size_t size() const {
        return data.size();
    }
    
public:
    double operator[](const size_t index) const {
        if(index < 0 || index >= data.size()) {
            throw std::runtime_error("Index " + std::to_string(index) + " invalid for matrix of dimension " + std::to_string(data.size()));
        }
        return data[index];
    }

    static SquareMatrix fromFileSequential(
        const size_t matrix_dimension,
        const std::string &filename) {
        SquareMatrix self{matrix_dimension};
        const auto matrix_size = matrix_dimension * matrix_dimension;
        std::ifstream fin{filename, std::ios::binary};
        if (!fin) {
            generateFile(matrix_dimension, filename);
            fin.open(filename, std::ios::binary);
            if (!fin) {
                throw std::runtime_error("Unable to open " + filename);
            }
        }
        fin.read(reinterpret_cast<char *>(*self), sizeof(double) * matrix_size);
        fin.close();
        return self;
    }
    static SquareMatrix fromFileParallel(
        const size_t matrix_dimension,
        const std::string &filename) {
    
        SquareMatrix self{matrix_dimension};
        const auto total_elements = matrix_dimension * matrix_dimension;
        std::ifstream check_fin{filename, std::ios::binary};
    
        if (!check_fin) {
            generateFile(matrix_dimension, filename);
        } else {
            check_fin.close();
        }

        const auto num_threads = getThreadCount();
        std::vector<std::thread> threads;
        threads.reserve(num_threads);

        const auto chunk_size = total_elements / num_threads;
        const size_t remainder = total_elements % num_threads;

        size_t current_index = 0;
        double* raw_data = *self;

        for (auto i = 0ull; i < num_threads; ++i) {
            auto start_idx = current_index;
            auto count = chunk_size + (i < remainder ? 1 : 0);
            current_index += count;

            if (count > 0) {
                threads.emplace_back([start_idx, count, raw_data, &filename]() {
                    std::ifstream fin{filename, std::ios::binary};
                    if (!fin) {
                        throw std::runtime_error("Thread failed to open tap for " + filename);
                    }
                    fin.seekg(start_idx * sizeof(double), std::ios::beg);
                    fin.read(reinterpret_cast<char*>(raw_data + start_idx), count * sizeof(double));
                });
            }
        }
        for (auto& t : threads) {
            t.join();
        }
        return self;
    }
    static SquareMatrix fromMockData(size_t matrix_dimension, std::vector<double>&& data) {
        if (data.size() != matrix_dimension * matrix_dimension) {
            throw std::runtime_error("Invalid matrix for dimension " + std::to_string(matrix_dimension));
        }
        SquareMatrix self{matrix_dimension};
        self.data = data;
        return self;
    }
    SquareMatrix operator*(const SquareMatrix &other) const noexcept {
        return multiplyParallelWithTranspose(*this, other);
    }

    void writeTo(const std::string &filename) const {
        std::ofstream fout{filename, std::ios::binary};
        fout.write(reinterpret_cast<const char *>(**this), sizeof(double) * size());
        fout.close();
    }

    bool operator==(const SquareMatrix &other) const noexcept {
        constexpr auto epsilon = 1e-10;
        if (size() != other.size()) return false;
        for (auto i = 0ull; i < size(); ++i) {
            if (std::abs((*this)[i] - other[i]) > epsilon) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const SquareMatrix &other) const {
        return !(other == *this);
    }
    static void testIdentity() {
        size_t dim = 10; 
        SquareMatrix A{dim};
        for(size_t i = 0; i < dim * dim; ++i) (*A)[i] = randomDouble();
        
        SquareMatrix I = SquareMatrix::Identity(dim);
        SquareMatrix result = A * I;
        
        assert(result == A && "Matrix * Identity must equal itself");
        std::cout << "Identity Test: [PASS]\n";
    }

    static void testScaling() {
        size_t dim = 4;
        SquareMatrix Ones = SquareMatrix::AllOnes(dim);
        SquareMatrix result = Ones * Ones;
        
        for(size_t i = 0; i < dim * dim; ++i) {
            assert(std::abs(result[i] - static_cast<double>(dim)) < 1e-10 
                   && "Each element in Ones*Ones must equal the dimension");
        }
        std::cout << "Scaling Test: [PASS]\n";
    }

    static void testParallelConsistency() {
        size_t dim = 64; // Larger dim to ensure threads actually spawn
        SquareMatrix A{dim}, B{dim};
        for(size_t i = 0; i < dim * dim; ++i) {
            (*A)[i] = randomDouble();
            (*B)[i] = randomDouble();
        }

        SquareMatrix resSeq = SquareMatrix::multiplySequentialWithTranspose(A, B);
        SquareMatrix resPar = SquareMatrix::multiplyParallelWithTranspose(A, B);

        assert(resSeq == resPar && "Parallel and Sequential results must match exactly");
        std::cout << "Consistency Test: [PASS]\n";
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
    static SquareMatrix multiplyParallelWithTranspose(
        const SquareMatrix &A,
        const SquareMatrix &B) noexcept {
        const auto dim = A.matrix_dimension;
        const auto B_transpose = B.transpose();

        SquareMatrix C{dim};
        auto cData = *C;

        const size_t thread_count = getThreadCount();
        std::vector<std::thread> threads;
        threads.reserve(thread_count);

        const auto worker = [&](size_t start_row, size_t end_row) -> void {
            for (auto i = start_row; i < end_row; ++i) {
                const auto a_row = A.cbegin() + i * dim;
                for (auto j = 0ull; j < dim; ++j) {
                    const auto b_row = B_transpose.cbegin() + j * dim;
                    auto accumulator = 0.0;
                    for (auto k = 0ull; k < dim; ++k) {
                        accumulator += a_row[k] * b_row[k];
                    }
                    cData[i * dim + j] = accumulator;
                }
            }
        };

        const auto chunk_size = dim / thread_count;
        const auto remainder = dim % thread_count;
        auto current_row = 0ull;

        for (auto i = 0ull; i < thread_count; ++i) {
            auto start_row = current_row;
            auto end_row = start_row + chunk_size + (i < remainder ? 1 : 0);
            current_row = end_row;

            if (start_row < end_row) {
                threads.emplace_back(worker, start_row, end_row);
            }
        }
        for (auto& thread : threads) {
            thread.join();
        }

        return C;
    }

};

int main(const int argc, const char *argv[]) {
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
    const auto seq_work_start_t = std::chrono::high_resolution_clock::now();
    const auto C_seq = SquareMatrix::multiplySequentialWithTranspose(A, B);
    const auto seq_work_end_t = std::chrono::high_resolution_clock::now();
    const auto par_work_start_t = std::chrono::high_resolution_clock::now();
    const auto C_par = SquareMatrix::multiplyParallelWithTranspose(A, B);
    const auto par_work_end_t = std::chrono::high_resolution_clock::now();
    const auto write_start_t = std::chrono::high_resolution_clock::now();
    C_par.writeTo(argv[4]);
    const auto write_end_t = std::chrono::high_resolution_clock::now();
    if (C_par != C_seq) {
        throw std::runtime_error("Failed :(");
    }
    const auto end_t = std::chrono::high_resolution_clock::now();
    const auto read_t = std::chrono::duration<double>(read_end_t - read_start_t).count();
    const auto seq_work_t = std::chrono::duration<double>(seq_work_end_t - seq_work_start_t).count();
    const auto par_work_t = std::chrono::duration<double>(par_work_end_t - par_work_start_t).count();
    const auto write_t = std::chrono::duration<double>(write_end_t - write_start_t).count();
    const auto total_t = std::chrono::duration<double>(end_t - start_t).count();
    std::cout << std::fixed << std::setw(12) << std::setprecision(10)
            << matrix_size << "\t" << read_t
            << "\t" << seq_work_t << "\t" << par_work_t << "\t"
            << "\t" << seq_work_t / par_work_t << "\t" << write_t << "\t" << total_t
            << std::endl;

    return 0;
}
