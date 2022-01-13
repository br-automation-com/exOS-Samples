#include "WaterTankLogger.hpp"

ExosLogger::ExosLogger(exos_log_handle_t* logger, EXOS_LOG_LEVEL logLevel, EXOS_LOG_TYPE logType)
    : logger(logger)
    , logLevel(logLevel)
    , logType(logType)
{ 
}

void ExosLogger::flush() 
{
    switch(logLevel)
    {
        case EXOS_LOG_LEVEL_INFO:
            exos_log_info(logger, logType, const_cast<char*>(sstream.str().c_str()));
            break;
        case EXOS_LOG_LEVEL_DEBUG:
            exos_log_debug(logger, logType, const_cast<char*>(sstream.str().c_str()));
            break;
        case EXOS_LOG_LEVEL_ERROR:
            exos_log_error(logger, const_cast<char*>(sstream.str().c_str()));
            break;
        case EXOS_LOG_LEVEL_SUCCESS:
            exos_log_success(logger, logType, const_cast<char*>(sstream.str().c_str()));
            break;
        case EXOS_LOG_LEVEL_WARNING:
            exos_log_warning(logger, logType, const_cast<char*>(sstream.str().c_str()));
            break;
    }
    sstream.str(std::string());
    sstream.clear();
}
