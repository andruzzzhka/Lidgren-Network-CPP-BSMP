
//https://gist.github.com/holtgrewe/8728757

#include <condition_variable>
#include <list>

template <typename T>
class ThreadSafeQueue
{
public:
    // Returns whether could push/pop or queue was closed.  Currently, we only implemented the blocking
    // operations.
    enum QueueResult
    {
        OK,
        CLOSED,
        EMPTY,
        FULL
    };

    // Initialize the queue with a maximal size.
    explicit ThreadSafeQueue(size_t maxSize = 0) : state(State::OPEN), currentSize(0), maxSize(maxSize)
    {
    }

    // Push v to queue.  Blocks if queue is full.
    void push(T const& v)
    {
        // Create temporary queue.
        decltype(list) tmpList;
        tmpList.push_back(v);

        // Pushing with lock, only using list<>::splice().
        {
            std::unique_lock<std::mutex> lock(mutex);

            // Wait until there is space in the queue.
            while (currentSize == maxSize && state == State::OPEN)
                cvPush.wait(lock);

            // Check that the queue is not closed and thus pushing is allowed.
            if (state == State::CLOSED)
                throw std::runtime_error("Trying to push to a closed queue.");

            // Push to queue.
            currentSize += 1;
            list.splice(list.end(), tmpList, tmpList.begin());

            // Wake up one popping thread.
            if (currentSize == 1u)
                cvPop.notify_one();
        }
    }

    // Push to queue with rvalue reference.
    void push(T&& v)
    {
        // Create temporary queue.
        decltype(list) tmpList;
        tmpList.push_back(v);

        // Pushing with lock, only using list<>::splice().
        {
            std::unique_lock<std::mutex> lock(mutex);

            // Wait until there is space in the queue.
            while (currentSize == maxSize && state == State::OPEN)
                cvPush.wait(lock);

            // Check that the queue is not closed and thus pushing is allowed.
            if (state == State::CLOSED)
                throw std::runtime_error("Trying to push to a closed queue.");

            // Push to queue.
            currentSize += 1;
            list.splice(list.end(), tmpList, tmpList.begin());

            // Wake up one popping thread.
            cvPop.notify_one();
        }
    }

    // Pop value from queue and write to v.
    //
    // If this succeeds, OK is returned.  CLOSED is returned if the queue is empty and was closed.
    QueueResult pop(T& v, bool blocking)
    {
        decltype(list) tmpList;

        // Pop into tmpList which is finally written out.
        {
            std::unique_lock<std::mutex> lock(mutex);

            // If there is no item then we wait until there is one.
            if (blocking)
            {

                while (list.empty() && state != State::CLOSED)
                    cvPop.wait(lock);
            }
            else
            {
                if (list.empty() && state != State::CLOSED) //My attempts on making this non-blocking
                    return EMPTY;
            }

            // If the queue was closed and the list is empty then we cannot return anything.
            if (list.empty() && state == State::CLOSED)
                return CLOSED;

            // If we reach here then there is an item, get it.
            currentSize -= 1;
            tmpList.splice(tmpList.begin(), list, list.begin());
            // Wake up one pushing thread.
            cvPush.notify_one();
        }

        // Write out value.
        v = tmpList.front();

        return OK;
    }

    // Pushing data is not allowed any more, when the queue is 
    void close() noexcept
    {
        std::unique_lock<std::mutex> lock(mutex);
        state = State::CLOSED;

        // Notify all consumers.
        cvPop.notify_all();
    }

private:

    // Whether the queue is running or closed.
    enum class State
    {
        OPEN,
        CLOSED
    };

    // The current state.
    State state;
    // The current size.
    size_t currentSize;
    // The maximal size.
    size_t maxSize;
    // The condition variables to use for pushing/popping.
    std::condition_variable cvPush, cvPop;
    // The mutex for locking the queue.
    std::mutex mutex;
    // The list that the queue is implemented with.
    std::list<T> list;
};