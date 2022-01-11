//
// Created by haiqixu on 1/6/2022.
//

#ifndef BOOLVECTOR_HPP
#define BOOLVECTOR_HPP

// C++ standard libraries
#include <vector>
#include <unordered_set>

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "Defs.h"
#include "TraceableException.hpp"
#include "streaming_compression/zstd/Compressor.hpp"

// 2GB
#define MAX_CAPACITY 2147483648
#define DEFAULT_CAPACITY 1024

template <typename DictionaryIdType>
class BoolVector {
public:

    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "BoolVector operation failed";
        }
    };

    BoolVector();
    explicit BoolVector(size_t initial_capacity);

    // insert ID as it occurs in the segment
    void insert_id(DictionaryIdType index);

    // number of IDs that occurred in the segment
    size_t num_ids() const { return m_num_element; }

    void reset();

    // insert all ids from another bool vector
    void insert_id_from(const BoolVector<DictionaryIdType>& input);

    // insert all ids from an unordered_set
    void insert_id_from_set(const std::unordered_set<DictionaryIdType>& input);

    void write_to_compressor(streaming_compression::zstd::Compressor& segment_index_compressor) const;
private:
    std::vector<bool> m_data;
    size_t m_num_element;
    size_t m_largest_index;

    // not used yet, but we might want different array to have different default capacity.
    size_t m_initial_capacity;

    // keep increasing the capacity by factor of 2 until the input index is within the capacity range
    void increase_capacity(size_t index);
};

template <typename DictionaryIdType>
BoolVector<DictionaryIdType>::BoolVector(){
    m_initial_capacity = DEFAULT_CAPACITY;
    m_data.resize(m_initial_capacity, false);
    m_num_element = 0;
    m_largest_index = 0;
}

template <typename DictionaryIdType>
BoolVector<DictionaryIdType>::BoolVector(size_t initial_capacity){
    m_initial_capacity = initial_capacity;
    m_data.resize(m_initial_capacity, false);
    m_num_element = 0;
    m_largest_index = 0;
}

template <typename DictionaryIdType>
void BoolVector<DictionaryIdType>::insert_id(DictionaryIdType index) {

    // if index is out of current bound, increase
    if(index >= m_data.size()) {
        increase_capacity(index);
    }
    // if the data is not already inserted
    if(!m_data[index]) {
        m_num_element++;
        m_data[index] = true;
        // update max index if necessary
        if(index > m_largest_index) {
            m_largest_index = index;
        }
    }
}

template <typename DictionaryIdType>
void BoolVector<DictionaryIdType>::reset() {
    m_data.clear();
    m_data.resize(m_initial_capacity, false);
    m_num_element = 0;
    m_largest_index = 0;
}

template <typename DictionaryIdType>
void BoolVector<DictionaryIdType>::increase_capacity(size_t index) {
    if(index < m_data.size()) {
        SPDLOG_ERROR("Calling increase_capacity on IDs smaller than capacity.");
        throw OperationFailed(ErrorCode_Corrupt, __FILENAME__, __LINE__);
    }
    size_t capacity = m_data.size();
    do{
        capacity = capacity * 2;
    } while (capacity <= index);
    if(capacity > MAX_CAPACITY){
        SPDLOG_ERROR("Size out of Bound.");
        throw OperationFailed(ErrorCode_OutOfBounds, __FILENAME__, __LINE__);
    }
    m_data.resize(capacity, false);
}

template <typename DictionaryIdType>
void BoolVector<DictionaryIdType>::insert_id_from(const BoolVector<DictionaryIdType> &input) {
    auto input_data_array = input.m_data;
    size_t input_max_index = input.m_largest_index;
    if(input_max_index >= m_data.size()) {
        increase_capacity(input_max_index);
    }
    for(auto id = 0; id <= input_max_index; id++) {
        if(!m_data[id] && input_data_array[id]) {
            m_data[id] = true;
            m_num_element++;
        }
    }
    if(input_max_index > m_largest_index){
        m_largest_index = input_max_index;
    }
}

template <typename DictionaryIdType>
void BoolVector<DictionaryIdType>::insert_id_from_set(const std::unordered_set<DictionaryIdType>& input){
    for(const DictionaryIdType id : input) {
        insert_id(id);
    }
}

template <typename DictionaryIdType>
void BoolVector<DictionaryIdType>::write_to_compressor(streaming_compression::zstd::Compressor& segment_index_compressor) const {

    for(size_t id = 0; id <= m_largest_index; id++) {
        if(m_data[id]){
            segment_index_compressor.write_numeric_value((DictionaryIdType)id);
        }
    }
}

#endif //BOOLVECTOR_HPP
