// ring_buffer_rpc/rb_message_queue.cpp

#include "ring_buffer_rpc/rb_message_queue.h"
#include <cstdint>
#include <cstring>
#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <iostream>

// ------------------------------------------------------------------------------------------------------------
// 1) Internal single‐producer/single‐consumer RingBuffer class WITH blocking dequeue
// ------------------------------------------------------------------------------------------------------------
class RingBuffer {
public:
    // capacity must be >= 5 (4‐byte header + at least 1 payload byte + 1 unused)
    explicit RingBuffer(size_t capacity)
      : _capacity(capacity),
        _buffer(new uint8_t[capacity]),
        _head(0),
        _tail(0)
    {
        if (capacity < sizeof(uint32_t) + 1) {
            throw std::invalid_argument("Capacity too small for ring buffer");
        }
    }

    ~RingBuffer() {
        delete[] _buffer;
    }

    // Try to enqueue a message of length `length` (payload only) from `data[]`.
    // Returns true on success, false if there isn't enough free space right now.
    // If successful, notifies any waiting consumer.
    bool enqueue(const uint8_t* data, uint32_t length) {
        // 4 bytes for the length header + payload
        uint32_t total = uint32_t(sizeof(uint32_t)) + length;
        if (total > _capacity - 1) {
            // The message is too large to ever fit (we always leave 1 byte unused)
            return false;
        }

        std::unique_lock<std::mutex> lock(_mutex);

        uint32_t free_space = calculate_free_space_locked();
        if (free_space < total) {
            // Not enough room right now
            return false;
        }

        // Build a 4‐byte little‐endian header
        uint8_t hdr[4];
        hdr[0] = uint8_t((length >>  0) & 0xFF);
        hdr[1] = uint8_t((length >>  8) & 0xFF);
        hdr[2] = uint8_t((length >> 16) & 0xFF);
        hdr[3] = uint8_t((length >> 24) & 0xFF);

        // 1) Copy the 4‐byte header at index _head (wrapping if needed)
        _head = copy_into_ring(_head, hdr, 4);

        // 2) Copy the payload bytes immediately after
        _head = copy_into_ring(_head, data, length);

        // Notify any waiting consumer that there is now at least one message
        lock.unlock();
        _cv.notify_one();
        return true;
    }

    // BLOCKING: waits until at least one full message is available, then
    // copies up to `max_length` bytes of payload into dest[] and returns
    // the true message length. This call only returns when there is a message.
    uint32_t dequeue(uint8_t* dest, uint32_t max_length) {
        std::unique_lock<std::mutex> lock(_mutex);

        // Wait until there is at least a 4‐byte header (i.e. head != tail)
        _cv.wait(lock, [&]() {
            return (_head != _tail);
        });

        // At this point, there is at least the 4‐byte length header to read.
        // 1) Read 4‐byte header at _tail
        uint8_t hdr[4];
        uint32_t next = copy_from_ring(_tail, hdr, 4);
        uint32_t msg_len =
             (uint32_t)hdr[0]
           | ((uint32_t)hdr[1] <<  8)
           | ((uint32_t)hdr[2] << 16)
           | ((uint32_t)hdr[3] << 24);

        // Sanity check: message length must be <= capacity-4
        if (msg_len > _capacity - sizeof(uint32_t)) {
            throw std::runtime_error("RingBuffer::dequeue: invalid message length");
        }

        // Ensure the entire payload is present: if not, wait until it is.
        uint32_t total_size = sizeof(uint32_t) + msg_len;
        _cv.wait(lock, [&]() {
            return calculate_used_space_locked() >= total_size;
        });

        // 2) Copy payload bytes (up to max_length) into dest[]
        uint32_t to_copy = (msg_len <= max_length ? msg_len : max_length);
        next = copy_from_ring(next, dest, to_copy);

        // 3) Advance past remainder of payload if truncated
        uint32_t skip = msg_len - to_copy;
        if (skip > 0) {
            if (next + skip < _capacity) {
                next += skip;
            } else {
                next = (next + skip) - _capacity;
            }
        }

        // Commit the new tail
        _tail = next;

        // Return the actual payload length (caller sees this, even if truncated)
        return msg_len;
    }

private:
    // CALLED WITH _mutex HELD
    uint32_t calculate_free_space_locked() const {
        if (_head >= _tail) {
            // used = head - tail; free = capacity - used - 1
            return uint32_t(_capacity - (_head - _tail) - 1);
        } else {
            // head < tail: used = capacity - (tail - head); free = (tail - head) - 1
            return uint32_t((_tail - _head) - 1);
        }
    }

