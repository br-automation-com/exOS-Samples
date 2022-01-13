#define EXOS_STATIC_INCLUDE
#include "WaterTankDatamodel.hpp"

WaterTankDatamodel::WaterTankDatamodel()
    : log("gWaterTank_0")
{
    log.success << "starting gWaterTank_0 application.." << std::endl;

    exos_assert_ok((&log), exos_datamodel_init(&datamodel, "WaterTank_0", "gWaterTank_0"));
    datamodel.user_context = this;

    FillValve.init(&datamodel, "FillValve", &log);
    EnableHeater.init(&datamodel, "EnableHeater", &log);
    HeaterConfig.init(&datamodel, "HeaterConfig", &log);
    Status.init(&datamodel, "Status", &log);
}

void WaterTankDatamodel::connect() {
    exos_assert_ok((&log), exos_datamodel_connect_watertank(&datamodel, &WaterTankDatamodel::_datamodelEvent));

    FillValve.connect((EXOS_DATASET_TYPE)EXOS_DATASET_SUBSCRIBE);
    EnableHeater.connect((EXOS_DATASET_TYPE)EXOS_DATASET_SUBSCRIBE);
    HeaterConfig.connect((EXOS_DATASET_TYPE)(EXOS_DATASET_PUBLISH+EXOS_DATASET_SUBSCRIBE));
    Status.connect((EXOS_DATASET_TYPE)EXOS_DATASET_PUBLISH);
}

void WaterTankDatamodel::disconnect() {
    exos_assert_ok((&log), exos_datamodel_disconnect(&datamodel));
}

void WaterTankDatamodel::setOperational() {
    exos_assert_ok((&log), exos_datamodel_set_operational(&datamodel));
}

void WaterTankDatamodel::process() {
    exos_assert_ok((&log), exos_datamodel_process(&datamodel));
    log.process();
}

int WaterTankDatamodel::getNettime() {
    return exos_datamodel_get_nettime(&datamodel);
}

void WaterTankDatamodel::datamodelEvent(exos_datamodel_handle_t *datamodel, const EXOS_DATAMODEL_EVENT_TYPE event_type, void *info) {
    switch (event_type)
    {
    case EXOS_DATAMODEL_EVENT_CONNECTION_CHANGED:
        log.info << "application changed state to " << exos_get_state_string(datamodel->connection_state) << std::endl;
        connectionState = datamodel->connection_state;
        _onConnectionChange();
        switch (datamodel->connection_state)
        {
        case EXOS_STATE_DISCONNECTED:
            isOperational = false;
            isConnected = false;
            break;
        case EXOS_STATE_CONNECTED:
            isConnected = true;
            break;
        case EXOS_STATE_OPERATIONAL:
            log.success << "gWaterTank_0 operational!" << std::endl;
            isOperational = true;
            break;
        case EXOS_STATE_ABORTED:
            log.error << "application error " << datamodel->error << " (" << exos_get_error_string(datamodel->error) << ") occured" << std::endl;
            isOperational = false;
            isConnected = false;
            break;
        }
        break;
    case EXOS_DATAMODEL_EVENT_SYNC_STATE_CHANGED:
        break;

    default:
        break;

    }
}

WaterTankDatamodel::~WaterTankDatamodel()
{
    exos_assert_ok((&log), exos_datamodel_delete(&datamodel));
}
