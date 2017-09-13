/*******************************************************************
**                                                                **
** ADC dependent system functions                                 **
**                                                                **
*******************************************************************/

#ifndef __ADC_H__
#define __ADC_H__

#include <system.h>
/** \addtogroup MMI MyMeterInfo add-on functions
 *  @{
 */

/*!
 * @brief ADC channel number abstraction
 */
typedef enum {
	ADCChannel0 = 0,   //!< ADCChannel0
	ADCChannel1 = 1,   //!< ADCChannel1
	ADCChannel2 = 2,   //!< ADCChannel2
	ADCChannel3 = 3,   //!< ADCChannel3
	ADCChannel4 = 4,   //!< ADCChannel4
	ADCChannel5 = 5,   //!< ADCChannel5
	ADCChannel6 = 6,   //!< ADCChannel6
	ADCChannel7 = 7,   //!< ADCChannel7
	ADCChannelTemp = 8,//!< ADCChannelTemp
	ADCChannelVDD = 9  //!< ADCChannelVDD
}ADC_CHANNEL;
/*!
 * @brief ADC trigger level detection
 */
typedef enum {
	ADCTriggerLevelNone = 0,//!< ADCTriggerLevelNone
	ADCTriggerRising = 1,   //!< ADCTriggerRising
	ADCTriggerFalling = 2,  //!< ADCTriggerFalling
	ADCTriggerBoth = 3      //!< ADCTriggerBoth
}ADC_TRIGGER_OUTPUT;
/*!
 * @brief ADC comparator reference
 */
typedef enum {
	ADCLowReference, //!< ADCLowReference
	ADCMidReference, //!< ADCMidReference
	ADCHighReference,//!< ADCHighReference
	ADCVCCReference, //!< ADCVCCReference
	ADCExtReference  //!< ADCExtReference
}ADC_REFERENCE;
/*!
 * ADC sampling cycle time for each sample
 */
typedef enum {
	ADCSamplingTime1,  //!< 1 clock cycle per sample
	ADCSamplingTime2,  //!< 2 clock cycles per sample
	ADCSamplingTime4,  //!< 4 clock cycles per sample
	ADCSamplingTime8,  //!< 8 clock cycles per sample
	ADCSamplingTime16, //!< 16 clock cycles per sample
	ADCSamplingTime32, //!< 32 clock cycles per sample
	ADCSamplingTime64, //!< 64 clock cycles per sample
	ADCSamplingTime128,//!< 128 clock cycles per sample
	ADCSamplingTime256 //!< 256 clock cycles per sample
}ADC_SAMPLINGTIME;

/*!
 * @brief User defined ADC complete IRQ handler
 * @param[in] ADCNum	ADC port number (depends on microcontroller type)
 * @param[in] value		ADC converted value
 */
void INTAD_Handler(int ADCNum, unsigned long value);
/*!
 * @brief User defined ADC trigger IRQ handler
 * @param[in] ADCNum	ADC port number (depends on microcontroller type)
 * @param[in] outLevel  ADC trigger level detected
 */
void INTADCTrigger_Handler(int ADCNum, ADC_TRIGGER_OUTPUT outLevel);	// User defined interrupt

/*!
 * @brief Open an ADC port to perform either single or recurrent ADC conversions
 * @param[in] ADCNum	ADC port number (depends on microcontroller type)
 * @param[in] location	ADC port location (depends on microcontroller type)
 * @param[in] channel	ADC channel (depends on microcontroller type)
 * @param[in] ref		ADC voltage reference to use
 * @param[in] cycles	Clock cycles per ADC sampling
 * @param[in] ADCTime_us ADC sampling duration
 */
