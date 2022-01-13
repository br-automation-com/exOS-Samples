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

typedef struct WaterTankInit
{
	/* VAR_OUTPUT (analog) */
	unsigned long Handle;
	/* VAR (analog) */
	unsigned char _state;
} WaterTankInit_typ;

typedef struct WaterTankCyclic
{
	/* VAR_INPUT (analog) */
	unsigned long Handle;
	struct WaterTank* pWaterTank;
	/* VAR (analog) */
	unsigned char _state;
	/* VAR_INPUT (digital) */
	plcbit Enable;
	plcbit Start;
	/* VAR_OUTPUT (digital) */
	plcbit Active;
	plcbit Error;
	plcbit Disconnected;
	plcbit Connected;
	plcbit Operational;
	plcbit Aborted;
} WaterTankCyclic_typ;

typedef struct WaterTankExit
{
	/* VAR_INPUT (analog) */
	unsigned long Handle;
	/* VAR (analog) */
	unsigned char _state;
} WaterTankExit_typ;



/* Prototyping of functions and function blocks */
_BUR_PUBLIC void WaterTankInit(struct WaterTankInit* inst);
_BUR_PUBLIC void WaterTankCyclic(struct WaterTankCyclic* inst);
_BUR_PUBLIC void WaterTankExit(struct WaterTankExit* inst);


#ifdef __cplusplus
};
#endif
#endif /* _WATERTANK_ */

