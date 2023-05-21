#ifndef newReaderInterface_HPP
#define newReaderInterface_HPP

// Project headers
#include "ReaderInterface.hpp"

class BufferedReaderInterface : public ReaderInterface {
public:
    // Types
    class MyStringView {
    public:
        MyStringView(): m_buffer_pos(0), m_size(0) {}
        MyStringView(size_t pos, size_t size) :
            m_buffer_pos(pos), m_size(size) {}
        // variable
        size_t m_buffer_pos;
        size_t m_size;
    };
    [[nodiscard]] virtual bool try_read_string_view (MyStringView& str_view, size_t read_size) = 0;
    [[nodiscard]] virtual const char* get_buffer_ptr () = 0;
};


#endif // newReaderInterface_HPP
