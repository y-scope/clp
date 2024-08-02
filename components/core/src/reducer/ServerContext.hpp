#ifndef REDUCER_SERVERCONTEXT_HPP
#define REDUCER_SERVERCONTEXT_HPP

#include <optional>
#include <set>

#include <boost/asio.hpp>
#include <json/single_include/nlohmann/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>

#include "../clp/TraceableException.hpp"
#include "CommandLineArguments.hpp"
#include "Pipeline.hpp"
#include "types.hpp"

namespace reducer {
enum class ServerStatus : uint8_t {
    Idle,
    Running,
    ReceivedAllResults,
    RecoverableFailure,
    UnrecoverableFailure
};

namespace cJobAttributes {
constexpr char JobId[] = "job_id";
constexpr char TimeBucketSize[] = "count_by_time_bucket_size";
}  // namespace cJobAttributes

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
                : clp::TraceableException{error_code, filename, line_number} {}

        // Methods
        [[nodiscard]] char const* what() const noexcept override {
            return "reducer::ServerContext operation failed";
        }
    };

    // Constructors
    explicit ServerContext(CommandLineArguments& args);

    // Methods
    /**
     * Resets all state in the ServerContext. This method must be called between invocations of
     * run().
     */
    void reset();

    /**
     * Executes the server event loop until no tasks remain.
     */
    void run() { m_ioctx.run(); }

    /**
     * Stops the event loop by closing the connection to the scheduler, and cancelling any ongoing
     * operations on this server's listener socket.
     *
     * NOTE: There may be a short period where periodic tasks and results receivers keep running
     * before the event loop exits.
     */
    void stop_event_loop();

    /**
     * Opens a connection between this reducer and the scheduler.
     * @param endpoints A list of endpoints that can be used to connect to the scheduler.
     * @return Whether a connection was opened successfully.
     */
    bool register_with_scheduler(boost::asio::ip::tcp::resolver::results_type const& endpoints);

    /**
     * Synchronously sends a generic acknowledgement to the query scheduler.
     * @return Whether the acknowledgement was sent successfully.
     */
    bool ack_query_scheduler();

    /**
     * Increments the number of active receiver tasks which may receive some results.
     */
    void increment_num_active_receiver_tasks() { ++m_num_active_receiver_tasks; }

    /**
     * Decrements the number of active receiver tasks, and calls try_finalize_results if the server
     * is in the state ReceivedAllResults and there are no remaining active receiver tasks.
     */
    void decrement_num_active_receiver_tasks();

    /**
     * Sets up an in-memory aggregation pipeline according to the given query config.
     * @param query_config
     */
    void set_up_pipeline(nlohmann::json const& query_config);

    /**
     * Pushes a record group into the reducer pipeline.
     * @param group_tags The tags in the record group.
     * @param record_it An iterator for the records in the record group.
     */
    void push_record_group(GroupTags const& tags, ConstRecordIterator& record_it);

    /**
     * Upserts the current set of timeline entries from the reducer pipeline to MongoDB and clears
     * the tags that were updated in the last period. This method is executed repeatedly in the main
     * polling loop while running a reduction pipeline that is set to periodically upsert results.
     * @return Whether the upsert succeeded (or was unnecessary).
     */
    bool upsert_timeline_results();

    /**
     * Publishes the reducer pipeline results to MongoDB.
     * @return Whether the publication succeeded.
     */
    bool publish_pipeline_results();

    /**
     * If all results have been received then this function tries to publish the pipeline's results
     * to the results cache and notify the query scheduler.
     * @return true if not all results have been received yet, or the results were published
     * successfully.
     * @return false otherwise.
     */
    bool try_finalize_results();

    boost::asio::io_context& get_io_context() { return m_ioctx; }

    boost::asio::ip::tcp::acceptor& get_tcp_acceptor() { return m_tcp_acceptor; }

    boost::asio::ip::tcp::socket& get_scheduler_update_socket() { return m_scheduler_socket; }

    std::vector<char>& get_scheduler_update_buffer() { return m_scheduler_update_buffer; }

    [[nodiscard]] std::string const& get_reducer_host() const { return m_reducer_host; }

    [[nodiscard]] int get_reducer_port() const { return m_reducer_port; }

    [[nodiscard]] ServerStatus get_status() const { return m_status; }

    void set_status(ServerStatus new_status) { m_status = new_status; }

    [[nodiscard]] job_id_t get_job_id() const { return m_job_id; }

    [[nodiscard]] bool is_timeline_aggregation() const { return m_is_timeline_aggregation; }

    boost::asio::steady_timer& get_upsert_timer() { return m_upsert_timer; }

    [[nodiscard]] int get_upsert_interval() const { return m_upsert_interval; }

private:
    boost::asio::io_context m_ioctx;
    boost::asio::ip::tcp::acceptor m_tcp_acceptor;
    boost::asio::ip::tcp::socket m_scheduler_socket;
    std::vector<char> m_scheduler_update_buffer;

    std::string m_reducer_host;
    int m_reducer_port;
    int m_num_active_receiver_tasks{0};

    ServerStatus m_status{ServerStatus::Idle};
    job_id_t m_job_id{-1};

    std::unique_ptr<Pipeline> m_pipeline;
    bool m_is_timeline_aggregation{false};
    std::set<GroupTags> m_updated_tags;

    boost::asio::steady_timer m_upsert_timer;
    int m_upsert_interval;

    mongocxx::client m_mongodb_client;
    mongocxx::database m_mongodb_results_database;
    mongocxx::collection m_mongodb_results_collection;
};

}  // namespace reducer

#endif  // REDUCER_SERVERCONTEXT_HPP
