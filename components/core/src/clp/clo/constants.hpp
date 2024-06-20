#ifndef CLP_CLO_CONSTANTS_HPP
#define CLP_CLO_CONSTANTS_HPP

namespace clp::clo { namespace cResultsCache {
constexpr char cOrigFileId[]{"orig_file_id"};

namespace IrOutput {
constexpr char cPath[]{"path"};
constexpr char cBeginMsgIx[]{"begin_msg_ix"};
constexpr char cEndMsgIx[]{"end_msg_ix"};
constexpr char cIsLastIrChunk[]{"is_last_ir_chunk"};
}  // namespace IrOutput

namespace SearchOutput {
constexpr char cOrigFilePath[]{"orig_file_path"};
constexpr char cLogEventIx[]{"log_event_ix"};
constexpr char cTimestamp[]{"timestamp"};
constexpr char cMessage[]{"message"};
}  // namespace SearchOutput
}}  // namespace clp::clo::cResultsCache

#endif  // CLP_CLO_CONSTANTS_HPP
