#ifndef ARRAYBACKEDPOSINTSET_HPP
#define ARRAYBACKEDPOSINTSET_HPP

// C++ standard libraries
#include <unordered_set>
#include <vector>

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "Defs.h"
#include "streaming_compression/zstd/Compressor.hpp"
#include "TraceableException.hpp"

/**
 * Template class of set implemented with vector<bool> for continuously increasing numeric value
 * @tparam PosIntType
 */
template<typename PosIntType>
class ArrayBackedPosIntSet {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException(error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "ArrayBackedPosIntSet operation failed";
        }
    };

    // Constructors
    ArrayBackedPosIntSet ();

    explicit ArrayBackedPosIntSet (size_t initial_capacity);

    // Methods
    /**
     * Gets the number of unique values in the set
     */
    size_t size () const { return m_size; }

    /**
     * Clears the set and restores its initial capacity
     */
    void clear ();

    void insert (PosIntType value);

    /**
     * Inserts all values from the given set
     * @param input_set
     */
    void insert_all (const ArrayBackedPosIntSet<PosIntType>& input_set);

    /**
     * Inserts all values from the given set
     * @param input_set
     */
    void insert_all (const std::unordered_set<PosIntType>& input_set);

    /**
     * Inserts all values from the given vector
     * @param input_vector
     */
    void insert_all (const std::vector<PosIntType>& input_vector);

    /**
     * Writes all values in the set into the given compressor
     * @param compressor
     */
    void write_to_compressor (streaming_compression::Compressor& compressor) const;

private:
    // Methods
    /**
     * Increases the capacity of the bool array so that
     * the given value becomes a valid index in the array
     * @param value
     */
    void increase_capacity (size_t value);

    // Variables
    std::vector<bool> m_data;
    size_t m_initial_capacity;

    // The number of unique values in the set
    size_t m_size;

    // The largest value in the set
    PosIntType m_largest_value;
};

template<typename PosIntType>
ArrayBackedPosIntSet<PosIntType>::ArrayBackedPosIntSet () {
    constexpr size_t cDefaultInitialCapacity = 1024;
    m_initial_capacity = cDefaultInitialCapacity;
    clear();
}

template<typename PosIntType>
ArrayBackedPosIntSet<PosIntType>::ArrayBackedPosIntSet (size_t initial_capacity) {
    m_initial_capacity = initial_capacity;
    clear();
}

template<typename PosIntType>
void ArrayBackedPosIntSet<PosIntType>::clear () {
    m_data.clear();
    m_data.resize(m_initial_capacity, false);
    m_size = 0;
    m_largest_value = 0;
}

template<typename PosIntType>
void ArrayBackedPosIntSet<PosIntType>::insert (PosIntType value) {
    if (value >= m_data.size()) {
        increase_capacity(value);
    }

    // Add the value if it is not already in the set
    if (false == m_data[value]) {
        m_data[value] = true;
        m_size++;

        // Update the largest value if necessary
        if (value > m_largest_value) {
            m_largest_value = value;
        }
    }
}

template<typename PosIntType>
void ArrayBackedPosIntSet<PosIntType>::insert_all (const ArrayBackedPosIntSet<PosIntType>& input_set) {
    // Increase capacity if necessary
    size_t input_set_largest_value = input_set.m_largest_value;
    if (input_set_largest_value >= m_data.size()) {
        increase_capacity(input_set_largest_value);
    }

    // Copy values from the input set
    auto input_set_data = input_set.m_data;
    for (auto value = 0; value <= input_set_largest_value; ++value) {
        // Add a value only if
        // - doesn't exist in this set
        // - exists in the input set
        if (false == m_data[value] && input_set_data[value]) {
            m_data[value] = true;
            m_size++;
        }
    }

    // Update the largest value if necessary
    if (input_set_largest_value > m_largest_value) {
        m_largest_value = input_set_largest_value;
    }
}

template<typename PosIntType>
void ArrayBackedPosIntSet<PosIntType>::insert_all (const std::unordered_set<PosIntType>& input_set) {
    for (const auto value : input_set) {
        insert(value);
    }
}

template<typename PosIntType>
void ArrayBackedPosIntSet<PosIntType>::insert_all (const std::vector<PosIntType>& input_vector) {
    for (const auto value : input_vector) {
        insert(value);
    }
}

template<typename PosIntType>
void ArrayBackedPosIntSet<PosIntType>::write_to_compressor (streaming_compression::Compressor& compressor) const {
    for (PosIntType value = 0; value <= m_largest_value; ++value) {
        if (m_data[value]) {
            compressor.write_numeric_value(value);
        }
    }
}

template<typename PosIntType>
void ArrayBackedPosIntSet<PosIntType>::increase_capacity (size_t value) {
    if (value < m_data.size()) {
        SPDLOG_ERROR("Calling increase_capacity on value smaller than capacity.");
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    auto capacity = m_data.size();
    do {
        capacity += capacity / 2;
    } while (capacity <= value);

    m_data.resize(capacity, false);
}

#endif //ARRAYBACKEDPOSINTSET_HPP
