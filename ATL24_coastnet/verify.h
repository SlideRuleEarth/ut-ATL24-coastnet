#pragma once

#include <sstream>
#include <stdexcept>

/// @brief Support for VERIFY macro
inline void Verify (const char *e, const char *file, unsigned line)
{
    std::stringstream s;
    s << "verification failed in " << file << ", line " << line << ": " << e;
    throw std::runtime_error (s.str ());
}

/// @brief VERIFY that a statement is true, ignoring NDEBUG
#define VERIFY(e) (void)((e) || (Verify (#e, __FILE__, __LINE__), 0))
