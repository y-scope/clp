#ifndef GLT_PLATFORM_HPP
#define GLT_PLATFORM_HPP

#include <cstdint>

namespace glt {
/**
 * Enum defining the supported platforms. This allows us to use C++ constants instead of macros when
 * defining code that's platform-dependent. Using constants is generally cleaner than using macros
 * everywhere since the code isn't completely invisible to the compiler when a macro is not set.
 * However, it does mean that we have to define shims for symbols that exist on one platform and not
 * the others. Luckily, defining shims can generally be done in headers rather than being
 * interspersed in functions. Moreover, by defining these shims, it makes it very clear what symbols
 * are missing on different platforms.
 *
 * For example, if we define some code conditionally for macOS:
 * - With macros:
 *
 *   #if defined(__APPLE__) || defined(__MACH__)
 *   method(MACOS_SPECIFIC_MACRO);
 *   #else
 *   method(LINUX_SPECIFIC_MACRO);
 *   #endif
 *
 * - With C++ constants
 *
 *   if constexpr (Platforms::MacOs == cCurrentPlatform) {
 *       method(MACOS_SPECIFIC_MACRO);
 *   } else {
 *       method(LINUX_SPECIFIC_MACRO);
 *   }
 *
 * When using C++ constants, this code is more readable and in case we make a mistake like
 * forgetting a semicolon, the compiler will warn us no matter what platform we're building on. The
 * price we pay is that we have to write a shim for MACOS_SPECIFIC_MACRO and LINUX_SPECIFIC_MACRO.
 */
enum class Platform {
    MacOs = 0,
    Linux,
};

// Define the current platform based on which platform macros exist and are supported.
#if defined(__APPLE__) || defined(__MACH__)
constexpr Platform cCurrentPlatform = Platform::MacOs;
#else
constexpr Platform cCurrentPlatform = Platform::Linux;
#endif
}  // namespace glt

#endif  // GLT_PLATFORM_HPP
