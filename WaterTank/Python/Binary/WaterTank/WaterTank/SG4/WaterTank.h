/* Automation Studio generated header file */
/* Do not edit ! */
/* WaterTank  */

#ifndef _WATERTANK_
#define _WATERTANK_
#ifdef __cplusplus
extern "C" 
{
#endif

#include <bur/plctypes.h>

#ifndef _BUR_PUBLIC
#define _BUR_PUBLIC
#endif
#ifdef _SG3
		#include "ExData.h"
#endif
#ifdef _SG4
		#include "ExData.h"
#endif
#ifdef _SGC
		#include "ExData.h"
#endif


/* Datatypes and datatypes of function blocks */
typedef enum VALVE_STATE
{	VALVE_CLOSED = 0,
	VALVE_OPEN = 1
} VALVE_STATE;

typedef struct WaterTankHeaterConfig
{	float MaxTemperature;
	float MaxPower;
} WaterTankHeaterConfig;

typedef struct WaterTankHeaterStatus
{	float WaterTemperature;
	plcbit HeatingActive;
} WaterTankHeaterStatus;

typedef struct WaterTankStatus
{	plcbit LevelHigh;
	plcbit LevelLow;
	signed long FillValveDelay;
	signed long WaterLevel;
	struct WaterTankHeaterStatus Heater;
} WaterTankStatus;

typedef struct WaterTank
{	enum VALVE_STATE FillValve;
	plcbit EnableHeater;
	struct WaterTankHeaterConfig HeaterConfig;
	struct WaterTankStatus Status;
	signed long StatusDelay;
	unsigned long StatusUpdates;
} WaterTank;

typedef struct WaterTankCyclic
{
	/* VAR_INPUT (analog) */
	struct WaterTank* pWaterTank;
	/* VAR (analog) */
	unsigned long _Handle;
	/* VAR_INPUT (digital) */
	plcbit Enable;
	plcbit Start;
	/* VAR_OUTPUT (digital) */
	plcbit Connected;
	plcbit Operational;
	plcbit Error;
	/* VAR (digital) */
	plcbit _Start;
	plcbit _Enable;
} WaterTankCyclic_typ;



/* Prototyping of functions and function blocks */
_BUR_PUBLIC void WaterTankCyclic(struct WaterTankCyclic* inst);


#ifdef __cplusplus
};
#endif
#endif /* _WATERTANK_ */

