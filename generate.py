# ! python3
import os

import numpy as np
import tqdm


def generate(count):
    return np.random.rand(count, count), \
        np.random.rand(count, count)


def write_text(filename, matrix):
    with open(filename, 'w') as fout:
        for row in matrix:
            fout.write(" ".join(map(str, row)) + "\n")


def write_binary(filename, matrix):
    with open(filename, "wb") as fout:
        fout.write(matrix.astype(np.float64).tobytes())


def main():
    data_dir = "./data/"
    if not os.path.exists(data_dir):
        os.makedirs(data_dir)

    dimensions = [5, 10, 50, 100, 500, 1000, 5000]

    for dim in tqdm.tqdm(dimensions):
        first, second = generate(dim)

        file_base = os.path.join(data_dir, str(dim))

        write_text(f"{file_base}A.txt", first)
        write_text(f"{file_base}B.txt", second)

        write_binary(f"{file_base}A.bin", first)
        write_binary(f"{file_base}B.bin", second)


if __name__ == "__main__":
    main()
