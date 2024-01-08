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

    boost::asio::io_context ioctx;
    boost::asio::ip::tcp::acceptor acceptor;
    std::unique_ptr<Pipeline> pipeline;
    clp::MySQLDB db;  // TODO: consider switching to boost asio mysql connector
    ServerStatus status;
    int64_t job_id;
    int64_t port;
    mongocxx::client mongodb_client;
    mongocxx::database results_database;
    mongocxx::collection results_collection;
    mongocxx::collection jobs_metric_collection;
    std::string host;
    std::string mongodb_jobs_metric_collection;
    int polling_interval_ms;
    bool timeline_aggregation;
    std::set<GroupTags> updated_tags;

    explicit ServerContext(CommandLineArguments& args);

    ServerStatus execute_assign_new_job();

    bool execute_update_job_status(JobStatus new_status);

    ServerStatus execute_poll_job_done();

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
};

}  // namespace reducer

#endif  // REDUCER_REDUCER_SERVER
