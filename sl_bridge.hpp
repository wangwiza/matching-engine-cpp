#include <condition_variable>
#include <mutex>

class sl_bridge {
  std::mutex mtx_;
  std::condition_variable cv_;
  long current_dir_ = 0; // 0=empty, +ve=SELL, -ve=BUY

public:
  sl_bridge() = default;

  void enter_buy() {
    std::unique_lock<std::mutex> lock(mtx_);
    // Wait until bridge is empty or already in our direction
    cv_.wait(lock, [this]() { return current_dir_ <= 0; });
    current_dir_--;
  }

  void exit_buy() {
    std::unique_lock<std::mutex> lock(mtx_);
    current_dir_++;
    if (current_dir_ == 0) {
      cv_.notify_all(); // Wake all waiting threads
    }
  }

  void enter_sell() {
    std::unique_lock<std::mutex> lock(mtx_);
    cv_.wait(lock, [this]() { return current_dir_ >= 0; });
    current_dir_++;
  }

  void exit_sell() {
    std::unique_lock<std::mutex> lock(mtx_);
    current_dir_--;
    if (current_dir_ == 0) {
      cv_.notify_all();
    }
  }
};
