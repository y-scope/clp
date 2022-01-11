//
// Created by haiqixu on 1/6/2022.
//

#include "BoolVector.hpp"
// 2GB
#define HALF_MAX_CAPACITY 2147483648
#define DEFAULT_CAPACITY 1024

BoolVector::BoolVector(){
    m_initial_capacity = DEFAULT_CAPACITY;
    m_data.resize(m_initial_capacity, false);
    m_num_element = 0;
    m_largest_index = 0;
}

BoolVector::BoolVector(size_t initial_capacity){
    m_initial_capacity = initial_capacity;
    m_data.resize(m_initial_capacity, false);
    m_num_element = 0;
    m_largest_index = 0;
}

void BoolVector::insert_id(size_t index) {

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

void BoolVector::reset() {
    m_data.clear();
    m_data.resize(m_initial_capacity, false);
    m_num_element = 0;
    m_largest_index = 0;
}

void BoolVector::increase_capacity(size_t index) {
    if(index < m_data.size()) {
        SPDLOG_ERROR("Calling increase_capacity on IDs smaller than capacity.");
        throw OperationFailed(ErrorCode_Corrupt, __FILENAME__, __LINE__);
    }
    size_t capacity = m_data.size();
    do{
        capacity = capacity * 2;
    } while (capacity <= index);
    if(capacity > HALF_MAX_CAPACITY){
        SPDLOG_ERROR("Size out of Bound.");
        throw OperationFailed(ErrorCode_OutOfBounds, __FILENAME__, __LINE__);
    }
    m_data.resize(capacity, false);
}

void BoolVector::insert_id_from(const BoolVector &input) {
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