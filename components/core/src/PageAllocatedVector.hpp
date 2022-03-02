#ifndef PAGEALLOCATEDVECTOR_HPP
#define PAGEALLOCATEDVECTOR_HPP

// C standard libraries
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

// C++ standard libraries
#include <cstring>
#include <vector>

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "Defs.h"
#include "ErrorCode.hpp"
#include "TraceableException.hpp"

/**
 * A minimal vector that is allocated in increments of pages rather than individual elements
 * @tparam ValueType The type of value contained in the vector
 */
template <typename ValueType>
class PageAllocatedVector {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "PageAllocatedVector operation failed";
        }
    };

    // Constructors
    /**
     * Constructor
     * @throw PageAllocatedVector::OperationFailed if could not determine page size or if type of value does not fit within a page
     */
    PageAllocatedVector ();

    // Destructor
    ~PageAllocatedVector ();

    // Methods
    /**
     * Pushes all given values to the back of the vector
     * @param values
     * @throw Same as PageAllocatedVector::increase_capacity
     */
    void push_back_all (const std::vector<ValueType>& values);
    /**
     * Pushes the given value to the back of the vector
     * @param value
     * @throw Same as PageAllocatedVector::increase_capacity
     */
    void push_back (const ValueType& value);
    /**
     * Pushes the given value to the back of the vector
     * @param value
     * @throw Same as PageAllocatedVector::increase_capacity
     */
    void push_back (ValueType& value);
    /**
     * Clears the vector
     */
    void clear () noexcept;

    /**
     * Gets underlying array
     * @return Constant pointer to underlying array
     */
    const ValueType* data () const noexcept;
    /**
     * Gets underlying array
     * @return Pointer to underlying array
     */
    ValueType* data () noexcept;

    /**
     * Gets vector's capacity
     * @return Number of values this vector can hold
     */
    size_t capacity () const noexcept;
    /**
     * Gets vector's length
     * @return Number of values in vector
     */
    size_t size () const noexcept;
    /**
     * Gets vector's size in bytes
     * @return Vector's size in bytes
     */
    size_t size_in_bytes () const noexcept;

private:
    // Methods
    /**
     * Increases the vector's capacity to the given value
     * @param required_capacity
     * @throw PageAllocatedVector::OperationFailed if memory allocation fails
     */
    void increase_capacity (size_t required_capacity);

    // Variables
    long m_page_size;

    ValueType* m_values;

    // The capacity of the vector in bytes
    size_t m_capacity_in_bytes;
    // The number of values the vector can contain without reallocation
    size_t m_capacity;
    // The number of values the vector contains
    size_t m_size;
};

template <typename ValueType>
PageAllocatedVector<ValueType>::PageAllocatedVector () : m_values(nullptr), m_capacity_in_bytes(0), m_capacity(0), m_size(0) {
    m_page_size = sysconf(_SC_PAGESIZE);
    if (-1 == m_page_size) {
        throw OperationFailed(ErrorCode::Errno, __FILENAME__, __LINE__);
    }

    if (sizeof(ValueType) > m_page_size) {
        throw OperationFailed(ErrorCode::Unsupported, __FILENAME__, __LINE__);
    }
}

template <typename ValueType>
PageAllocatedVector<ValueType>::~PageAllocatedVector () {
    clear();
}

template <typename ValueType>
void PageAllocatedVector<ValueType>::push_back_all (const std::vector<ValueType>& values) {
    size_t num_new_values = values.size();
    size_t new_size = m_size + num_new_values;
    if (new_size > m_capacity) {
        increase_capacity(new_size);
    }

    memcpy(&m_values[m_size], values.data(), num_new_values*sizeof(ValueType));
    m_size += num_new_values;
}

template <typename ValueType>
void PageAllocatedVector<ValueType>::push_back (const ValueType& value) {
    size_t new_size = m_size + 1;
    if (new_size > m_capacity) {
        increase_capacity(new_size);
    }

    m_values[m_size] = value;
    ++m_size;
}

template <typename ValueType>
void PageAllocatedVector<ValueType>::push_back (ValueType& value) {
    const ValueType& const_value = value;
    push_back(const_value);
}

template <typename ValueType>
void PageAllocatedVector<ValueType>::clear () noexcept {
    if (nullptr != m_values) {
        int retval = munmap(m_values, m_capacity_in_bytes);
        if (0 != retval) {
            SPDLOG_ERROR("PageAllocatedVector::clear munmap failed with errno={}", errno);
            return;
        }

        m_values = nullptr;
        m_capacity_in_bytes = 0;
        m_capacity = 0;
        m_size = 0;
    }
}

template <typename ValueType>
const ValueType* PageAllocatedVector<ValueType>::data () const noexcept {
    return m_values;
}

template <typename ValueType>
ValueType* PageAllocatedVector<ValueType>::data () noexcept {
    return m_values;
}

template <typename ValueType>
size_t PageAllocatedVector<ValueType>::capacity () const noexcept {
    return m_capacity;
}

template <typename ValueType>
size_t PageAllocatedVector<ValueType>::size () const noexcept {
    return m_size;
}

template <typename ValueType>
size_t PageAllocatedVector<ValueType>::size_in_bytes () const noexcept {
    return m_size*sizeof(ValueType);
}

/*
 * To lower the number of calls necessary to increase the vector's capacity, we use a heuristic to grow to max(2*m_capacity, required_capacity)
 */
template <typename ValueType>
void PageAllocatedVector<ValueType>::increase_capacity (size_t required_capacity) {
    if (required_capacity <= m_capacity) {
        return;
    }
    size_t new_size = ROUND_UP_TO_MULTIPLE(std::max(2*m_capacity, required_capacity)*sizeof(ValueType), m_page_size);

    void* new_region;
    if (nullptr == m_values) {
        // NOTE: Regions with the MAP_SHARED flag cannot be remapped for some reason
        new_region = mmap(nullptr, new_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (MAP_FAILED == new_region) {
            throw OperationFailed(ErrorCode::Errno, __FILENAME__, __LINE__);
        }
    } else {
        new_region = mremap(m_values, m_capacity_in_bytes, new_size, MREMAP_MAYMOVE);
        if (MAP_FAILED == new_region) {
            throw OperationFailed(ErrorCode::Errno, __FILENAME__, __LINE__);
        }
    }
    m_values = (ValueType*)new_region;
    m_capacity_in_bytes = new_size;
    m_capacity = m_capacity_in_bytes/sizeof(ValueType);
}

#endif // PAGEALLOCATEDVECTOR_HPP
