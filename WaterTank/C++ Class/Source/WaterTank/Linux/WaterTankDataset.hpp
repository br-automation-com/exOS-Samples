#ifndef _WATERTANKDATASET_H_
#define _WATERTANKDATASET_H_

#include <string>
#include <iostream>
#include <string.h>
#include <functional>

extern "C" {
    #include "exos_watertank.h"
}

#include "WaterTankLogger.hpp"
#define exos_assert_ok(_plog_,_exp_)                                                                                                    \
    do                                                                                                                                  \
    {                                                                                                                                   \
        EXOS_ERROR_CODE err = _exp_;                                                                                                    \
        if (EXOS_ERROR_OK != err)                                                                                                       \
        {                                                                                                                               \
            _plog_->error << "Error in file " << __FILE__ << ":" << __LINE__ << std::endl;                                               \
            _plog_->error << #_exp_ " returned " << err << " (" << exos_get_error_string(err) << ") instead of expected 0" << std::endl; \
        }                                                                                                                               \
    } while (0)

template <typename T>
class WaterTankDataset
{
private:
    exos_dataset_handle_t dataset = {};
    WaterTankLogger* log;
    std::function<void()> _onChange = [](){};
    void datasetEvent(exos_dataset_handle_t *dataset, EXOS_DATASET_EVENT_TYPE event_type, void *info) {
        switch (event_type)
        {
            case EXOS_DATASET_EVENT_UPDATED:
                log->verbose << "dataset " << dataset->name << " updated! latency (us):" << (exos_datamodel_get_nettime(dataset->datamodel) - dataset->nettime) << std::endl;
                nettime = dataset->nettime;
                _onChange();
                break;
            case EXOS_DATASET_EVENT_PUBLISHED:
                log->verbose << "dataset " << dataset->name << "  published to local server for distribution! send buffer free:" << dataset->send_buffer.free << std::endl;
                break;
            case EXOS_DATASET_EVENT_DELIVERED:
                log->verbose << "dataset " << dataset->name << " delivered to remote server for distribution! send buffer free:" << dataset->send_buffer.free << std::endl;
                break;
            case EXOS_DATASET_EVENT_CONNECTION_CHANGED:
                log->info << "dataset " << dataset->name << " changed state to " << exos_get_state_string(dataset->connection_state) << std::endl;
                
                switch (dataset->connection_state)
                {
                    case EXOS_STATE_DISCONNECTED:
                        break;
                    case EXOS_STATE_CONNECTED:
                        break;
                    case EXOS_STATE_OPERATIONAL:
                        break;
                    case EXOS_STATE_ABORTED:
                        log->error << "dataset " << dataset->name << " error " << dataset->error << " (" << exos_get_error_string(dataset->error) << ") occured" << std::endl;
                        break;
                }
                break;
        }
    }
    static void _datasetEvent(exos_dataset_handle_t *dataset, EXOS_DATASET_EVENT_TYPE event_type, void *info) {
        WaterTankDataset* inst = static_cast<WaterTankDataset*>(dataset->user_context);
        inst->datasetEvent(dataset, event_type, info);
    }

public:
    WaterTankDataset() {};
    
    T value;
    int nettime;
    void init(exos_datamodel_handle_t *datamodel, const char *browse_name, WaterTankLogger* _log) {
        log = _log;
        exos_assert_ok(log, exos_dataset_init(&dataset, datamodel, browse_name, &value, sizeof(value)));
        dataset.user_context = this;
    };
    void connect(EXOS_DATASET_TYPE type) {
        exos_assert_ok(log, exos_dataset_connect(&dataset, type, &WaterTankDataset::_datasetEvent));
    };
    void publish() {
        exos_dataset_publish(&dataset);
    };
    void onChange(std::function<void()> f) {_onChange = std::move(f);};
    
    ~WaterTankDataset() {
        exos_assert_ok(log, exos_dataset_delete(&dataset));
    };
};

#endif
