#ifndef CLP_CLO_CONSTANTS_HPP
#define CLP_CLO_CONSTANTS_HPP

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays, readability-identifier-naming)
namespace clp::clo::cResultsCacheKeys {
namespace IrOutput {
constexpr char Path[]{"path"};
constexpr char StreamId[]{"stream_id"};
constexpr char BeginMsgIx[]{"begin_msg_ix"};
constexpr char EndMsgIx[]{"end_msg_ix"};
constexpr char IsLastChunk[]{"is_last_chunk"};
}  // namespace IrOutput

namespace SearchOutput {
constexpr char OrigFileId[]{"orig_file_id"};
constexpr char OrigFilePath[]{"orig_file_path"};
constexpr char LogEventIx[]{"log_event_ix"};
constexpr char Timestamp[]{"timestamp"};
constexpr char Message[]{"message"};
}  // namespace SearchOutput
}  // namespace clp::clo::cResultsCacheKeys

// NOLINTEND(cppcoreguidelines-avoid-c-arrays, readability-identifier-naming)

#endif  // CLP_CLO_CONSTANTS_HPP
