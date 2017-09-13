Introduction
============

The LoRaWAN SKT project is designed to make a Maestro Wireless module of the S40 family work
 using the standard LoRaWAN protocol including the SKT specific requirements.
 
 The project is divided in several sub directories such as:
 
 * __inc__ contains Main application include files for global definitions
 * __src__ contains Main application source code
 * __LoRaWAN__ contains a shadowed subset of original LoRaMac-node-master directory cloned from github  
 and some hardware abstracted equivalent functions to make it work.
 * __EFM32_MMI__ contains some add-on helper functions to help abstracting the hardware used
 * __MCU__ contains the hardware specific source code that shall be adapted depending on the  
 current microcontroller in use
 * __FreeRTOS__ contains the original current version of FreeRTOS. To upgrade to the latest  
 version just copy the original FreeRTOS files in this directory and re-compile.
 * __LoRaMac-node-development__ is a clone of the original github implementation proposed by the  
 LoRa Alliance to upgrade to the latest version, just copy the new clone  
 into this directory and recompile. _Note that this whole directory is excluded from the  
 build._
 
 _NOTE:_ The commissioning sensitive information is stored in a reserved hidden page of the main  
Flash memory which will be totally erased upon debug connection if the processor debug  
register is locked. So no one will be able to read back commissioning information and keys.
However, the address of this reserved page might change depending on compilation type and it  
would be difficult to store the needed data at this specific moving address during factory process.  
For manufacturing settings, the commissioning data (and configuration data) will be stored in the User Data  
Flash memory region and upon device reset, this information will be copied to the "hidden" reserved  
page and all LoRaWAN information will be zeroed from the User Data Flash memory region.  
(see _USERDATA_ struct definition for more information)

_NOTE:_ By setting the macro [USE_SKT_FORMAT](@ref USE_SKT_FORMAT) to 1, the S47 device will set the  
[FLAG_USE_SKT_APP](@ref FLAG_USE_SKT_APP) device flag to use SKT/Daliworks LoRaWAN messages, otherwise  
a set of LoRaWAN messages that will simulate what the original S41 Wireless MBus would transfer as  
a reference example. Note that this format would be the format in use outside of SKT area.

The version of firmware differs depending on the expected attached sensor(s), using three different  
macros defined in the *global.h* file located in the **inc** directory:
 
###NODE_PULSE
Setting [NODE_PULSE](@ref NODE_PULSE) macro to 1 or 2, will build a firmware version for a 1 or 2 pulse input(s) device.
###NODE_TEMP
Setting [NODE_TEMP](@ref NODE_TEMP) macro to 1 or 2, will build a firmware version for device using 1 or 2 TSIC 306  
temperature sensor(s).
###NODE_HYGRO
Setting [NODE_HYGRO](@ref NODE_HYGRO) macro to 1, will build a firmware version for a device using a SHT75 temperature  
and humidity sensor.
###NODE_ANALOG
Setting [NODE_ANALOG](@ref NODE_ANALOG) macro to 1, will build a firmware version for an analog input (0-10V, 4-20mA) device.
 
Program Structure
=================

The program main function sets up 4 FreeRTOS tasks before giving control to the FreeRTOS scheduler:

##IDLE Task
This task is created automatically by FreeRTOS and used internally by FreeRTOS to go to Low Power mode.  
It's the lowest priority task of the system.

##SUPER Task
This task is created with a medium priority by the main program entry point, it's the main  
_supervisor_ task that is responsible of responding to every system events  
(Button, Periodic timer, RF interface, etc.). The various events are sent by system interrupts or  
other processes using either [DeviceSendEvent](@ref DeviceSendEvent) to place the event in back of the queue or  
[DevicePostEvent](@ref DevicePostEvent) to put the event in the front of the queue to be treated immediately.
All supervisor command functions are prefixed by the term **SUPERVISOR_**.
### System Events Handling
####IDLE_EVENT
This event is sent when the [DeviceWaitForEvent](@ref DeviceWaitForEvent) function returns because  
of a timeout. If [DeviceWaitForEvent](@ref DeviceWaitForEvent) function is called with *portMAX_DELAY*  
as a parameter then this event will never occur.
####BUTTON_EVENT
This event is sent when a magnet is put close to the magnetic sensor on the bottom of the device  
box. This event is triggered by a system interrupt. The current handling is to call the  
[DevicePerformKeypressTasks](@ref DevicePerformKeypressTasks) function to run a different function  
depending on the amount of time the magnet is hold close to the sensor.  
If the magnet detection is too short, then the [DevicePerformKeypressTasks](@ref DevicePerformKeypressTasks)  
function will return false. In every case, as soon as a magnet activity has been detected, a timer  
is reset to re-enable the pulse LED feedback (_see_ [PULSE_EVENT](@ref PULSE_EVENT)).
####ERROR_EVENT
This event is raised by program. Currently, the *SUPER Task* is ignoring this event.
####BATTERYLOW_EVENT
This event is raised by a system interrupt which sets the [DEVICE_LOW_BATTERY](@ref DEVICE_LOW_BATTERY)  
flag in the device status variable. Currently, the *SUPER Task* is ignoring this event.
####PULSE_EVENT
This event is triggered by a system interrupt as a pulse has been detected on a pulse input.  
The default behaviour is to quickly flash the LED to show a feedback to the user. However, to reduce  
consumption, this LED flash feedback will be invalidated after 5 minutes of magnet inactivity  
(_see_ [BUTTON_EVENT](@ref BUTTON_EVENT)).
####PERIODIC_EVENT
This event is triggered by the *PERIODIC Task* when the RFPeriod time has expired. The *SUPER Task*  
will make a sensor value reading and send a periodic standard LoRaWAN message to the server.
####PERIODIC_RESEND
This event is triggered by program to re-send the periodic standard LoRaWAN message to the server  
using the last sent value(s).
####RF_INDICATION
This event is triggered by the *LW_EVENT Task* upon reception of a LoRaWAN mac Indication event  
indicating that the server sent a message to the device on the LoRaWAN Network (a packet confirmation  
or a command to process). The *SUPER Task* calls [DEVICEAPP_ParseMessage](@ref DEVICEAPP_ParseMessage)  
function to handle this event.
####RF_MLME
This event is triggered by the *LW_EVENT Task* upon reception of a LoRaWAN mac Mlme event.
The *SUPER Task* calls [DEVICEAPP_ParseMlme](@ref DEVICEAPP_ParseMlme) function to handle this event.
####RF_ERROR_EVENT
This event is raised by program. Currently, the *SUPER Task* is ignoring this event.

##PERIODIC Task
This task is created with the highest priority and controlled by the _SUPER_ task.  
This task is responsible of sending a [PERIODIC_EVENT](@ref PERIODIC_EVENT) periodically.  
The period delay is defined by the _RFPeriod_ variable controlled by the _SUPER_ task.

##LW_EVENT Task
This task is created with a high priority by the _lorawan_task_ functions. The goal of this task is to wait for a  
LoRaWAN mac event feedback, store LoRaWAN mac event data structure for later use and dispatch  
a System Event that will be handled by the _SUPER_ task. This prevents the LoRaWAN event feedback  
routine from taking too much time with system interrupts disabled as this event is handled by an  
Interrupt Service Routine (ISR) and this would defeat the FreeRTOS kernel processing.
# S47
