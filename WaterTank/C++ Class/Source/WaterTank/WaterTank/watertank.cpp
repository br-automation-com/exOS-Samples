#include <string.h>
#include <stdbool.h>
#include "WaterTankDatamodel.hpp"

/* datamodel features:

main methods:
    watertank->connect()
    watertank->disconnect()
    watertank->process()
    watertank->setOperational()
    watertank->dispose()
    watertank->getNettime() : (int32_t) get current nettime

void(void) user lambda callback:
    watertank->onConnectionChange([&] () {
        // watertank->connectionState ...
    })

boolean values:
    watertank->isConnected
    watertank->isOperational

logging methods:
    watertank->log.error << "some value:" << 1 << std::endl;
    watertank->log.warning << "some value:" << 1 << std::endl;
    watertank->log.success << "some value:" << 1 << std::endl;
    watertank->log.info << "some value:" << 1 << std::endl;
    watertank->log.debug << "some value:" << 1 << std::endl;
    watertank->log.verbose << "some value:" << 1 << std::endl;

dataset FillValve:
    watertank->FillValve.publish()
    watertank->FillValve.value : (VALVE_STATE)  actual dataset values

dataset EnableHeater:
    watertank->EnableHeater.publish()
    watertank->EnableHeater.value : (bool)  actual dataset value

dataset HeaterConfig:
    watertank->HeaterConfig.publish()
    watertank->HeaterConfig.onChange([&] () {
        watertank->HeaterConfig.value ...
    })
    watertank->HeaterConfig.nettime : (int32_t) nettime @ time of publish
    watertank->HeaterConfig.value : (WaterTankHeaterConfig)  actual dataset values

dataset Status:
    watertank->Status.onChange([&] () {
        watertank->Status.value ...
    })
    watertank->Status.nettime : (int32_t) nettime @ time of publish
    watertank->Status.value : (WaterTankStatus)  actual dataset values
*/


_BUR_PUBLIC void WaterTankInit(struct WaterTankInit *inst)
{
    WaterTankDatamodel* watertank = new WaterTankDatamodel();
    if (NULL == watertank)
    {
        inst->Handle = 0;
        return;
    }
    inst->Handle = (UDINT)watertank;
}

_BUR_PUBLIC void WaterTankCyclic(struct WaterTankCyclic *inst)
{
    // return error if reference to structure is not set on function block
    if(NULL == (void*)inst->Handle || NULL == inst->pWaterTank)
    {
        inst->Operational = false;
        inst->Connected = false;
        inst->Error = true;
        return;
    }
    WaterTankDatamodel* watertank = static_cast<WaterTankDatamodel*>((void*)inst->Handle);
    if (inst->Enable && !inst->_Enable)
    {
        watertank->onConnectionChange([&] () {
            if (watertank->connectionState == EXOS_STATE_OPERATIONAL) {    
                watertank->log.success << "WaterTank operational!" << std::endl;
            }
        });

        watertank->HeaterConfig.onChange([&] () {
            watertank->log.success << "dataset config changed, check your new datasets" << std::endl;
            memcpy(&inst->pWaterTank->HeaterConfig, &watertank->HeaterConfig.value, sizeof(inst->pWaterTank->HeaterConfig));
        });
        watertank->Status.onChange([&] () {
            inst->pWaterTank->StatusDelay = watertank->getNettime() - watertank->Status.nettime;
            inst->pWaterTank->StatusUpdates++;

            if (watertank->Status.value.LevelHigh > inst->pWaterTank->Status.LevelHigh) {
                watertank->log.success << "Level High reached" << std::endl;
            }
            if (watertank->Status.value.LevelLow > inst->pWaterTank->Status.LevelLow) {
                watertank->log.success << "Level Low reached" << std::endl;
            }
            if (watertank->Status.value.Heater.WaterTemperature != inst->pWaterTank->Status.Heater.WaterTemperature) {
                watertank->log.verbose << "Water temperature is now: "<< watertank->Status.value.Heater.WaterTemperature <<" nettime delay: " << inst->pWaterTank->StatusDelay << " us" << std::endl;
            }
    
            memcpy(&inst->pWaterTank->Status, &watertank->Status.value, sizeof(inst->pWaterTank->Status));
        });
        watertank->connect();
    }
    if (!inst->Enable && inst->_Enable)
    {
        watertank->disconnect();
    }
    inst->_Enable = inst->Enable;

    if(inst->Start && !inst->_Start && watertank->isConnected)
    {
        watertank->setOperational();
        inst->_Start = inst->Start;
    }
    if(!inst->Start)
    {
        inst->_Start = false;
    }

    //trigger callbacks
    watertank->process();

    if (watertank->isConnected)
    {
        //publish the FillValve dataset as soon as there are changes
        if (0 != memcmp(&inst->pWaterTank->FillValve, &watertank->FillValve.value, sizeof(watertank->FillValve.value)))
        {
            memcpy(&watertank->FillValve.value, &inst->pWaterTank->FillValve, sizeof(watertank->FillValve.value));
            watertank->FillValve.publish();
        }
        //publish the EnableHeater dataset as soon as there are changes
        if (inst->pWaterTank->EnableHeater != watertank->EnableHeater.value)
        {
            watertank->EnableHeater.value = inst->pWaterTank->EnableHeater;
            watertank->EnableHeater.publish();
        }
        //publish the HeaterConfig dataset as soon as there are changes
        if (0 != memcmp(&inst->pWaterTank->HeaterConfig, &watertank->HeaterConfig.value, sizeof(watertank->HeaterConfig.value)))
        {
            memcpy(&watertank->HeaterConfig.value, &inst->pWaterTank->HeaterConfig, sizeof(watertank->HeaterConfig.value));
            watertank->HeaterConfig.publish();
        }
    }

    inst->Connected = watertank->isConnected;
    inst->Operational = watertank->isOperational;
}

_BUR_PUBLIC void WaterTankExit(struct WaterTankExit *inst)
{
    WaterTankDatamodel* watertank = static_cast<WaterTankDatamodel*>((void*)inst->Handle);
    delete watertank;
}

