#ifndef REDUCER_REDUCER_SERVER
#define REDUCER_REDUCER_SERVER

#include <optional>
#include <set>

#include <boost/asio.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <msgpack.hpp>

#include "../clp/TraceableException.hpp"
#include "CommandLineArguments.hpp"
#include "Pipeline.hpp"

namespace reducer {
enum class ServerStatus : uint8_t {
    Idle,
    Running,
    ReceivedAllResults,
    RecoverableFailure,
    UnrecoverableFailure
};

namespace JobAttributes {
char const cBucketSize[] = "bucket_size";
}  // namespace JobAttributes

/**
 * Class which manages interactions with the jobs database and result cache database. Also holds
 * state for the reducer job this server is handling.
 */
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
     * Take in a msgpack query configuration and configure the in memory aggregation pipeline
     */
    void set_up_pipeline(std::map<std::string, msgpack::type::variant>& query_config);

    /**
     * Synchronously send a generic ack message to the search scheduler.
     * @return true if the ack is sent succesfully
     * @return false on any error
     */
    bool ack_search_scheduler();

    /**
     * Upsert the current set of results from the reducer pipeline to MongoDB and clear the tags
     * updated in the last period.
     * Executed repeatedly in the main polling loop while running a reduction pipeline that is set
     * to periodically upsert results.
     * @return true on success
     * @return false on any error
     */
    bool upsert_timeline_results();

    /**
     * Publish final reducer pipeline results to MongoDB.
     * @return true if the insert succeeds, false on failure
     */
    bool publish_pipeline_results();

    /**
     * If all results have been received then this function tries to push results to the results
     * cache and notify the search scheduler. If there are still results in flight then this
     * function will succeed silently.
     */
    bool try_finalize_results();

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

    /**
     * Increment the counter tracking the number of active receiver tasks which may possibly receive
     * some results.
     */
    void increment_remaining_receiver_tasks() { ++m_remaining_receiver_tasks; }

    /**
     * Decrement the counter tracking the number of active receiver tasks which may possibly receive
     * some results. If the counter hits zero, and the server is in state ReceivedAllResults then
     * the results of the aggregation are uploaded to the results cache, and the scheduler is
     * notified of task completion.
     */
    void decrement_remaining_receiver_tasks();

    /**
     * Opens a connection between this reducer and the scheduler
     * @param endpoint an endpoint that can be used to connect to the scheduler
     * @return true if a connection is opened succesfully
     */
    bool register_with_scheduler(boost::asio::ip::tcp::resolver::results_type const& endpoint);

    /**
     * @return a reference to the tcp socket used to communicate with the search scheduler
     */
    boost::asio::ip::tcp::socket& get_scheduler_update_socket() { return m_scheduler_socket; }

    /**
     * @return a reference to the buffer used to receive messages from the search scheduler
     */
    std::vector<char>& get_scheduler_update_buffer() { return m_scheduler_update_buffer; }

    /**
     * @return a reference to the timer object used to periodically upsert results to the results
     * cache
     */
    boost::asio::steady_timer& get_upsert_timer() { return m_upsert_timer; }

    /**
     * Stop the event loop by closing the connection with the scheduler, and cancelling any ongoing
     * operations on this server's listener socket. There may be a short period where periodic tasks
     * and results receivers keep running before the event loop exits.
     */
    void stop_event_loop();

private:
    boost::asio::io_context m_ioctx;
    boost::asio::ip::tcp::acceptor m_tcp_acceptor;
    boost::asio::ip::tcp::socket m_scheduler_socket;
    boost::asio::steady_timer m_upsert_timer;
    std::vector<char> m_scheduler_update_buffer;
    std::unique_ptr<Pipeline> m_pipeline;
    ServerStatus m_status;
    int64_t m_job_id;
    int m_reducer_port;
    mongocxx::client m_mongodb_client;
    mongocxx::database m_mongodb_results_database;
    mongocxx::collection m_mongodb_results_collection;
    std::string m_reducer_host;
    bool m_timeline_aggregation;
    std::set<GroupTags> m_updated_tags;
    int m_polling_interval_ms;
    int m_remaining_receiver_tasks{0};
};

}  // namespace reducer

#endif  // REDUCER_REDUCER_SERVER
