# Search query flow

This document details the complete flow of a search query from WebUI client submission to result 
retrieval in the CLP package, including both CLP-S (JSON logs) and CLP (Text logs) configurations.

## Overview

The CLP search query flow involves multiple components working together:
- **WebUI client**: Submits search queries and displays results
- **WebUI server**: Handles API requests, coordinates the search process, and retrieves results
- **MySQL database**: Stores job information and metadata
- **Job orchestration**: Uses Celery to manage query execution
- **MongoDB**: Caches search results with real-time updates

### Search query sequence diagram

The diagram below illustrates the end-to-end lifecycle of a search query within the CLP package.

:::{mermaid}
sequenceDiagram
    participant Client as WebUI client
    participant Server as WebUI server
    participant MySQL as MySQL database
    participant Mongo as MongoDB
    participant Scheduler as Job scheduler
    participant Worker as CLP/CLP-S workers

    %% Step 1: Query submission from client
    Client->>Server: POST /api/search/query (QueryJobCreation)

    %% Step 2: Server-side processing
    Server->>MySQL: Insert search + aggregation jobs
    Server->>Mongo: Create collections (searchJobId, aggregationJobId)
    Server->>Mongo: Insert SearchResultsMetadataDocument
    Server-->>Client: Return {searchJobId, aggregationJobId}

    %% Step 3: Job orchestration with job scheduler
    Scheduler->>MySQL: Poll for jobs (PENDING)
    MySQL-->>Scheduler: Pending jobs list
    Scheduler->>Scheduler: Create sub-tasks per archive
    Scheduler->>Worker: Dispatch search/aggregation tasks

    %% Step 4: Result caching in MongoDB
    Worker->>Worker: Run clp / clp-s binaries
    Worker->>Mongo: Write search results to job collections

    %% Step 5: Job status management
    Server->>Mongo: Update SearchResultsMetadataDocument
    MySQL->>MySQL: Update job state (RUNNING → SUCCEEDED/FAILED)

    %% Step 6: Result retrieval and streaming
    Client->>Server: Subscribe (collection::find::subscribe)
    Server->>Mongo: Watch job collection
    Mongo-->>Server: New documents (real-time)
    Server-->>Client: Emit collection::find::update events
    Client->>Server: Unsubscribe (collection::find::unsubscribe)
:::

## Package configuration: CLP-S vs CLP

CLP comes in two flavors with different capabilities: **CLP-S (JSON logs)** and **CLP (Text logs)**.
See the [Quick Start Guide](../user-docs/quick-start/index.md) for details on choosing a flavor.

The storage and query engine settings are configured in the package's `etc/clp-config.yml` and 
they are passed to the WebUI client via `client/public/settings.json` and the server via
`server/settings.json` at startup.

## Complete flow

### 1. Query submission from client

When a client wants to perform a search, it makes a POST request to the WebUI server:

**API Endpoint**: `POST /api/search/query`

**Request Body**:
```ts
interface QueryJobCreation {
  "dataset": null | string,
  "ignoreCase": boolean,
  "queryString": string,
  "timeRangeBucketSizeMillis": number,
  "timestampBegin": number,
  "timestampEnd": number
}
```

**Important Note on Dataset Field**:
- The `dataset` field is required and only used by CLP-S (JSON logs) configurations
  - For CLP (Text logs) configurations, this field should typically be `null`

**Response**:
```ts
interface QueryJob {
  "searchJobId": number,
  "aggregationJobId": number
}
```

### 2. Server-side processing

Upon receiving the query, the server:

1. **Creates two jobs in MySQL database**:
   - **Search job**: For basic search results
   - **Aggregation job**: For time-based aggregations

2. **Creates MongoDB collections**:
   - Collection named after `searchJobId`
   - Collection named after `aggregationJobId`

