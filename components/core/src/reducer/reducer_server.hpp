#ifndef REDUCER_REDUCER_SERVER
#define REDUCER_REDUCER_SERVER

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <set>

#include <boost/asio.hpp>

#include "../clp/MySQLDB.hpp"
#include "../clp/MySQLPreparedStatement.hpp"
#include "../clp/TraceableException.hpp"
#include "CommandLineArguments.hpp"
#include "Pipeline.hpp"

namespace reducer {
enum class ServerStatus {
    IDLE,
    RUNNING,
    FINISHING_SUCCESS,
    FINISHING_REDUCER_ERROR,
    FINISHING_REMOTE_ERROR,
    FINISHING_CANCELLED
};

// This enum is a hidden binding between the python
// scheduler and this c++ reducer
enum class JobStatus : int {
    PENDING = 0,
    RUNNING,
    DONE,
    SUCCESS,
    SUCCESS_WITH_ERRORS,
    FAILED,
    CANCELLING,
    CANCELLED,
    PENDING_REDUCER,
    REDUCER_READY,
    PENDING_REDUCER_DONE,
    NO_MATCHING_ARCHIVE,
    WAITING_FOR_BATCH
};

class ServerContext {
public:
    // Types
    class OperationFailed : public clp::TraceableException {
    public:
        // Constructors
        OperationFailed(clp::ErrorCode error_code, char const* filename, int line_number)
                : clp::TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override { return "ServerContext operation failed"; }
    };

    // Constructors
    explicit ServerContext(CommandLineArguments& args);

    // Methods
    void run() { m_ioctx.run(); }

    boost::asio::io_context& get_io_context() { return m_ioctx; }

    boost::asio::ip::tcp::acceptor& get_tcp_acceptor() { return m_tcp_acceptor; }

    int get_polling_interval() const { return m_polling_interval_ms; }

    bool is_timeline_aggregation() const { return m_timeline_aggregation; }

    std::string const& get_reducer_host() const { return m_reducer_host; }

    int64_t get_reducer_port() const { return m_reducer_port; }

    int64_t get_job_id() const { return m_job_id; }

    ServerStatus get_status() const { return m_status; }

    void set_status(ServerStatus new_status) { m_status = new_status; }

    ServerStatus execute_assign_new_job();

    bool execute_update_job_status(JobStatus new_status);

    ServerStatus execute_poll_job_done();

    ServerStatus upsert_timeline_results();

    bool publish_pipeline_results();

    bool publish_reducer_job_metrics(JobStatus finish_status);

    void push_record_group(RecordGroup const& record_group) {
        if (m_timeline_aggregation) {
            m_updated_tags.insert(record_group.get_tags());
        }
        m_pipeline->push_record_group(record_group);
    }

    void reset();

private:
    static constexpr char const cGetNewJobs[]
            = "SELECT id, search_config FROM distributed_search_jobs WHERE status={}";
    static constexpr char const cTakeSearchJobStatement[]
            = "UPDATE distributed_search_jobs SET status=?, reducer_port=?, reducer_host=? WHERE "
              "status=? and id=?";
    static constexpr char const cUpdateJobStatusStatement[]
            = "UPDATE distributed_search_jobs SET status=? WHERE id=?";
    static constexpr char const cPollJobDone[]
            = "SELECT status FROM distributed_search_jobs WHERE id={}";
    std::string m_get_new_jobs;
    std::unique_ptr<clp::MySQLPreparedStatement> m_take_search_job;
    std::unique_ptr<clp::MySQLPreparedStatement> m_update_job_status;

    boost::asio::io_context m_ioctx;
    boost::asio::ip::tcp::acceptor m_tcp_acceptor;
    std::unique_ptr<Pipeline> m_pipeline;
    clp::MySQLDB m_db;  // TODO: consider switching to boost asio mysql connector
    ServerStatus m_status;
    int64_t m_job_id;
    int64_t m_reducer_port;
    mongocxx::client m_mongodb_client;
    mongocxx::database m_mongodb_results_database;
    mongocxx::collection m_mongodb_results_collection;
    std::string m_reducer_host;
    std::string m_mongodb_job_metrics_collection;
    bool m_timeline_aggregation;
    std::set<GroupTags> m_updated_tags;
    int m_polling_interval_ms;
};

}  // namespace reducer

#endif  // REDUCER_REDUCER_SERVER