void ADCOpen(int ADCNum, int location, ADC_CHANNEL channel, ADC_REFERENCE ref, ADC_SAMPLINGTIME cycles, int ADCTime_us);
/*!
 * @brief Open an ADC port to perform either single or recurrent ADC conversions
 * @param[in] ADCNum	ADC port number (depends on microcontroller type)
 * @param[in] location	ADC port location (depends on microcontroller type)
 * @param[in] posChannel ADC positive channel (depends on microcontroller type)
 * @param[in] negChannel ADC negative channel (depends on microcontroller type)
 * @param[in] ref		ADC voltage reference to use
 * @param[in] cycles	Clock cycles per ADC sampling
 * @param[in] ADCTime_us ADC sampling duration
 */
void ADCOpenDifferential(int ADCNum,int location,ADC_CHANNEL posChannel, ADC_CHANNEL negChannel, ADC_REFERENCE ref,ADC_SAMPLINGTIME cycles, int ADCTime_us);
/*!
 * @brief Close an ADC port to stop doing conversions and disable channel to save power
 * @param[in] ADCNum	ADC port number (depends on microcontroller type)
 * @param[in] channel	ADC channel (depends on microcontroller type)
 */
void ADCClose(int ADCNum, ADC_CHANNEL channel);
/*!
 * @brief Return ADC voltage reference in use in mV
 * @param[in] ADCNum	ADC port number (depends on microcontroller type)
 * @return reference voltage in mV
 */
int ADCGetReference(int ADCNum);
/*!
 * @brief Return the maximum possible value returned by an ADC conversion
 * @param[in] ADCNum	ADC port number (depends on microcontroller type)
 * @return maximum possible value
 */
int ADCGetFullScale(int ADCNum);
/*!
 * @ brief Perform a single synchronous ADC conversion and returns result directly
 * @param[in] ADCNum	ADC port number (depends on microcontroller type)
 * @return conversion result
 */
unsigned long ADCConvertOnce(int ADCNum);
/*!
 * @brief Start asynchronous ADC conversions
 * @param[in] ADCNum	ADC port number (depends on microcontroller type)
 * @note The user defined IRQ handler will be called after each conversion
 */
void ADCStartConvert(int ADCNum);
/*!
 * @brief Stop asynchronous ADC conversions
 * @param[in] ADCNum	ADC port number (depends on microcontroller type)
 */
void ADCStopConvert(int ADCNum);

// Trigger Level is in 1/64 of VDD increments
/*!
 * @brief Perform asynchronous ADC conversions and compare result to a predefined trigger level.
 * @param[in] ADCNum	ADC port number (depends on microcontroller type)
 * @param[in] location	ADC port location (depends on microcontroller type)
 * @param[in] channel	ADC channel (depends on microcontroller type)
 * @param[in] triggerLevel  Trigger level to compare with expressed in 1/64 increments of VDD
 * @param[in] outLevel  Type of trigger comparison to react to
 * @note The user defined IRQ handler will be called upon valid trigger comparison
 */
void ADCTriggerStart(int ADCNum, int location, ADC_CHANNEL channel, int triggerLevel, ADC_TRIGGER_OUTPUT outLevel);
/*!
 * @brief Enable/Disable asynchronous ADC trigger level comparisons
 * @param[in] ADCNum	ADC port number (depends on microcontroller type)
 * @param[in] enable	true to enable, false to disable asynchronous ADC trigger comparisons
 */
void ADCTriggerEnable(int ADCNum, BOOL enable);
/*!
 * brief Get current state of ADC trigger level
 * @param[in] ADCNum	ADC port number (depends on microcontroller type)
 * @return current state of ADC trigger level
 */
ADC_TRIGGER_OUTPUT ADCTriggerGetOutputLevel(int ADCNum);
/*!
 * Stop asynchronous ADC trigger level comparisons and disable channel to save power
 * @param[in] ADCNum	ADC port number (depends on microcontroller type)
 * @param[in] channel	ADC channel (depends on microcontroller type)
 */
void ADCTriggerStop(int ADCNum, ADC_CHANNEL channel);

/** }@ */
#endif