    // CALLED WITH _mutex HELD
    uint32_t calculate_used_space_locked() const {
        if (_head >= _tail) {
            return uint32_t(_head - _tail);
        } else {
            return uint32_t(_capacity - (_tail - _head));
        }
    }

    // Copy `n` bytes from `src` into `_buffer` starting at `idx`, wrapping around if needed.
    // Returns the new index (modulo _capacity).
    uint32_t copy_into_ring(uint32_t idx, const uint8_t* src, uint32_t n) {
        if (idx + n <= _capacity) {
            // fits contiguously
            std::memcpy(&_buffer[idx], src, n);
            idx += n;
            if (idx == _capacity) idx = 0;
            return idx;
        } else {
            // split into two pieces
            uint32_t first_part = uint32_t(_capacity - idx);
            std::memcpy(&_buffer[idx], src, first_part);
            uint32_t second_part = n - first_part;
            std::memcpy(&_buffer[0], src + first_part, second_part);
            return second_part;
        }
    }

    // Copy `n` bytes from `_buffer` at index `idx` into `dst`, wrapping around if needed.
    // Returns the new index (modulo _capacity).
    uint32_t copy_from_ring(uint32_t idx, uint8_t* dst, uint32_t n) const {
        if (idx + n <= _capacity) {
            std::memcpy(dst, &_buffer[idx], n);
            idx += n;
            if (idx == _capacity) idx = 0;
            return idx;
        } else {
            uint32_t first_part = uint32_t(_capacity - idx);
            std::memcpy(dst, &_buffer[idx], first_part);
            uint32_t second_part = n - first_part;
            std::memcpy(dst + first_part, &_buffer[0], second_part);
            return second_part;
        }
    }

    const size_t _capacity;
    uint8_t*    _buffer;
    uint32_t    _head;    // next write index
    uint32_t    _tail;    // next read index

    // For blocking behavior:
    std::mutex              _mutex;
    std::condition_variable _cv;
};

// ------------------------------------------------------------------------------------------------------------
// 2) Two global ring buffers (requests & responses)
// ------------------------------------------------------------------------------------------------------------
static constexpr size_t REQUEST_BUFFER_CAPACITY  = 1 * 1024 * 1024; // 1 MiB
static constexpr size_t RESPONSE_BUFFER_CAPACITY = 1 * 1024 * 1024; // 1 MiB

static RingBuffer g_request_rb (REQUEST_BUFFER_CAPACITY);
static RingBuffer g_response_rb(RESPONSE_BUFFER_CAPACITY);

// ------------------------------------------------------------------------------------------------------------
// 3) Public API (as declared in rb_message_queue.h)
// ------------------------------------------------------------------------------------------------------------

// Enqueue a request. If it doesn't fit, print an error.
void queue_message(const uint8_t* data, uint32_t length) {
    // Check “never‐fit” condition first
    if (uint64_t(length) + sizeof(uint32_t) > REQUEST_BUFFER_CAPACITY - 1) {
        std::cerr << "[RingBuffer] queue_message: message too large ("
                  << length << " bytes)\n";
        return;
    }
    bool ok = g_request_rb.enqueue(data, length);
    if (!ok) {
        std::cerr << "[RingBuffer] queue_message: not enough space for "
                  << length << "‐byte message\n";
    }
}

// Dequeue exactly one request. BLOCKS until a complete message is available.
// Returns the actual payload length (caller can compare against max_length
// to see if truncation occurred, but in normal operation payload ≤ max_length).
uint32_t dequeue_message(uint8_t* dest, uint32_t max_length) {
    return g_request_rb.dequeue(dest, max_length);
}

// Enqueue a response. If it doesn't fit, print an error.
void queue_response(const uint8_t* data, uint32_t length) {
    if (uint64_t(length) + sizeof(uint32_t) > RESPONSE_BUFFER_CAPACITY - 1) {
        std::cerr << "[RingBuffer] queue_response: message too large ("
                  << length << " bytes)\n";
        return;
    }
    bool ok = g_response_rb.enqueue(data, length);
    if (!ok) {
        std::cerr << "[RingBuffer] queue_response: not enough space for "
                  << length << "‐byte message\n";
    }
}

// Dequeue exactly one response. BLOCKS until a complete message is available.
// Returns the actual payload length.
uint32_t dequeue_response(uint8_t* dest, uint32_t max_length) {
    return g_response_rb.dequeue(dest, max_length);
}