3. **Inserts metadata** into `SearchResultsMetadataDocument` in MongoDB (refer to `components/webui/common/src/metadata.ts`):

   ```typescript
   interface SearchResultsMetadataDocument {
       /** Search job ID as string */
       _id: string;
   
       /** Error message if job failed, null otherwise */
       errorMsg: Nullable<string>;
   
       /** Error name if job failed, null otherwise */
       errorName: Nullable<string>;
   
       /** Current job state */
       lastSignal: SEARCH_SIGNAL | PRESTO_SEARCH_SIGNAL;
   
       /** Total results (only set when job completes) */
       numTotalResults?: number;
   
       /** Query engine used (CLP, CLP_S or PRESTO) */
       queryEngine: CLP_QUERY_ENGINES;
   }
   ```

4. **Returns job IDs** to the client

### 3. Job orchestration with Celery

When the query engine used is CLP or CLP_S (rather than Presto), the job orchestration system 
works as follows:

1. **Query scheduler**
   - Continuously polls MySQL database for jobs with status `PENDING`
   - For each pending job, it identifies matching archives based on time ranges and query parameters
   - Creates sub-tasks for each archive to be searched

2. **Task distribution**:
   - Tasks are distributed via Celery to workers
   - Each task corresponds to searching one or more archives
   - Uses the `clp-s` or `clp` binaries for actual search operations based on the configured storage engine

3. **Task execution**
   - Celery workers execute the search tasks using core CLP binaries
   - Results are written directly to MongoDB collections named after job IDs

### 4. Result caching in MongoDB

Search results are stored in MongoDB with the following characteristics:

1. **Collection structure**: Each job has its own collection named after its job ID
2. **Indexing**: Two indexes are created for efficient time-based queries:
   - `timestamp-ascending`: `{timestamp: 1, _id: 1}`
   - `timestamp-descending`: `{timestamp: -1, _id: -1}`
3. **Document format**: The document structure differs based on the package configuration:

   **For CLP-S (JSON logs)**:
   Each document contains:
     - `"timestamp"`: Timestamp of the log event
     - `"message"`: The decoded log message
     - `"archive_id"`: ID of the archive containing the log
     - `"original_path"`: The original file path
     - `"log_event_ix"`: Index of the log event in the archive

   NOTE: `archive_id`, `original_path` and `log_event_ix` are used to uniquely identify a log event,
   which can be used to retrieve the original context if needed.

   **For CLP (Text logs)**:
   Each document contains:
     - `"timestamp"`: Timestamp of the log event
     - `"message"`: The decoded log message
     - `"orig_file_path"`: Path of the original file that contains the result
     - `"orig_file_id"`: ID of the original file that contains the result
     - `"log_event_ix"`: Index of the log event in the original file

   NOTE: `orig_file_path`, `orig_file_id` and `log_event_ix` are used to uniquely identify a log 
   event, which can be used to retrieve the original context if needed.

4. **Real-time updates**: As tasks complete, results are immediately written to the MongoDB collection

### 5. Job status management

Job states are tracked in the MySQL database using the `QueryJobStatus` enum:

```py
class QueryJobStatus(StatusIntEnum):
    # Job queued but not yet started
    PENDING = 0
    # Job currently executing
    RUNNING = auto()
    # Job completed successfully
    SUCCEEDED = auto()
    # Job failed during execution
    FAILED = auto()
    # Job cancellation requested
    CANCELLING = auto()
    # Job was cancelled
    CANCELLED = auto()
    # Job was forcefully killed
    KILLED = auto()
```

### 6. Result retrieval and streaming

Query progress and results can be retrieved through the MongoDB collections:

#### Search job metadata updates

As the search job progresses, the WebUI server also inserts a special document in the search job 
metadata collection:

- **Metadata updates**: When jobs complete, metadata is updated in `Searchresultsmetadatadocument`
  with:
  - `lastSignal` set to `RESP_DONE`
  - `numTotalResults` indicating count of results
  - `errorMsg` if the job failed

#### Real-time streaming via Socket.IO
- **Subscription**: Clients can subscribe to real-time updates using the `collection::find::subscribe` event
- **Event flow**:
  1. Client sends subscription request with collection name (job ID) and query parameters
  2. Server responds with `collection::find::update` events as documents are added/changed in the collection
  3. Updates continue until the job completes

**Socket event types**:
- `collection::find::subscribe`: Subscribe to collection changes
- `collection::find::update`: Server sends updates when collection content changes
- `collection::find::unsubscribe`: Unsubscribe from updates
