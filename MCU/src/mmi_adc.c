/*******************************************************************
**                                                                **
** ADC dependent system functions                                 **
**                                                                **
*******************************************************************/
/** \addtogroup MMI MyMeterInfo add-on functions
 *  @{
 */

#include "mmi_adc.h"
#include "system.h"
#include <em_cmu.h>
#include <em_adc.h>
#include <em_acmp.h>

#ifdef ADC_PRESENT
static const ADC_Init_TypeDef ADCInit = ADC_INIT_DEFAULT;
static const ADC_InitSingle_TypeDef ADCsInit = ADC_INITSINGLE_DEFAULT;
#ifndef EXCLUDE_DEFAULT_ADC_IRQ_HANDLER
__interrupt_handler  __attribute__((used)) void ADC0_IRQHandler(void) {
#if ADC_COUNT > 0
	if (ADC0->IF & ADC_IF_SINGLE) {
#ifdef ADC_IFC_SINGLE
		ADC0->IFC = ADC_IFC_SINGLE;
#endif
		INTAD_Handler(0,ADC0->SINGLEDATA);
	}
#endif
#if ADC_COUNT > 1
	if (ADC1->IF & ADC_IF_SINGLE) {
#ifdef ADC_IFC_SINGLE
		ADC1->IFC = ADC_IFC_SINGLE;
#endif
		INTAD_Handler(1,ADC1->SINGLEDATA);
	}
#endif
#if ADC_COUNT > 2
	if (ADC2->IF & ADC_IF_SINGLE) {
#ifdef ADC_IFC_SINGLE
		ADC2->IFC = ADC_IFC_SINGLE;
#endif
		INTAD_Handler(2,ADC2->SINGLEDATA);
	}
#endif
}
#endif
#endif
#ifdef ACMP_PRESENT
static const ACMP_Init_TypeDef ACMPInit = ACMP_INIT_DEFAULT;

#ifndef EXCLUDE_DEFAULT_ACMP_IRQ_HANDLER
__interrupt_handler  __attribute__((used)) void ACMP0_IRQHandler(void) {
	if (ACMP0->IF & ACMP_IF_EDGE) {
		ACMP0->IFC = ACMP_IFC_EDGE;
		INTADCTrigger_Handler(0,(ACMP0->STATUS & ACMP_STATUS_ACMPOUT) ? ADCTriggerRising : ADCTriggerFalling);
	}
#if ACMP_COUNT > 1
	if (ACMP1->IF & ACMP_IF_EDGE) {
		ACMP1->IFC = ACMP_IFC_EDGE;
		INTADCTrigger_Handler(1,(ACMP1->STATUS & ACMP_STATUS_ACMPOUT) ? ADCTriggerRising : ADCTriggerFalling);
	}
#endif
#if ACMP_COUNT > 2
	if (ACMP2->IF & ACMP_IF_EDGE) {
		ACMP2->IFC = ACMP_IFC_EDGE;
		INTADCTrigger_Handler(2,(ACMP2->STATUS & ACMP_STATUS_ACMPOUT) ? ADCTriggerRising : ADCTriggerFalling);
	}
#endif
}
#endif
#endif

/***** A/D convert interrupt vector ******/
__weak void INTAD_Handler(int ADCNum, unsigned long value) {
	(void)ADCNum;
	(void)value;
}
__weak void INTADCTrigger_Handler(int ADCNum, ADC_TRIGGER_OUTPUT level) {
	(void)ADCNum;
	(void)level;
}
#ifdef ADC_PRESENT
static ADC_TypeDef* _GetADC(int ADCNum) {
#if ADC_COUNT > 0
	if (ADCNum == 0) return ADC0;
#if ADC_COUNT > 1
	else if (ADCNum == 1) return ADC1;
#endif
#if ADC_COUNT > 2
	else if (ADCNum == 2) return ADC2;
#endif
#else
	(void)ADCNum;
#endif
	return NULL;
}

static void ADCPowerEnable(int ADCNum, int enable) {
#ifdef ADC_PRESENT
	if (ADCNum == 0)
		CMU_ClockEnable(cmuClock_ADC0, enable);
#if ADC_COUNT > 1
	else if (ADCNum == 1)
		CMU_ClockEnable(cmuClock_ADC1, enable);
#endif
#if ADC_COUNT > 2
	else if (ADCNum == 2)
		CMU_ClockEnable(cmuClock_ADC2, enable);
#endif
#else
	(void)ADCNum;
	(void)enable;
#endif
}
#endif

