#include "TimestampEntry.hpp"

#include <cmath>
#include <sstream>

#include "Utils.hpp"

namespace clp_s {
void TimestampEntry::ingest_timestamp(epochtime_t timestamp) {
    if (m_encoding == DoubleEpoch) {
        if (timestamp < std::ceil(m_epoch_start_double)) {
            m_epoch_start_double = timestamp;
        }
        if (timestamp > std::floor(m_epoch_end_double)) {
            m_epoch_end_double = timestamp;
        }

        return;
    }

    if (m_encoding == UnkownTimestampEncoding) {
        m_encoding = Epoch;
    }

    if (timestamp < m_epoch_start) {
        m_epoch_start = timestamp;
    }
    if (timestamp > m_epoch_end) {
        m_epoch_end = timestamp;
    }
}

void TimestampEntry::ingest_timestamp(double timestamp) {
    if (m_encoding == UnkownTimestampEncoding) {
        m_encoding = DoubleEpoch;
    } else if (m_encoding == Epoch) {
        m_encoding = DoubleEpoch;
        m_epoch_start_double = m_epoch_start;
        m_epoch_end_double = m_epoch_end;
    }

    if (timestamp < m_epoch_start_double) {
        m_epoch_start_double = timestamp;
    }
    if (timestamp > m_epoch_end_double) {
        m_epoch_end_double = timestamp;
    }
}

void TimestampEntry::merge_range(TimestampEntry const& entry) {
    if (entry.m_encoding == Epoch) {
        ingest_timestamp(entry.m_epoch_start);
        ingest_timestamp(entry.m_epoch_end);
    } else if (entry.m_encoding == DoubleEpoch) {
        ingest_timestamp(entry.m_epoch_start_double);
        ingest_timestamp(entry.m_epoch_end_double);
    }
}

void TimestampEntry::write_to_stream(std::stringstream& stream) const {
    write_numeric_value<uint64_t>(stream, m_key_name.size());
    stream.write(m_key_name.data(), m_key_name.size());
    write_numeric_value<uint64_t>(stream, m_column_ids.size());
    for (auto const& id : m_column_ids) {
        write_numeric_value<int32_t>(stream, id);
    }

    write_numeric_value<TimestampEncoding>(stream, m_encoding);
    if (m_encoding == Epoch) {
        write_numeric_value<epochtime_t>(stream, m_epoch_start);
        write_numeric_value<epochtime_t>(stream, m_epoch_end);
    } else if (m_encoding == DoubleEpoch) {
        write_numeric_value<double>(stream, m_epoch_start_double);
        write_numeric_value<double>(stream, m_epoch_end_double);
    }
}

ErrorCode TimestampEntry::try_read_from_file(ZstdDecompressor& decompressor) {
    ErrorCode error_code;

    uint64_t column_len;
    error_code = decompressor.try_read_numeric_value<uint64_t>(column_len);
    if (ErrorCodeSuccess != error_code) {
        return error_code;
    }

    error_code = decompressor.try_read_string(column_len, m_key_name);
    if (ErrorCodeSuccess != error_code) {
        return error_code;
    }

    uint64_t column_ids_size;
    error_code = decompressor.try_read_numeric_value<uint64_t>(column_ids_size);
    if (ErrorCodeSuccess != error_code) {
        return error_code;
    }
    for (int i = 0; i < column_ids_size; ++i) {
        int32_t id;
        error_code = decompressor.try_read_numeric_value<int32_t>(id);
        if (ErrorCodeSuccess != error_code) {
            return error_code;
        }
        m_column_ids.insert(id);
    }

    uint64_t encoding;
    error_code = decompressor.try_read_numeric_value<TimestampEncoding>(m_encoding);
    if (ErrorCodeSuccess != error_code) {
        return error_code;
    }

    if (m_encoding == Epoch) {
        error_code = decompressor.try_read_numeric_value<epochtime_t>(m_epoch_start);
        if (ErrorCodeSuccess != error_code) {
            return error_code;
        }
        error_code = decompressor.try_read_numeric_value<epochtime_t>(m_epoch_end);
        if (ErrorCodeSuccess != error_code) {
            return error_code;
        }
    } else if (m_encoding == DoubleEpoch) {
        error_code = decompressor.try_read_numeric_value<double>(m_epoch_start_double);
        if (ErrorCodeSuccess != error_code) {
            return error_code;
        }
        error_code = decompressor.try_read_numeric_value<double>(m_epoch_end_double);
        if (ErrorCodeSuccess != error_code) {
            return error_code;
        }
    }

    return error_code;
}

void TimestampEntry::read_from_file(ZstdDecompressor& decompressor) {
    auto error_code = try_read_from_file(decompressor);
    if (ErrorCodeSuccess != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }
}

EvaluatedValue TimestampEntry::evaluate_filter(FilterOperation op, double timestamp) {
    if (op == FilterOperation::EXISTS || op == FilterOperation::NEXISTS) {
        return EvaluatedValue::Unknown;
    }

    if (m_encoding == DoubleEpoch) {
        switch (op) {
            case FilterOperation::EQ:
                if (timestamp >= m_epoch_start_double && timestamp <= m_epoch_end_double) {
                    return EvaluatedValue::Unknown;
                } else {
                    return EvaluatedValue::False;
                }
            case FilterOperation::NEQ:
                if (timestamp >= m_epoch_start_double && timestamp <= m_epoch_end_double) {
                    return EvaluatedValue::Unknown;
                } else {
                    return EvaluatedValue::True;
                }
            case FilterOperation::LT:
                if (timestamp > m_epoch_end_double) {
                    return EvaluatedValue::True;
                } else if (timestamp <= m_epoch_start_double) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            case FilterOperation::LTE:
                if (timestamp >= m_epoch_end_double) {
                    return EvaluatedValue::True;
                } else if (timestamp < m_epoch_start_double) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            case FilterOperation::GT:
                if (timestamp < m_epoch_start_double) {
                    return EvaluatedValue::True;
                } else if (timestamp >= m_epoch_end_double) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            case FilterOperation::GTE:
                if (timestamp <= m_epoch_start_double) {
                    return EvaluatedValue::True;
                } else if (timestamp > m_epoch_end_double) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            default:
                return EvaluatedValue::Unknown;
        }
    } else if (m_encoding == Epoch) {
        double epoch_start_tmp = m_epoch_start, epoch_end_tmp = m_epoch_end;
        switch (op) {
            case FilterOperation::EQ:
                if (timestamp >= epoch_start_tmp && timestamp <= epoch_end_tmp) {
                    return EvaluatedValue::Unknown;
                } else {
                    return EvaluatedValue::False;
                }
            case FilterOperation::NEQ:
                if (timestamp >= epoch_start_tmp && timestamp <= epoch_end_tmp) {
                    return EvaluatedValue::Unknown;
                } else {
                    return EvaluatedValue::True;
                }
            case FilterOperation::LT:
                if (timestamp > epoch_end_tmp) {
                    return EvaluatedValue::True;
                } else if (timestamp <= epoch_start_tmp) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            case FilterOperation::LTE:
                if (timestamp >= epoch_end_tmp) {
                    return EvaluatedValue::True;
                } else if (timestamp < epoch_start_tmp) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            case FilterOperation::GT:
                if (timestamp < epoch_start_tmp) {
                    return EvaluatedValue::True;
                } else if (timestamp >= epoch_end_tmp) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            case FilterOperation::GTE:
                if (timestamp <= epoch_start_tmp) {
                    return EvaluatedValue::True;
                } else if (timestamp > epoch_end_tmp) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            default:
                return EvaluatedValue::Unknown;
        }
    } else {
        return EvaluatedValue::Unknown;
    }
}

EvaluatedValue TimestampEntry::evaluate_filter(FilterOperation op, epochtime_t timestamp) {
    if (op == FilterOperation::EXISTS || op == FilterOperation::NEXISTS) {
        return EvaluatedValue::Unknown;
    }

    if (m_encoding == DoubleEpoch) {
        /**
         * TODO: this borrows logic from the double_as_int function
         * should
         */
        epochtime_t epoch_start_tmp_ltgte = std::ceil(m_epoch_start_double);
        epochtime_t epoch_start_tmp_gtlte = std::floor(m_epoch_start_double);
        epochtime_t epoch_end_tmp_ltgte = std::ceil(m_epoch_end_double);
        epochtime_t epoch_end_tmp_gtlte = std::floor(m_epoch_end_double);
        switch (op) {
            case FilterOperation::EQ:
                if (timestamp >= epoch_start_tmp_ltgte && timestamp <= epoch_end_tmp_gtlte) {
                    return EvaluatedValue::Unknown;
                } else {
                    return EvaluatedValue::False;
                }
            case FilterOperation::NEQ:
                if (timestamp >= epoch_start_tmp_ltgte && timestamp <= epoch_end_tmp_gtlte) {
                    return EvaluatedValue::Unknown;
                } else {
                    return EvaluatedValue::True;
                }
            case FilterOperation::LT:
                if (timestamp > epoch_end_tmp_gtlte) {
                    return EvaluatedValue::True;
                } else if (timestamp <= epoch_start_tmp_gtlte) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            case FilterOperation::LTE:
                if (timestamp >= epoch_end_tmp_ltgte) {
                    return EvaluatedValue::True;
                } else if (timestamp < epoch_start_tmp_ltgte) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            case FilterOperation::GT:
                if (timestamp < epoch_start_tmp_ltgte) {
                    return EvaluatedValue::True;
                } else if (timestamp >= epoch_end_tmp_ltgte) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            case FilterOperation::GTE:
                if (timestamp <= epoch_start_tmp_gtlte) {
                    return EvaluatedValue::True;
                } else if (timestamp > epoch_end_tmp_gtlte) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            default:
                return EvaluatedValue::Unknown;
        }
    } else if (m_encoding == Epoch) {
        switch (op) {
            case FilterOperation::EQ:
                if (timestamp >= m_epoch_start && timestamp <= m_epoch_end) {
                    return EvaluatedValue::Unknown;
                } else {
                    return EvaluatedValue::False;
                }
            case FilterOperation::NEQ:
                if (timestamp >= m_epoch_start && timestamp <= m_epoch_end) {
                    return EvaluatedValue::Unknown;
                } else {
                    return EvaluatedValue::True;
                }
            case FilterOperation::LT:
                if (timestamp > m_epoch_end) {
                    return EvaluatedValue::True;
                } else if (timestamp <= m_epoch_start) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            case FilterOperation::LTE:
                if (timestamp >= m_epoch_end) {
                    return EvaluatedValue::True;
                } else if (timestamp < m_epoch_start) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            case FilterOperation::GT:
                if (timestamp < m_epoch_start) {
                    return EvaluatedValue::True;
                } else if (timestamp >= m_epoch_end) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            case FilterOperation::GTE:
                if (timestamp <= m_epoch_start) {
                    return EvaluatedValue::True;
                } else if (timestamp > m_epoch_end) {
                    return EvaluatedValue::False;
                } else {
                    return EvaluatedValue::Unknown;
                }
            default:
                return EvaluatedValue::Unknown;
        }
    } else {
        return EvaluatedValue::Unknown;
    }
}

epochtime_t TimestampEntry::get_begin_timestamp() const {
    if (Epoch == m_encoding) {
        return m_epoch_start;
    } else if (DoubleEpoch == m_encoding) {
        return static_cast<epochtime_t>(m_epoch_start_double);
    }
    return 0;
}

epochtime_t TimestampEntry::get_end_timestamp() const {
    if (Epoch == m_encoding) {
        return m_epoch_end;
    } else if (DoubleEpoch == m_encoding) {
        return static_cast<epochtime_t>(std::ceil(m_epoch_end_double));
    }
    return 0;
}
}  // namespace clp_s
