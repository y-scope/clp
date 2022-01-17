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

// Constant
#define DEFAULT_CAPACITY 1024
#define MAX_CAPACITY 2147483648

template<typename DictionaryIdType>
class ArrayBackedPosIntSet {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException(error_code, filename, line_number) {}
        // Methods
        const char* what () const
        noexcept override {
                return "ArrayBackedPosIntSet operation failed";
        }
    };
    // Constructors
    ArrayBackedPosIntSet ();

    explicit ArrayBackedPosIntSet (size_t initial_capacity);

    // Destructors
    ~ArrayBackedPosIntSet () = default;

    // Methods
    /**
     * Inserts the id into the data occurrence array
     * @param id
     */
    void insert_id (DictionaryIdType id);

    /**
     * Gets the number of different ids in the array
     * @return number of ids in the array
     */
    size_t num_ids () const { return m_num_ids; }

    /**
     * Clears the id occurrence info and reset
     * the array back to default initial capacity
     */
    void reset ();

    /**
     * Inserts all IDs from the given ArrayBackedPosIntSet
     * into the caller ArrayBackedPosIntSet object
     * @param array_backed_int_set
     */
    void insert_id_from_array (const ArrayBackedPosIntSet<DictionaryIdType>& array_backed_int_set);

    /**
     * Inserts all IDs from an unordered_set
     * into the caller ArrayBackedPosIntSet object
     * @param ids_set
     */
    void insert_id_from_set (const std::unordered_set <DictionaryIdType>& ids_set);

    /**
     * Writes all IDs in the array into the segment index compressor
     * @param segment_index_compressor
     */
    void write_to_compressor (streaming_compression::zstd::Compressor& segment_index_compressor) const;

private:

    std::vector<bool> m_data;
    size_t m_initial_capacity;

    // Tracks number of IDs that occurred in the array
    size_t m_num_ids;

    // Tracks the largest ID in the array
    size_t m_largest_id;

    /**
     * increase the capacity of the bool array so that
     * input id becomes an valid index of the array.
     * @throw IDOccurrenceArray::OperationFailed if the input id is already within the bool array's index range
     * @param id
     */
    void increase_capacity (size_t id);
};

template<typename DictionaryIdType>
ArrayBackedPosIntSet<DictionaryIdType>::ArrayBackedPosIntSet () {
    m_initial_capacity = DEFAULT_CAPACITY;
    reset();
}

template<typename DictionaryIdType>
ArrayBackedPosIntSet<DictionaryIdType>::ArrayBackedPosIntSet (size_t initial_capacity) {
    m_initial_capacity = initial_capacity;
    reset();
}

template<typename DictionaryIdType>
void ArrayBackedPosIntSet<DictionaryIdType>::reset () {
    m_data.clear();
    m_data.resize(m_initial_capacity, false);
    m_num_ids = 0;
    m_largest_id = 0;
}

template<typename DictionaryIdType>
void ArrayBackedPosIntSet<DictionaryIdType>::insert_id (DictionaryIdType id) {

    if (id >= m_data.size()) {
        increase_capacity(id);
    }
    // if the id is not already marked as "occurred"
    if (!m_data[id]) {
        m_data[id] = true;
        m_num_ids++;
        // update the largest id if necessary
        if (id > m_largest_id) {
            m_largest_id = id;
        }
    }
}

template<typename DictionaryIdType>
void ArrayBackedPosIntSet<DictionaryIdType>::increase_capacity (size_t id) {
    if (id < m_data.size()) {
        SPDLOG_ERROR("Calling increase_capacity on IDs smaller than capacity.");
        throw OperationFailed(ErrorCode_Corrupt, __FILENAME__, __LINE__);
    }
    size_t capacity = m_data.size();
    do {
        capacity = capacity * 2;
    } while (capacity <= id);

    if (capacity > MAX_CAPACITY) {
        SPDLOG_WARN("size of array {} is more than {} bytes.", capacity, MAX_CAPACITY);
    }
    m_data.resize(capacity, false);
}

template<typename DictionaryIdType>
void ArrayBackedPosIntSet<DictionaryIdType>::insert_id_from_array (const ArrayBackedPosIntSet<DictionaryIdType>& array_backed_int_set) {

    auto input_data_array = array_backed_int_set.m_data;
    size_t input_max_id = array_backed_int_set.m_largest_id;
    // increase size of the boolean array if needed
    if (input_max_id >= m_data.size()) {
        increase_capacity(input_max_id);
    }
    for (auto id = 0; id <= input_max_id; id++) {
        // if an ID
        // 1. doesn't occur in the caller array
        // 2. occurs in the input array
        // Adds it to the caller's array
        if (!m_data[id] && input_data_array[id]) {
            m_data[id] = true;
            m_num_ids++;
        }
    }
    if (input_max_id > m_largest_id) {
        m_largest_id = input_max_id;
    }
}

template<typename DictionaryIdType>
void ArrayBackedPosIntSet<DictionaryIdType>::insert_id_from_set (const std::unordered_set <DictionaryIdType>& ids_set) {
    for (const DictionaryIdType id: ids_set) {
        insert_id(id);
    }
}

template<typename DictionaryIdType>
void ArrayBackedPosIntSet<DictionaryIdType>::write_to_compressor (streaming_compression::zstd::Compressor& segment_index_compressor) const {

    for (size_t id = 0; id <= m_largest_id; id++) {
        if (m_data[id]) {
            segment_index_compressor.write_numeric_value((DictionaryIdType) id);
        }
    }
}

#endif //ARRAYBACKEDPOSINTSET_HPP
