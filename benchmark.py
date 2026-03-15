import sys
import subprocess
import os

def run_benchmark():
    if len(sys.argv) != 2:
        print("usage: benchmark <N>")
        sys.exit(1)

    n_value = sys.argv[1]
    directory = "data"
    fmt = "bin"

    file_a = os.path.join(directory, f"{n_value}.A.{fmt}")
    file_b = os.path.join(directory, f"{n_value}.B.{fmt}")
    file_out = os.path.join(directory, f"{n_value}.out.{fmt}")

    command = ["./sequential", n_value, file_a, file_b, file_out]

    try:
        # Dry run - is responsible for making sure the files are created before the actual benchmarks
        subprocess.run(command, stdout=subprocess.DEVNULL, check=True)

        for _ in range(10):
            subprocess.run(command, check=True)
            
    except subprocess.CalledProcessError as e:
        print(f"An error occurred while running the benchmark: {e}")
    except FileNotFoundError:
        print("Error: The executable './sequential' was not found.")

if __name__ == "__main__":
    run_benchmark()