#if ADC_COUNT > 0
static int ADCReference(ADC_REFERENCE ref) {
	switch(ref) {
	case ADCLowReference: return ADC_SINGLECTRL_REF_1V25;
	case ADCMidReference: return ADC_SINGLECTRL_REF_2V5;
	case ADCHighReference: return ADC_SINGLECTRL_REF_5V;
	case ADCVCCReference: return ADC_SINGLECTRL_REF_VDD;
	case ADCExtReference: return ADC_SINGLECTRL_REF_EXTSINGLE;
	}
	return 0;
}
#endif

void ADCOpen(int ADCNum, int location, ADC_CHANNEL Channel, ADC_REFERENCE ref,ADC_SAMPLINGTIME sampling, int ADCTime_us)
{
#if ADC_COUNT > 0
	ADC_TypeDef* adc = _GetADC(ADCNum);
	if (adc) {
		ADCPowerEnable(ADCNum,true);
		ADC_Init(adc, &ADCInit);
		ADC_InitSingle(adc, &ADCsInit);
		adc->CTRL &= ~_ADC_SINGLECTRL_REP_MASK | _ADC_CTRL_PRESC_MASK;	// Make sure single conversation only
//		adc->CTRL |= ADC_CTRL_OVSRSEL_X16;
		adc->SINGLECTRL &= ~(_ADC_SINGLECTRL_DIFF_MASK | _ADC_SINGLECTRL_AT_MASK | _ADC_SINGLECTRL_REF_MASK);
		adc->SINGLECTRL |= (sampling << _ADC_SINGLECTRL_AT_SHIFT);
		if (Channel == ADCChannelVDD) {
			adc->SINGLECTRL |= ADC_SINGLECTRL_POSSEL_AVDD | ADC_SINGLECTRL_REF_5V;
		} else if (Channel == ADCChannelTemp) {
			adc->SINGLECTRL |= ADC_SINGLECTRL_POSSEL_TEMP | ADC_SINGLECTRL_REF_2V5;
		} else {
			adc->SINGLECTRL |= ((Channel + (location * 32)) << _ADC_SINGLECTRL_POSSEL_SHIFT) | ADC_SINGLECTRL_NEGSEL_VSS | ADCReference(ref);
		}
		int adcClock = (adc->CTRL & _ADC_CTRL_TIMEBASE_MASK);
		int div = (((0x01 << sampling) + 12 )
				* ((( adc->SINGLECTRL & _ADC_SINGLECTRL_RES_MASK) == ADC_SINGLECTRL_RES_OVS) ? (0x02 << ((adc->CTRL & _ADC_CTRL_OVSRSEL_MASK) >> _ADC_CTRL_OVSRSEL_SHIFT)) : 1));
		do {
			int adcPre = (((adcClock >> _ADC_CTRL_TIMEBASE_SHIFT) * ADCTime_us *2) / div);
			adc->CTRL = (adc->CTRL & ~(_ADC_CTRL_PRESC_MASK | _ADC_CTRL_TIMEBASE_MASK))
					| ((adcPre  << _ADC_CTRL_PRESC_SHIFT) & _ADC_CTRL_PRESC_MASK) | adcClock;
			if (adcPre <= (_ADC_CTRL_PRESC_MASK >> _ADC_CTRL_PRESC_SHIFT)) break;
			adcClock <<= 1;
		} while (adcClock < _ADC_CTRL_TIMEBASE_MASK);
		for(int i=1000000; i  && ((adc->STATUS & ADC_STATUS_WARM) == 0); i--) __nop();
	}
#else
	(void)ADCNum;
	(void)Channel;
#endif
	(void)location;
}

void ADCOpenDifferential(int ADCNum,int location,ADC_CHANNEL posChannel, ADC_CHANNEL negChannel, ADC_REFERENCE ref, ADC_SAMPLINGTIME delay, int ADCTime_us) {
	ADCOpen(ADCNum,location,posChannel,ref,delay, ADCTime_us);
#if ADC_COUNT > 0
	ADC_TypeDef* adc = _GetADC(ADCNum);
	if (adc) {
		adc->SINGLECTRL &= ~_ADC_SINGLECTRL_NEGSEL_MASK;
		adc->SINGLECTRL |= ((negChannel + (location * 32)) << _ADC_SINGLECTRL_NEGSEL_SHIFT) | ADC_SINGLECTRL_DIFF;
	}
#endif
}

