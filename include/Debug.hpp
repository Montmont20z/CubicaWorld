// Debug.hpp
/* Usage:
LOG_DEBUG("Generating terrain");
LOG_INFO("Generated terrain");
LOG_WARNING("Unexpected landform, falling back to plains");
LOG_ERROR("Failed to generate chunk");
*/

#pragma once

#ifdef DEBUG_MODE

#include <iostream>
    #define LOG_DEBUG(message) \
        std::cout << "[DEBUG] " << message << '\n'

    #define LOG_INFO(message) \
        std::cerr << "[INFO] " << message << '\n'

    #define LOG_WARNING(message) \
        std::cout << "[WARNING] " << message << '\n'

    #define LOG_ERROR(message) \
        std::cerr << "[ERROR] " << message << '\n'

#else

    #define LOG_DEBUG(message) ((void)0)
    #define LOG_INFO(message) ((void)0)
    #define LOG_WARNING(message) ((void)0)
    #define LOG_ERROR(message) ((void)0)

#endif