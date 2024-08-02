#ifndef CLP_CLO_CONSTANTS_HPP
#define CLP_CLO_CONSTANTS_HPP

// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays, readability-identifier-naming)
namespace clp::clo::cResultsCacheKeys {
constexpr char OrigFileId[]{"orig_file_id"};

namespace IrOutput {
constexpr char Path[]{"path"};
constexpr char FileSplitId[]{"file_split_id"};
constexpr char BeginMsgIx[]{"begin_msg_ix"};
constexpr char EndMsgIx[]{"end_msg_ix"};
constexpr char IsLastIrChunk[]{"is_last_ir_chunk"};
}  // namespace IrOutput

namespace SearchOutput {
constexpr char OrigFilePath[]{"orig_file_path"};
constexpr char LogEventIx[]{"log_event_ix"};
constexpr char Timestamp[]{"timestamp"};
constexpr char Message[]{"message"};
}  // namespace SearchOutput
}  // namespace clp::clo::cResultsCacheKeys

// NOLINTEND(cppcoreguidelines-avoid-c-arrays, readability-identifier-naming)

#endif  // CLP_CLO_CONSTANTS_HPP
