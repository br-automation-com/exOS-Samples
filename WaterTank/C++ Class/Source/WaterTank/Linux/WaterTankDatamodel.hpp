#ifndef _WATERTANKDATAMODEL_H_
#define _WATERTANKDATAMODEL_H_

#include <string>
#include <iostream>
#include <string.h>
#include <functional>
#include "WaterTankDataset.hpp"

class WaterTankDatamodel
{
private:
    exos_datamodel_handle_t datamodel = {};
    std::function<void()> _onConnectionChange = [](){};

    void datamodelEvent(exos_datamodel_handle_t *datamodel, const EXOS_DATAMODEL_EVENT_TYPE event_type, void *info);
    static void _datamodelEvent(exos_datamodel_handle_t *datamodel, const EXOS_DATAMODEL_EVENT_TYPE event_type, void *info) {
        WaterTankDatamodel* inst = static_cast<WaterTankDatamodel*>(datamodel->user_context);
        inst->datamodelEvent(datamodel, event_type, info);
    }

public:
    WaterTankDatamodel();
    void process();
    void connect();
    void disconnect();
    void setOperational();
    int getNettime();
    void onConnectionChange(std::function<void()> f) {_onConnectionChange = std::move(f);};

    bool isOperational = false;
    bool isConnected = false;
    EXOS_CONNECTION_STATE connectionState = EXOS_STATE_DISCONNECTED;

    WaterTankLogger log;

    WaterTankDataset<VALVE_STATE> FillValve;
    WaterTankDataset<bool> EnableHeater;
    WaterTankDataset<WaterTankHeaterConfig> HeaterConfig;
    WaterTankDataset<WaterTankStatus> Status;

    ~WaterTankDatamodel();
};

#endif