int ADCGetReference(int ADCNum) {	// Returns ADC reference in mV
#if ADC_COUNT > 0
	ADC_TypeDef* adc = _GetADC(ADCNum);
	if (adc) {
		switch(adc->SINGLECTRL & _ADC_SINGLECTRL_REF_MASK) {
		case ADC_SINGLECTRL_REF_1V25: return 1250;
		case ADC_SINGLECTRL_REF_2V5: return 2500;
		case ADC_SINGLECTRL_REF_VDD: return 3600;
		case ADC_SINGLECTRL_REF_5V: return 5000;
		default: return 0;
		}
	}
#endif
	return 0;
}

int ADCGetFullScale(int ADCNum) {	// Returns ADC maximum value
	(void)ADCNum;
#if ADC_COUNT > 0
	ADC_TypeDef* adc = _GetADC(ADCNum);
	if (adc) {
		switch(adc->SINGLECTRL & _ADC_SINGLECTRL_RES_MASK) {
		case ADC_SINGLECTRL_RES_6BIT:
			return (0x01 << 6) - 1;
			break;
		case ADC_SINGLECTRL_RES_8BIT:
			return (0x01 << 8) - 1;
			break;
		case ADC_SINGLECTRL_RES_12BIT:
			return (0x01 << 12) - 1;
			break;
		case ADC_SINGLECTRL_RES_OVS:
			if ((adc->CTRL & _ADC_CTRL_OVSRSEL_MASK) == ADC_CTRL_OVSRSEL_X2) return (0x01 << 13) - 1;
			if ((adc->CTRL & _ADC_CTRL_OVSRSEL_MASK) == ADC_CTRL_OVSRSEL_X4) return (0x01 << 14) - 1;
			if ((adc->CTRL & _ADC_CTRL_OVSRSEL_MASK) == ADC_CTRL_OVSRSEL_X8) return (0x01 << 15) - 1;
			break;
			return (0x01 << 16) - 1;
		}
	}
#endif
	return 0;
}

void ADCSetSamplingTime(int ADCNum, int duration)
{
#if ADC_COUNT > 0
	if(_GetADC(ADCNum)) ADC_Reset(_GetADC(ADCNum));
	ADCPowerEnable(ADCNum,false);
	(void)duration;
#else
	(void)ADCNum;
	(void)duration;
#endif
}

void ADCClose(int ADCNum, ADC_CHANNEL Channel)
{
#if ADC_COUNT > 0
	(void)Channel;
	if(_GetADC(ADCNum)) ADC_Reset(_GetADC(ADCNum));
	ADCPowerEnable(ADCNum,false);
#else
	(void)ADCNum;
	(void)Channel;
#endif
}

unsigned long ADCConvertOnce(int ADCNum)
{
#ifdef ADC_PRESENT
ADC_TypeDef* adc = _GetADC(ADCNum);
unsigned long val = 0;
	if (adc) {
		ADC_Start(adc, adcStartSingle);
		for (int i=100000; i; i--) if (adc->IF & ADC_IF_SINGLE) {
			val = ADC_DataSingleGet(adc);
			break;
		}
		adc->CMD = ADC_CMD_SINGLESTOP;
	}
	return val;
#else
	(void)ADCNum;
	return 0;
#endif
}

void ADCStartConvert(int ADCNum)
{
#ifdef ADC_PRESENT
ADC_TypeDef* adc = _GetADC(ADCNum);
	if (adc) {
		adc->CTRL |= ADC_CTRL_WARMUPMODE_KEEPADCWARM;
		adc->SINGLECTRL |= ADC_SINGLECTRL_REP;
		adc->IEN = ADC_IEN_SINGLE;
		SystemIRQEnable(ADC0_IRQn);	// There is only one ADC IRQ
		ADC_Start(adc, adcStartSingle);
	}
#else
	(void)ADCNum;
#endif
}
void ADCStopConvert(int ADCNum) {
#ifdef ADC_PRESENT
ADC_TypeDef* adc = _GetADC(ADCNum);
	if (adc) {
		adc->CTRL &= ~_ADC_CTRL_WARMUPMODE_MASK;
		adc->IEN &= ~ADC_IEN_SINGLE;
		adc->CTRL &= ~_ADC_SINGLECTRL_REP_MASK;	// Make sure single conversation only
//		NVIC_DisableIRQ(ADC0_IRQn);
	}
#else
	(void)ADCNum;
#endif
}

