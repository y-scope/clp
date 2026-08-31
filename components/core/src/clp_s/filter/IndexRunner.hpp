#ifndef CLP_S_FILTER_INDEX_RUNNER_HPP
#define CLP_S_FILTER_INDEX_RUNNER_HPP

#include <cstdint>
#include <memory>

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/filter/BitmapView.hpp>

namespace clp_s::search::ast {
class Expression;
}  // namespace clp_s::search::ast

namespace clp_s::filter {
/**
 * View over the set of candidate archives passed to `IndexRunner::filter`, where bit i corresponds
 * to the local archive ID i. The component type fixes the word size used to represent the candidate
 * set as it is passed across the FFI boundary.
 */
using CandidateArchiveBitmapView = BitmapView<uint64_t>;

/**
 * Interface for using a deserialized index to narrow the set of archives that could match a query
 * at filtering time.
 */
class IndexRunner {
public:
    // Constructors
    IndexRunner() = default;

    // Delete copy constructor and assignment operator
    IndexRunner(IndexRunner const&) = delete;
    auto operator=(IndexRunner const&) -> IndexRunner& = delete;

    // Default move constructor and assignment operator
    IndexRunner(IndexRunner&&) noexcept = default;
    auto operator=(IndexRunner&&) noexcept -> IndexRunner& = default;

    // Destructor
    virtual ~IndexRunner() = default;

    // Methods
    /**
     * Narrows the set of candidate archives for a query by clearing the bits of archives that
     * cannot contain any results matching the query.
     *
     * @param query The root of the query AST to filter against.
     * @param candidate_archive_bitmap The set of candidate archives, where bit i corresponds to the
     * local archive ID i. Returns the narrowed set of candidate archives.
     * @return A void result on success, or an error code indicating the failure:
     * - Error codes are defined by the derived class.
     */
    [[nodiscard]] virtual auto filter(
            std::shared_ptr<clp_s::search::ast::Expression> const& query,
            CandidateArchiveBitmapView& candidate_archive_bitmap
    ) -> ystdlib::error_handling::Result<void>
            = 0;
};
}  // namespace clp_s::filter

#endif  // CLP_S_FILTER_INDEX_RUNNER_HPP
