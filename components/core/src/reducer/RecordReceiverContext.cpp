#include "RecordReceiverContext.hpp"

#include "../clp/spdlog_with_specializations.hpp"
#include "DeserializedRecordGroup.hpp"
#include "types.hpp"

namespace reducer {
bool RecordReceiverContext::read_connection_init_packet() {
    job_id_t job_id{0};

    if (m_buf_num_bytes_occupied != sizeof(job_id)) {
        SPDLOG_ERROR("Rejecting connection due to invalid negotiation");
        return false;
    }

    memcpy(&job_id, m_buf.data(), sizeof(job_id));
    if (job_id != m_server_ctx->get_job_id()) {
        SPDLOG_ERROR(
                "Rejecting connection from worker with job_id={} during processing of "
                "job_id={}",
                job_id,
                m_server_ctx->get_job_id()
        );
        return false;
    }
    m_buf_num_bytes_occupied = 0;

    return true;
}

bool RecordReceiverContext::send_connection_accept_packet() {
    char const response = 'y';
    boost::system::error_code e;
    auto transferred
            = boost::asio::write(m_socket, boost::asio::buffer(&response, sizeof(response)), e);
    if (e || transferred < sizeof(response)) {
        SPDLOG_ERROR("Rejecting connection due to failure to send acceptance - {}", e.message());
        return false;
    }

    return true;
}

bool RecordReceiverContext::read_record_groups_packet() {
    size_t record_size{0};
    auto* read_head = m_buf.data();
    while (m_buf_num_bytes_occupied > 0) {
        if (m_buf_num_bytes_occupied < sizeof(record_size)) {
            break;
        }
        memcpy(&record_size, read_head, sizeof(record_size));

        // terminate if record group size is over 16MB
        if (record_size >= cMaxRecordSize) {
            SPDLOG_ERROR("Record too large: {}B", record_size);
            return false;
        }

        if (m_buf_num_bytes_occupied < record_size + sizeof(record_size)) {
            break;
        }
        read_head += sizeof(record_size);

        auto record_group = DeserializedRecordGroup{read_head, record_size};
        m_server_ctx->push_record_group(record_group.get_tags(), record_group.record_iter());
        m_buf_num_bytes_occupied -= (record_size + sizeof(record_size));
        read_head += record_size;
    }

    if (m_buf_num_bytes_occupied > 0) {
        if (m_buf.size() < record_size + sizeof(record_size)) {
            std::vector<char> new_buf(sizeof(record_size) + record_size);
            std::copy(m_buf.begin(), m_buf.end(), new_buf.begin());
            m_buf.swap(new_buf);
        } else {
            memmove(m_buf.data(), read_head, m_buf_num_bytes_occupied);
        }
    }

    return true;
}
}  // namespace reducer
