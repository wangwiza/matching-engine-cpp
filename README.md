# ✨ Matching Engine ✨

A high-performance, low-latency matching engine for financial instruments, built with modern C++.

-----

## 🚀 Overview

This project implements a sophisticated matching engine designed for speed and reliability. It operates on a client-server model, communicating over UNIX sockets to process buy and sell orders. The core of the engine is a powerful order book that matches trades based on a price-time priority algorithm, ensuring fair and efficient execution. The multi-threaded architecture allows for concurrent client connections, making it a scalable solution for real-time trading environments.

-----

## ⭐ Features

  * **High Performance:** Engineered for low-latency order processing and matching.
  * **Price-Time Priority:** Implements the standard algorithm for fair order execution.
  * **Concurrency:** Utilizes a multi-threaded design to handle multiple clients simultaneously.
  * **UNIX Socket API:** Provides a simple and efficient communication interface for clients.
  * **Order Types:** Supports both **Buy** and **Sell** orders.
  * **Robust and Reliable:** Designed for stability in high-throughput scenarios.

-----

## 🛠️ Getting Started

Follow these instructions to get the matching engine up and running on your local machine.

### Prerequisites

Ensure you have the following installed:

  * A C++20 compatible compiler (the project is configured for `clang++`)
  * `make`

### Building the Project

To compile the `engine` and `client` executables, simply run the `make` command from the project's root directory:

```sh
make
```

This will place the `engine` and `client` binaries in the root directory.

### Running the Engine

Start the matching engine by providing a file path for the UNIX socket:

```sh
./engine /tmp/matching_engine.sock
```

The engine is now running and will listen for incoming client connections on the specified socket.

-----

## Usage

You can interact with the engine using the provided `client` application or by directly sending commands to the socket.

### Order Format

The engine accepts simple text-based commands for placing orders:

  * **Buy Order:** `B <order_id> <instrument> <price> <quantity>`
  * **Sell Order:** `S <order_id> <instrument> <price> <quantity>`

### Example

Here is an example of a sequence of orders from `tests/basic-example1.in`:

```
1
o
S 123 GOOG 2700 10
S 124 GOOG 2701 25
B 125 GOOG 2705 30
x
```

This script first places two sell orders for Google stock and then a buy order at a higher price, which would trigger a match.

-----

## 🐳 Docker

For a containerized setup, you can use the provided Docker configuration.

### Build the Docker Image

```sh
docker build -t CS3211/build -f Dockerfile.build .
```

### Run the Build Container

```sh
docker run --name build -v "./:/home/ubuntu/workspace" -it CS3211/build
```

### Remove the Container

```sh
docker rm -f build
```

-----

## License

This project is licensed under the MIT License - see the LICENSE file for details.
