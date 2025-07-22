#ifndef CLP_TRANSACTIONMANAGER_HPP
#define CLP_TRANSACTIONMANAGER_HPP

#include <type_traits>

namespace clp {
/**
 * A class that on destruction, performs different actions depending on whether a transaction
 * succeeds or fails. The default state assumes the transaction fails.
 * @tparam SuccessHandler A cleanup lambda to call on success.
 * @tparam FailureHandler A cleanup lambda to call on failure.
 */
template <typename SuccessHandler, typename FailureHandler>
requires(std::is_nothrow_invocable_v<SuccessHandler> && std::is_nothrow_invocable_v<FailureHandler>)
class TransactionManager {
public:
    // Constructor
    TransactionManager(SuccessHandler success_handler, FailureHandler failure_handler)
            : m_success_handler{success_handler},
              m_failure_handler{failure_handler} {}

    // Delete copy/move constructor and assignment
    TransactionManager(TransactionManager const&) = delete;
    TransactionManager(TransactionManager&&) = delete;
    auto operator=(TransactionManager const&) -> TransactionManager& = delete;
    auto operator=(TransactionManager&&) -> TransactionManager& = delete;

    // Destructor
    ~TransactionManager() {
        if (m_success) {
            m_success_handler();
        } else {
            m_failure_handler();
        }
    }

    // Methods
    /**
     * Marks the transaction as successful.
     */
    auto mark_success() -> void { m_success = true; }

private:
    // Variables
    SuccessHandler m_success_handler;
    FailureHandler m_failure_handler;
    bool m_success{false};
};
}  // namespace clp

#endif  // CLP_TRANSACTIONMANAGER_HPP
