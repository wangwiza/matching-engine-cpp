# run this script in the scripts directory

import random
import subprocess

MAX_ITERATIONS = 1024

def generate_large_test_file(filename, num_threads=40, num_operations=200):
    # Generate a random seed for reproducibility
    seed = random.randint(1, 999999)
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

def main():
    test_file = "large_test_file.in"
    
    for i in range(1, MAX_ITERATIONS + 1):
        # Generate test file and get the seed
        seed = generate_large_test_file(test_file)
        
        # Run grader and capture output
        grader_stdout, grader_stderr = run_grader_on_test_file(test_file)
        
        # Check if there was an error in grader output
        if grader_stderr.strip() != 'test passed.':
            # Create a unique output file name based on the seed
            output_file = f"grader_output_seed_{seed}.out"
            
            # Record the seed and grader output to the unique output file
            with open(output_file, "w") as f:
                f.write(f"# Seed: {seed}\n\n")
                f.write("# Grader STDOUT:\n")
                f.write(grader_stdout + "\n")
                if grader_stderr:
                    f.write("\n# Grader STDERR:\n")
                    f.write(grader_stderr + "\n")
            
            print(f"Error detected! Grader output saved to: {output_file}")
            print(f"Stopping after {i} iterations.")
            return  # Exit on first error
        
        print(f"Iteration {i} passed without errors.")
    
    print("All iterations passed without errors.")

if __name__ == "__main__":
    main()
