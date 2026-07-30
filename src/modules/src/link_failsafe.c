#define DEBUG_MODULE "LINK_FAILSAFE"

#include "FreeROTS.h"
#include "task.h"

#include "debug.h"
#include "crtp.h"
#include "supervisor.h"
#include "commander.h"
#include "param.h"
#include "log.h"
#include "static_mem.h"

// define failsafe parameters
static uint8_t  failsafeEnable = 1;
static uint32_t gracePeriod = 1500;     //ms, must be greater than radio activity timeout
static float    hoverThrust = 28000;    //PWM
static uint32_t rampTime = 3000;        //ms, time to ramp motors to 0
static uint32_t postLandTime = 1500;    //ms

// initialize other required variables
static uint32_t linkLostAt;
static uint32_t startTick;
static uint32_t failsafeTriggered;
static float    currentThrust;

// define failsafe states
typedef enum {
    FAILSAFE_IDLE,
    FAILSAFE_DISCONNECT,
    FAILSAFE_RAMP_DOWN,
    FAILSAFE_LANDED,
} failsafeState_t;

// initialize failsafe state 
static failsafeState_t state = FAILSAFE_IDLE;

// define failsafe 'tick' rate
#define FAILSAFE_PERIOD 50 // ms

STATIC_MEM_TASK_ALLOC(failsafeTask, 2*configMINIMAL_STACK_SIZE);

// function to send ramp down setpoints
static void rampDown(float thrust)
{
    setpoint_t sp;
    memset(&sp, 0, sizeof(sp));

    // disable position and velocity controllers
    sp.mode.x = modeDisable;
    sp.mode.y = modeDisable;
    sp.mode.z = modeDisable;
    sp.mode.roll = modeAbs;
    sp.mode.pitch = modeAbs;
    sp.mode.yaw = modeVelocity;

    // create 'level' setpoints
    sp.attitude.roll = 0.0f;
    sp.attitude.pitch = 0.0f;
    sp.attitudeRate.yaw = 0.0f;

    // send thrust command
    sp.thrust = thrust;
    commanderSetSetpoint(&sp, COMMANDER_PRIORITY_EXTRX)
}

// function to execute failsafe
static void linkFailsafeExecutor(void *param)
{
    vTaskDelay(M2T(2000));  // delay runtime for boot up

    while (1) {
        vTaskDelay(M2T(FAILSAFE_PERIOD));

        // if failsafe not enabled, set the state to idle
        if (!failsafeEnable) {
            state = FAILSAFE_IDLE;
            continue;
        }

        // poll crtp connection
        bool isConnected = crtpIsConnected();

        switch (state) {
            // if failsafe is idle
            case FAILSAFE_IDLE:
                // check for connection loss and record tick at which loss occurs
                if (!isConnected) {
                    linkLostAt = xTaskGetTickCount();
                    state = FAILSAFE_DISCONNECT;
                }
                break;

            // if quadcopter has disconnected
            case FAILSAFE_DISCONNECT:
                // check for reconnection
                if (connected) {
                    state = FAILSAFE_IDLE;
                }
                //else if ()//???
                // if quadcopter is still flying, start winding down motors
                else if (supervisorIsFlying()){
                    DEBUG_PRINT("Connection lost, landing...\n");
                    startTick = xTaskGetTickCount();
                    currentThrust = hoverThrust;
                    failsafeTriggered = 1;
                    state = FAILSAFE_RAMP_DOWN;
                }
                break;

            // if quadcopter is landing
            case FAILSAFE_RAMP_DOWN: {
                uint32_t totalTime = xTaskGetTickCount() - startTick;
                // if more time has passed than specified for the ramp, send 0 commands
                if (totalTime >= M2T(rampTime)) {
                    currentThrust = 0.0f;
                    rampDown(0.0f);
                    startTick = xTaskGetTickCount(); 
                    state = FAILSAFE_LANDED;
                }
                // otherwise, send a thrust setpoint to the quadcopter
                else{
                    float percentageOfRamp = 1.0f - ((float)totalTime / (float)M2T(rampTime));
                    currentThrust = hoverThrust * percentageOfRamp;
                    rampDown(currentThrust);
                }
                break;
            }

            // if quadcopter has landed
            case FAILSAFE_LANDED:
                // keep sending 0 to motors to make sure supervisor will change to 'not flying'
                rampDown(0.0f);
                // if we have waited sufficiently long, reset failsafe state
                if ((xTaskGetTickCount() - startTick) > M2T(postLandTime)) {
                    failsafeTriggered = 0;
                    state = FAILSAFE_IDLE;
                }
                break;
        }
    }
}

void failsafeInit(void)
{
    STATIC_MEME_TASK_CREATE(failsafeTask, failsafeTask, "FAILSAFE", NULL, 2);
}

// add parameters and link to script variables
PARAM_GROUP_START(failsafe)
PARAM_ADD(PARAM_UINT8, enable, &failsafeEnable)
PARAM_ADD(PARAM_UINT32, gracePeriod, &gracePeriod)
PARAM_ADD(PARAM_FLOAT, defaultThrust, &hoverThrust)
PARAM_ADD(PARAM_UINT32, timeToLand, &rampTime)
PARAM_ADD(PARAM_UINT32, waitTime, &postLandTime)
PARAM_GROUP_STOP(failsafe)

LOG_GROUP_START(failsafe)
LOG_ADD(LOG_UINT8, triggered, &failsafeTriggered)
LOG_ADD(LOG_FLOAT, thrust, &currentThrust)
LOG_GROUP_STOP(failsafe)