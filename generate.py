# ! python3
import struct
import numpy as np
import tqdm as tqdm


def generate(count):
    return np.random.rand(count, count), \
        np.random.rand(count, count)


def write_text(filename, first, second):
    row_count = first.shape[0]

    with open(filename, 'w') as fout:
        fout.write(f'{row_count} \n')

        for row in first:
            fout.write(" ".join(map(str, row)) + "\n")

        for row in second:
            fout.write(" ".join(map(str, row)) + "\n")


def write_binary(filename, first, second):
    row_count = first.shape[0]

    with open(filename, "wb") as fout:
        fout.write(struct.pack("Q", row_count))
        fout.write(first.astype(np.float64).tobytes())
        fout.write(second.astype(np.float64).tobytes())


def main():
    for dim in tqdm.tqdm([5, 10, 50, 100, 500, 1000, 5000]):
        first, second = generate(dim)
        write_text(f"./data/{dim}.txt", first, second)
        write_binary(f"./data/{dim}.bin", first, second)


if __name__ == "__main__":
    main()
