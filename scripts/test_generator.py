# run this script in the scripts directory

import argparse
import random
import subprocess
from pprint import pprint
from string import ascii_lowercase
import os

def generate_large_test_file(seed, filename, num_threads, num_operations):
    random.seed(seed)

    symbols = [
        "AGDSAGSO", "AGFG", "AGOD", "GOOD", "AGOO", "GOOG", "DOOG", "GOOS",
        "GDSOGSOG", "GODG", "GFG", "ADOX", "AGOZ", "GFGO", "GOSF", "AGOOD"
    ]
    order_id = 1
    order_owner = {}  # Track which client created each order

    # Create the test file
    with open(filename, "w") as f:
        # Write the seed at the top of the file
        f.write(f"# seed: {seed}\n")

        # Number of client threads
        f.write(f"{num_threads}\n")

        # Sync all threads
        f.write(".\n")

        # Connect all threads to server
        f.write("o\n")

        # Generate random operations
        for _ in range(num_operations):
            thread_id = random.randint(0, num_threads - 1)
            symbol = random.choice(symbols)
            price = random.randint(1, 500)
            count = random.randint(1, 20)
            action = random.choice(["B", "S", "C"])

            if action in ["B", "S"]:
                # Buy or Sell operation with unique order ID
                f.write(f"{thread_id} {action} {order_id} {symbol} {price} {count}\n")
                order_owner[order_id] = thread_id  # Track order owner
                order_id += 1
            elif action == "C" and order_owner:
                # Cancel operation (only if there's an order to cancel)
                possible_orders = [oid for oid, owner in order_owner.items() if owner == thread_id]
                if possible_orders:
                    cancel_id = random.choice(possible_orders)
                    f.write(f"{thread_id} C {cancel_id}\n")

        # Disconnect all threads
        f.write("x\n")

    return seed

def run_grader_on_test_file(test_file, grader_path="../grader", engine_path="../engine"):
    try:
        # Run the grader using subprocess and capture the output
        result = subprocess.run(
            [grader_path, engine_path],
            stdin=open(test_file, "r"),
            capture_output=True,
            text=True,
            timeout=120  # Timeout in case of deadlock
        )
        return result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return "Grader timed out.", ""
    except Exception as e:
        return f"Error running grader: {str(e)}", ""

def run_till_error(test_file, num_threads, num_ops):
    generate_large_test_file(args.seed, test_file, num_threads=num_threads,
                                                     num_operations=num_ops)
    count = 0
    while True:
        grader_stdout, grader_stderr = run_grader_on_test_file(test_file)

        if grader_stderr.strip() != 'test passed.':
            print(f"Error detected! Runs: {count}")
            print(f"Grader STDOUT:\n{grader_stdout}")
            print(f"Grader STDERR:\n{grader_stderr}")
            return

        count += 1
        print(f"Iteration {count} ran.", end="\r")

def run_forever(test_file, output_file, num_threads, num_ops):
    count = 0
    errors = {}
    raw_error_count = 0

    while True:
        # Generate a random seed for reproducibility
        seed = random.randint(1, 2**32)
        generate_large_test_file(seed, test_file, num_threads=num_threads,
                                                    num_operations=num_ops)

        # Run grader and capture output
        _, grader_stderr = run_grader_on_test_file(test_file)

        # Check if there was an error in grader output
        if grader_stderr.strip() != 'test passed.':
            raw_error_count += 1
            if grader_stderr in errors:
                continue
            errors[grader_stderr] = (seed, args.num_threads, args.num_ops)
            with open(output_file, 'w') as f:
                pprint(errors, stream=f)

        count += 1
        print(f"Iteration {count} ran. Raw errors: {raw_error_count} Unique Errors: {len(errors)}", end="\r")

def get_rand_filpath():
    rand_filename = ''.join(random.choice(ascii_lowercase) for _ in range(12))
    return f"/tmp/{rand_filename}.in"

def main(args):
    test_file = get_rand_filpath()
    while os.path.exists(test_file):
        test_file = get_rand_filpath()
    print(f"Using tmp file: {test_file}")

    try:
        if args.seed:
            print(f"Running once with seed {args.seed}")
            run_till_error(test_file, args.num_threads, args.num_ops)
        else:
            print("Running forever")
            run_forever(test_file, args.output_file, args.num_threads, args.num_ops)
    except KeyboardInterrupt:
        if os.path.exists(test_file):
            os.remove(test_file)
        print("\nExiting...")
        exit(0)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Automated test case generator.")
    parser.add_argument("output_file", type=str, help="Output file to write the errors to.")
    parser.add_argument("--seed", type=int, help="If provided, run once with the given seed.")
    parser.add_argument("--num_threads", type=int, default=40, help="Number of client threads.")
    parser.add_argument("--num_ops", type=int, default=500, help="Number of operations to generate.")
    args = parser.parse_args()
    main(args)

