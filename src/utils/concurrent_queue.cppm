module;
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

export module utils.concurrent_queue;

export namespace utils
{
template <typename T> class ConcurrentQueue
{
  public:
    explicit ConcurrentQueue(size_t capacity) : m_capacity(capacity)
    {
    }

    void push(T item)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond_full.wait(lock, [this] { return m_queue.size() < m_capacity || m_closed; });
        if (m_closed)
        {
            return;
        }
        m_queue.push(std::move(item));
        m_cond_empty.notify_one();
    }

    std::optional<T> pop()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond_empty.wait(lock, [this] { return !m_queue.empty() || m_closed; });
        if (m_queue.empty())
        {
            return std::nullopt;
        }
        T item = std::move(m_queue.front());
        m_queue.pop();
        m_cond_full.notify_one();
        return item;
    }

    void close()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_closed = true;
        m_cond_empty.notify_all();
        m_cond_full.notify_all();
    }

  private:
    const size_t m_capacity;
    std::queue<T> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond_empty;
    std::condition_variable m_cond_full;
    bool m_closed{false};
};
} // namespace utils
