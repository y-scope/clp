#ifndef REDUCER_REDUCER_SERVER
#define REDUCER_REDUCER_SERVER

#include <set>

#include <boost/asio.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>

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
    /**
     * Execute the server event loop until no tasks remain.
     * @return
     */
    void run() { m_ioctx.run(); }

    boost::asio::io_context& get_io_context() { return m_ioctx; }

    boost::asio::ip::tcp::acceptor& get_tcp_acceptor() { return m_tcp_acceptor; }

    int get_polling_interval() const { return m_polling_interval_ms; }

    bool is_timeline_aggregation() const { return m_timeline_aggregation; }

    std::string const& get_reducer_host() const { return m_reducer_host; }

    int get_reducer_port() const { return m_reducer_port; }

    int64_t get_job_id() const { return m_job_id; }

    ServerStatus get_status() const { return m_status; }

    void set_status(ServerStatus new_status) { m_status = new_status; }

    /**
     * Poll jobs table to and try to assign this server to run an available job.
     * Executed repeatedly in the main polling loop while the server is idle.
     * @return the new status of the ServerContext
     */
    ServerStatus take_job();

    /**
     * Write a new job status to the jobs table.
     * @return true on update success, false on failure
     */
    bool update_job_status(JobStatus new_status);

    /**
     * Poll the jobs table and check for an updated job status.
     * Executed repeatedly in the main polling loop when running a reduction operation.
     * @return the new status of the ServerContext
     */
    ServerStatus poll_job_done();

    /**
     * Upsert the current set of results from the reducer pipeline to MongoDB and clear the tags
     * updated in the last period.
     * Executed repeatedly in the main polling loop while running a reduction pipeline that is set
     * to periodically upsert results.
     * @return the new status of the ServerContext
     */
    ServerStatus upsert_timeline_results();

    /**
     * Publish final reducer pipeline results to MongoDB.
     * @return true if the insert succeeds, false on failure
     */
    bool publish_pipeline_results();

    /**
     * Publish metrics about job completion time and status to MongoDB.
     * @return true if the insert succeeds, false on failure
     */
    bool publish_reducer_job_metrics(JobStatus finish_status);

    /**
     * Pushes a group of records into the current local reducer pipeline. If this reducer
     * pipeline is periodically upserting results this function will also keep track of which
     * tags have been updated in the last period.
     * @param record_group RecordGroup being pushed
     * @return
     */
    void push_record_group(RecordGroup const& record_group) {
        if (m_timeline_aggregation) {
            m_updated_tags.insert(record_group.get_tags());
        }
        m_pipeline->push_record_group(record_group);
    }

    /**
     * Reset all state in the ServerContext. Must be called between invocations
     * of run().
     * @return
     */
    void reset();

private:
    static constexpr char const cGetNewJobs[] = "SELECT id, search_config FROM {} WHERE status={}";
    static constexpr char const cTakeSearchJobStatement[]
            = "UPDATE {} SET status=?, reducer_port=?, reducer_host=? WHERE "
              "status=? and id=?";
    static constexpr char const cUpdateJobStatusStatement[] = "UPDATE {} SET status=? WHERE id=?";
    static constexpr char const cPollJobDone[] = "SELECT status FROM {} WHERE id={}";
    std::string m_get_new_jobs_sql;
    std::string m_poll_job_done_sql;
    std::unique_ptr<clp::MySQLPreparedStatement> m_take_search_job_stmt;
    std::unique_ptr<clp::MySQLPreparedStatement> m_update_job_status_stmt;

    boost::asio::io_context m_ioctx;
    boost::asio::ip::tcp::acceptor m_tcp_acceptor;
    std::unique_ptr<Pipeline> m_pipeline;
    clp::MySQLDB m_db;  // TODO: consider switching to boost asio mysql connector
    ServerStatus m_status;
    int64_t m_job_id;
    int m_reducer_port;
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
