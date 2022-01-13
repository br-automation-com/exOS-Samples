#ifndef _WATERTANK_LOGGER_H_
#define _WATERTANK_LOGGER_H_

#include <iostream>
#include <sstream>
#include <string>

extern "C" {
    #include "exos_log.h"
}

class ExosLogger
{
private:
    exos_log_handle_t* logger;
    EXOS_LOG_LEVEL logLevel;
    EXOS_LOG_TYPE logType;
    std::stringstream sstream;
public:
    typedef std::ostream&  (*ManipFn)(std::ostream&);
    typedef std::ios_base& (*FlagsFn)(std::ios_base&);

    ExosLogger(exos_log_handle_t* logger, EXOS_LOG_LEVEL logLevel, EXOS_LOG_TYPE logType);
    
    template<class T>  // int, double, strings, etc
        ExosLogger& operator<<(const T& output)
    {
        sstream << output;
        return *this;
    }

    ExosLogger& operator<<(ManipFn manip) /// endl, flush, setw, setfill, etc.
    { 
        manip(sstream);

        if (manip == static_cast<ManipFn>(std::flush)
            || manip == static_cast<ManipFn>(std::endl ) )
            this->flush();

        return *this;
    }

    ExosLogger& operator<<(FlagsFn manip) /// setiosflags, resetiosflags
    {
        manip(sstream);
        return *this;
    }
    
    void flush();

    ExosLogger();
};

class WaterTankLogger
{
public:
    WaterTankLogger(std::string name)
        : info(&logger, EXOS_LOG_LEVEL_INFO, EXOS_LOG_TYPE_USER)
        , warning(&logger, EXOS_LOG_LEVEL_WARNING, EXOS_LOG_TYPE_USER)
        , error(&logger, EXOS_LOG_LEVEL_ERROR, EXOS_LOG_TYPE_USER)
        , debug(&logger, EXOS_LOG_LEVEL_DEBUG, EXOS_LOG_TYPE_USER)
        , verbose(&logger, EXOS_LOG_LEVEL_WARNING, EXOS_LOG_TYPE(EXOS_LOG_TYPE_USER+EXOS_LOG_TYPE_VERBOSE))
        , success(&logger, EXOS_LOG_LEVEL_SUCCESS, EXOS_LOG_TYPE_USER)
    {
        exos_log_init(&logger, name.c_str());
    };
    void process() {
        exos_log_process(&logger);
    }
    ~WaterTankLogger() {
        exos_log_delete(&logger);
    };
    ExosLogger info;
    ExosLogger warning;
    ExosLogger error;
    ExosLogger debug;
    ExosLogger verbose;
    ExosLogger success;
private:
    exos_log_handle_t logger = {};
};

#endif
