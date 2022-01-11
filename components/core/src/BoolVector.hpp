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
    void insert_id(size_t index);

    // number of IDs that occurred in the segment
    size_t num_ids() const { return m_num_element; }

    // getter
    const std::vector<bool>& get_data() const { return m_data; };

    // the largest ID in the segment

    size_t max_id() const { return m_largest_index; };
    void reset();

    // insert all ids from another bool vector
    void insert_id_from(const BoolVector& input);

private:
    std::vector<bool> m_data;
    size_t m_num_element;
    size_t m_largest_index;

    // not used yet, but we might want different array to have different default capacity.
    size_t m_initial_capacity;

    // keep increasing the capacity by factor of 2 until the input index is within the capacity range
    void increase_capacity(size_t index);
};

#endif //BOOLVECTOR_HPP
