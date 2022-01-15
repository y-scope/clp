//
// Created by haiqixu on 1/6/2022.
//

#ifndef IDOCCURRENCEARRAY_HPP
#define IDOCCURRENCEARRAY_HPP

// C++ standard libraries
#include <vector>
#include <unordered_set>

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "Defs.h"
#include "streaming_compression/zstd/Compressor.hpp"
#include "TraceableException.hpp"

// Constant
#define MAX_CAPACITY 2147483648
#define DEFAULT_CAPACITY 1024

template <typename DictionaryIdType>
class IDOccurrenceArray {
public:

    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "IDOccurrenceArray operation failed";
        }
    };

    // Constructors
    IDOccurrenceArray();
    explicit IDOccurrenceArray(size_t initial_capacity);

    // Destructors
    ~IDOccurrenceArray() = default;

    // Methods
    /**
     * insert the id into the data occurrence array
     * @param id
     */
    void insert_id(DictionaryIdType id);

    /**
     * Get the number of different ids in the array
     * @return number of ids in the array
     */
    size_t num_ids() const { return m_num_ids; }

    /**
     * Clear the id occurrence info and reset
     * the array back to default initial capacity
     */
    void reset();

    /**
     * Insert all IDs from another IDOccurrenceArray
     * into the caller IDOccurrenceArray object
     * @param ids_occurrence_array
     */
    void insert_id_from_array(const IDOccurrenceArray<DictionaryIdType>& ids_occurrence_array);

    /**
     * Insert all IDs from an unordered_set
     * into the caller IDOccurrenceArray object
     * @param ids_set
     */
    void insert_id_from_set(const std::unordered_set<DictionaryIdType>& ids_set);

    /**
     * write all IDs in the array into the segment index compressor
     * @param segment_index_compressor
     */
    void write_to_compressor(streaming_compression::zstd::Compressor& segment_index_compressor) const;

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
     * Throws an error if the input id is already in the
     * range of the array
     * @param id
     */
    void increase_capacity(size_t id);
};

template <typename DictionaryIdType>
IDOccurrenceArray<DictionaryIdType>::IDOccurrenceArray(){
    m_initial_capacity = DEFAULT_CAPACITY;
    reset();
}

template <typename DictionaryIdType>
IDOccurrenceArray<DictionaryIdType>::IDOccurrenceArray(size_t initial_capacity){
    m_initial_capacity = initial_capacity;
    reset();
}

template <typename DictionaryIdType>
void IDOccurrenceArray<DictionaryIdType>::reset() {
    m_data.clear();
    m_data.resize(m_initial_capacity, false);
    m_num_ids = 0;
    m_largest_id = 0;
}

template <typename DictionaryIdType>
void IDOccurrenceArray<DictionaryIdType>::insert_id(DictionaryIdType id) {

    if(id >= m_data.size()) {
        increase_capacity(id);
    }
    // if the id is not already marked as "occurred"
    if(!m_data[id]) {
        m_data[id] = true;
        m_num_ids++;
        // update largest id if necessary
        if(id > m_largest_id) {
            m_largest_id = id;
        }
    }
}

template <typename DictionaryIdType>
void IDOccurrenceArray<DictionaryIdType>::increase_capacity(size_t id) {
    if(id < m_data.size()) {
        SPDLOG_ERROR("Calling increase_capacity on IDs smaller than capacity.");
        throw OperationFailed(ErrorCode_Corrupt, __FILENAME__, __LINE__);
    }
    size_t capacity = m_data.size();
    do{
        capacity = capacity * 2;
    } while (capacity <= id);

    if(capacity > MAX_CAPACITY){
        SPDLOG_WARN("size of array {} is more than {} bytes.", capacity, MAX_CAPACITY);
    }
    m_data.resize(capacity, false);
}

template <typename DictionaryIdType>
void IDOccurrenceArray<DictionaryIdType>::insert_id_from_array(const IDOccurrenceArray<DictionaryIdType> &ids_occurrence_array) {

    auto input_data_array = ids_occurrence_array.m_data;
    size_t input_max_id = ids_occurrence_array.m_largest_id;
    // increase size of the boolean array if needed
    if(input_max_id >= m_data.size()) {
        increase_capacity(input_max_id);
    }
    for(auto id = 0; id <= input_max_id; id++) {
        // if an ID
        // 1. doesn't occur in the caller array
        // 2. occurs in the input array
        // Adds it to the caller's array
        if(!m_data[id] && input_data_array[id]) {
            m_data[id] = true;
            m_num_ids++;
        }
    }
    if(input_max_id > m_largest_id){
        m_largest_id = input_max_id;
    }
}

template <typename DictionaryIdType>
void IDOccurrenceArray<DictionaryIdType>::insert_id_from_set(const std::unordered_set<DictionaryIdType>& ids_set){
    for(const DictionaryIdType id : ids_set) {
        insert_id(id);
    }
}

template <typename DictionaryIdType>
void IDOccurrenceArray<DictionaryIdType>::write_to_compressor(streaming_compression::zstd::Compressor& segment_index_compressor) const {

    for(size_t id = 0; id <= m_largest_id; id++) {
        if(m_data[id]){
            segment_index_compressor.write_numeric_value((DictionaryIdType)id);
        }
    }
}

#endif //IDOCCURRENCEARRAY_HPP