#if ACMP_COUNT > 0
static ACMP_TypeDef *GetACMP(int ADCNum) {
	if (ADCNum == 0) return ACMP0;
#if ACMP_COUNT > 1
	if (ADCNum == 1) return ACMP1;
#endif
#if ACMP_COUNT > 2
	if (ADCNum == 2) return ACMP2;
#endif
	return NULL;
}
static void ACMPPowerEnable(int ADCNum, int enable) {
	if (ADCNum == 0)
		CMU_ClockEnable(cmuClock_ACMP0, enable);
#if ACMP_COUNT > 1
	else if (ADCNum == 1)
		CMU_ClockEnable(cmuClock_ACMP1, enable);
#endif
#if ACMP_COUNT > 2
	else if (ADCNum == 2)
		CMU_ClockEnable(cmuClock_ACMP2, enable);
#endif
}
#endif

void ADCTriggerStart(int ADCNum, int location, ADC_CHANNEL channel, int triggerLevel, ADC_TRIGGER_OUTPUT outLevel) {
#ifdef ACMP_PRESENT
	ACMP_TypeDef *acmp = GetACMP(ADCNum);
	if (acmp) {
		ACMPPowerEnable(ADCNum, true);
		acmp->ROUTELOC0 &= ~_ACMP_ROUTELOC0_OUTLOC_MASK;
		acmp->ROUTELOC0 |= min(location,3) << _ACMP_ROUTELOC0_OUTLOC_SHIFT;
		ACMP_Init(acmp, &ACMPInit);
		acmp->CTRL |=
				(outLevel & ADCTriggerRising ) ? ACMP_CTRL_IRISE_ENABLED : ACMP_CTRL_IRISE_DISABLED
				| (outLevel & ADCTriggerFalling ) ? ACMP_CTRL_IFALL_ENABLED : ACMP_CTRL_IFALL_DISABLED
				;
		ACMP_VAConfig_TypeDef cfg;
		cfg.div0 = cfg.div1 = min(triggerLevel,63);
		cfg.input = acmpVAInputVDD;
		ACMP_VASetup(acmp,&cfg);
		ACMP_ChannelSet(acmp, acmpInputVADIV, channel);
		for (int i=1000000; i && (acmp->STATUS & ACMP_STATUS_ACMPACT) == 0; i--);
		acmp->IFC = ACMP_IFC_EDGE | ACMP_IEN_WARMUP;
		SystemIRQEnable(ACMP0_IRQn);
		ACMP_IntEnable(acmp,ACMP_IEN_EDGE);
	}
#else
	(void)ADCNum;
	(void)location;
	(void)channel;
	(void)triggerLevel;
	(void)outLevel;
#endif
}
void ADCTriggerEnable(int ADCNum, BOOL enable) {
#ifdef ACMP_PRESENT
	ACMP_TypeDef *acmp = GetACMP(ADCNum);
	if (acmp) {
		if (enable) {
			ACMP_Enable(acmp);
			// Wait for ACMP to warm up
			for (int i=0xFFFFFF; i && ((acmp->STATUS & ACMP_STATUS_ACMPACT) == 0); i--) __nop();
		}
		else
			ACMP_Disable(acmp);
	}
#else
	(void)ADCNum;
	(void)enable;
#endif
}

ADC_TRIGGER_OUTPUT ADCTriggerGetOutputLevel(int ADCNum) {
#ifdef ACMP_PRESENT
	ACMP_TypeDef *acmp = GetACMP(ADCNum);
	if (acmp) {
		return ((acmp->STATUS & ACMP_STATUS_ACMPOUT) ? ADCTriggerRising : ADCTriggerFalling);
	}
#else
	(void)ADCNum;
#endif
	return 0;
}

void ADCTriggerStop(int ADCNum, ADC_CHANNEL channel) {
#ifdef ACMP_PRESENT
	(void) channel;	// Remove warning
	ACMP_TypeDef *acmp = GetACMP(ADCNum);
	if (acmp) {
		ACMP_Disable(acmp);
		ACMP_Reset(acmp);
		ACMPPowerEnable(ADCNum, false);
	}
#else
	(void)ADCNum;
	(void)channel
#endif
}
/** }@ */
