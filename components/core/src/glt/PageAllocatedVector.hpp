#ifndef GLT_PAGEALLOCATEDVECTOR_HPP
#define GLT_PAGEALLOCATEDVECTOR_HPP

#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstring>
#include <vector>

#include "Defs.h"
#include "ErrorCode.hpp"
#include "Platform.hpp"
#include "spdlog_with_specializations.hpp"
#include "TraceableException.hpp"

// Define a MREMAP_MAYMOVE shim for compilation (just compilation) on macOS
#if defined(__APPLE__) || defined(__MACH__)
    #define MREMAP_MAYMOVE 0
#endif

namespace glt {
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
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "PageAllocatedVector operation failed";
        }
    };

    // Constructors
    /**
     * Constructor
     * @throw PageAllocatedVector::OperationFailed if could not determine page size or if type of
     * value does not fit within a page
     */
    PageAllocatedVector();

    // Destructor
    ~PageAllocatedVector();

    // Methods
    /**
     * Pushes all given values to the back of the vector
     * @param values
     * @throw Same as PageAllocatedVector::increase_capacity
     */
    void push_back_all(std::vector<ValueType> const& values);
    /**
     * Pushes the given value to the back of the vector
     * @param value
     * @throw Same as PageAllocatedVector::increase_capacity
     */
    void push_back(ValueType const& value);
    /**
     * Pushes the given value to the back of the vector
     * @param value
     * @throw Same as PageAllocatedVector::increase_capacity
     */
    void push_back(ValueType& value);
    /**
     * Clears the vector
     */
    void clear() noexcept;

    /**
     * Gets underlying array
     * @return Constant pointer to underlying array
     */
    ValueType const* data() const noexcept;
    /**
     * Gets underlying array
     * @return Pointer to underlying array
     */
    ValueType* data() noexcept;

    /**
     * Gets vector's capacity
     * @return Number of values this vector can hold
     */
    size_t capacity() const noexcept;
    /**
     * Gets vector's length
     * @return Number of values in vector
     */
    size_t size() const noexcept;
    /**
     * Gets vector's size in bytes
     * @return Vector's size in bytes
     */
    size_t size_in_bytes() const noexcept;

private:
    // Methods
    /**
     * Memory maps a new readable/writeable anonymous region with the given size
     * @param new_size
     * @return A pointer to the new region
     */
    static void* map_new_region(size_t new_size);
    /**
     * Unmaps the existing region
     */
    static void unmap_region(void* region, size_t region_size);

    /**
     * Increases the vector's capacity to the given value
     * @param required_capacity
     * @throw PageAllocatedVector::OperationFailed if memory allocation fails
     */
    void increase_capacity(size_t required_capacity);

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
PageAllocatedVector<ValueType>::PageAllocatedVector()
        : m_values(nullptr),
          m_capacity_in_bytes(0),
          m_capacity(0),
          m_size(0) {
    m_page_size = sysconf(_SC_PAGESIZE);
    if (-1 == m_page_size) {
        throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
    }

    if (sizeof(ValueType) > m_page_size) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
}

template <typename ValueType>
PageAllocatedVector<ValueType>::~PageAllocatedVector() {
    clear();
}

template <typename ValueType>
void PageAllocatedVector<ValueType>::push_back_all(std::vector<ValueType> const& values) {
    size_t num_new_values = values.size();
    size_t new_size = m_size + num_new_values;
    if (new_size > m_capacity) {
        increase_capacity(new_size);
    }

    std::copy(values.data(), values.data() + num_new_values, &m_values[m_size]);
    m_size += num_new_values;
}

template <typename ValueType>
void PageAllocatedVector<ValueType>::push_back(ValueType const& value) {
    size_t new_size = m_size + 1;
    if (new_size > m_capacity) {
        increase_capacity(new_size);
    }

    m_values[m_size] = value;
    ++m_size;
}

template <typename ValueType>
void PageAllocatedVector<ValueType>::push_back(ValueType& value) {
    ValueType const& const_value = value;
    push_back(const_value);
}

template <typename ValueType>
void PageAllocatedVector<ValueType>::clear() noexcept {
    unmap_region(m_values, m_capacity_in_bytes);
    m_capacity_in_bytes = 0;
    m_capacity = 0;
    m_size = 0;
}

template <typename ValueType>
ValueType const* PageAllocatedVector<ValueType>::data() const noexcept {
    return m_values;
}

template <typename ValueType>
ValueType* PageAllocatedVector<ValueType>::data() noexcept {
    return m_values;
}

template <typename ValueType>
size_t PageAllocatedVector<ValueType>::capacity() const noexcept {
    return m_capacity;
}

template <typename ValueType>
size_t PageAllocatedVector<ValueType>::size() const noexcept {
    return m_size;
}

template <typename ValueType>
size_t PageAllocatedVector<ValueType>::size_in_bytes() const noexcept {
    return m_size * sizeof(ValueType);
}

template <typename ValueType>
void* PageAllocatedVector<ValueType>::map_new_region(size_t new_size) {
    // NOTE: Regions with the MAP_SHARED flag cannot be remapped for some reason
    void* new_region
            = mmap(nullptr, new_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (MAP_FAILED == new_region) {
        throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
    }
    return new_region;
}

template <typename ValueType>
void PageAllocatedVector<ValueType>::unmap_region(void* region, size_t region_size) {
    if (nullptr == region) {
        return;
    }

    int retval = munmap(region, region_size);
    if (0 != retval) {
        throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
    }
}

/*
 * To lower the number of calls necessary to increase the vector's capacity, we use a heuristic to
 * grow to max(2*m_capacity, required_capacity)
 */
template <typename ValueType>
void PageAllocatedVector<ValueType>::increase_capacity(size_t required_capacity) {
    if (required_capacity <= m_capacity) {
        return;
    }
    size_t new_size = ROUND_UP_TO_MULTIPLE(
            std::max(2 * m_capacity, required_capacity) * sizeof(ValueType),
            m_page_size
    );

    void* new_region;
    if (nullptr == m_values) {
        new_region = static_cast<ValueType*>(map_new_region(new_size));
    } else {
        if constexpr (Platform::MacOs == cCurrentPlatform) {
            // macOS doesn't support mremap, so we need to map a new region, copy the contents of
            // the old region, and then unmap the old region.
            new_region = map_new_region(new_size);
            std::copy(m_values, m_values + m_capacity, static_cast<ValueType*>(new_region));

            try {
                unmap_region(m_values, m_capacity_in_bytes);
            } catch (OperationFailed const& e) {
                // Unmap the new region so we don't leak it
                unmap_region(new_region, new_size);
                throw e;
            }
        } else {
            new_region = mremap(m_values, m_capacity_in_bytes, new_size, MREMAP_MAYMOVE);
            if (MAP_FAILED == new_region) {
                throw OperationFailed(ErrorCode_errno, __FILENAME__, __LINE__);
            }
        }
    }
    m_values = static_cast<ValueType*>(new_region);
    m_capacity_in_bytes = new_size;
    m_capacity = m_capacity_in_bytes / sizeof(ValueType);
}
}  // namespace glt

#endif  // GLT_PAGEALLOCATEDVECTOR_HPP
