/*
 * sx1276-board.c
 *
 */
/** \addtogroup LW LoRaWAN Implementation
 *  @{
 */

#include "board.h"
#include "global.h"

const struct Radio_s Radio =
{
    SX1276Init,
    SX1276GetStatus,
    SX1276SetModem,
    SX1276SetChannel,
    SX1276IsChannelFree,
    SX1276Random,
    SX1276SetRxConfig,
    SX1276SetTxConfig,
    SX1276CheckRfFrequency,
    SX1276GetTimeOnAir,
    SX1276Send,
    SX1276SetSleep,
    SX1276SetStby,
    SX1276SetRx,
    SX1276StartCad,
    SX1276SetTxContinuousWave,
    SX1276ReadRssi,
    SX1276Write,
    SX1276Read,
    SX1276WriteBuffer,
    SX1276ReadBuffer,
    SX1276SetMaxPayloadLength,
    SX1276SetPublicNetwork
};

static SPIPORT RfSpi;
/*!
 * \brief Initializes the radio I/Os pins interface
 */
void SX1276IoInit( void ) {
	// Setup LoRaWAN Original variables as expected by LoRaWAN SX1276 Driver
	SX1276.Reset = RF_RESET;
	SX1276.DIO0 = RF_DIO0;
	SX1276.DIO1 = RF_DIO1;
	SX1276.DIO2 = RF_DIO2;
	SX1276.DIO3 = RF_DIO3;
	SX1276.DIO4 = RF_DIO4;
	SX1276.DIO5 = RF_DIO5;
	SX1276.Spi.Sclk = RfSpi.Sclk = SPI_SCLK;
	SX1276.Spi.Miso = RfSpi.Miso = SPI_MISO;
	SX1276.Spi.Mosi = RfSpi.Mosi = SPI_MOSI;
	RfSpi.spi_name = SPI_NAME;
	RfSpi.routeLocation = SPI_LOCATION;
	SX1276.Spi.Nss = RF_NSS;
	SX1276.Spi.Spi.Instance = (void*)&RfSpi;// Fake value for LoRaWAN SX1276 driver to work
	SPIOpen(&RfSpi, 10000000UL);
	SX1276SetSleep();						// Make sure to switch off radio chip
}

/*!
 * \brief Initializes DIO IRQ handlers
 *
 * \param [in] irqHandlers Array containing the IRQ callback functions
 */
void SX1276IoIrqInit( DioIrqHandler **irqHandlers ) {
	SystemDefinePortIrqHandler(SX1276.DIO0,(SYSTEMPORT_IRQHANDLER)irqHandlers[0],GPIOIRQRising);
	SystemDefinePortIrqHandler(SX1276.DIO1,(SYSTEMPORT_IRQHANDLER)irqHandlers[1],GPIOIRQRising);
	SystemDefinePortIrqHandler(SX1276.DIO2,(SYSTEMPORT_IRQHANDLER)irqHandlers[2],GPIOIRQRising);
	SystemDefinePortIrqHandler(SX1276.DIO3,(SYSTEMPORT_IRQHANDLER)irqHandlers[3],GPIOIRQRising);
	SystemDefinePortIrqHandler(SX1276.DIO4,(SYSTEMPORT_IRQHANDLER)irqHandlers[4],GPIOIRQRising);
}

/*!
 * \brief De-initializes the radio I/Os pins interface.
 *
 * \remark Useful when going in MCU low power modes
 */
void SX1276IoDeInit( void ) {
	SX1276SetSleep();						// Make sure to switch off radio chip
}

/*!
 * \brief Sets the radio output power.
 *
 * \param [in] power Sets the RF output power
 */
void SX1276SetRfTxPower( int8_t power ) {
    uint8_t paConfig = 0;
    uint8_t paDac = 0;

    paConfig = SX1276Read( REG_PACONFIG );
    paDac = SX1276Read( REG_PADAC );

    paConfig = ( paConfig & RF_PACONFIG_PASELECT_MASK ) | SX1276GetPaSelect( SX1276.Settings.Channel );
    paConfig = ( paConfig & RF_PACONFIG_MAX_POWER_MASK ) | 0x70;

    if( ( paConfig & RF_PACONFIG_PASELECT_PABOOST ) == RF_PACONFIG_PASELECT_PABOOST )
    {
        if( power > 17 )
        {
            paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_ON;
        }
        else
        {
            paDac = ( paDac & RF_PADAC_20DBM_MASK ) | RF_PADAC_20DBM_OFF;
        }
        if( ( paDac & RF_PADAC_20DBM_ON ) == RF_PADAC_20DBM_ON )
        {
            if( power < 5 )
            {
                power = 5;
            }
            if( power > 20 )
            {
                power = 20;
            }
            paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 5 ) & 0x0F );
        }
        else
        {
            if( power < 2 )
            {
                power = 2;
            }
            if( power > 17 )
            {
                power = 17;
            }
            paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power - 2 ) & 0x0F );
        }
    }
    else
    {
        if( power < -1 )
        {
            power = -1;
        }
        if( power > 14 )
        {
            power = 14;
        }
        paConfig = ( paConfig & RF_PACONFIG_OUTPUTPOWER_MASK ) | ( uint8_t )( ( uint16_t )( power + 1 ) & 0x0F );
    }
    SX1276Write( REG_PACONFIG, paConfig );
    SX1276Write( REG_PADAC, paDac );
}

/*!
 * \brief Gets the board PA selection configuration
 *
 * \param [in] channel Channel frequency in Hz
 * \retval PaSelect RegPaConfig PaSelect value
 */
uint8_t SX1276GetPaSelect( uint32_t channel ) {
    if( channel > RF_MID_BAND_THRESH )
    {
        return RF_PACONFIG_PASELECT_PABOOST;
    }
    else
    {
        return RF_PACONFIG_PASELECT_RFO;
    }
}

/*!
 * \brief Set the RF Switch I/Os pins in Low Power mode
 *
 * \param [in] status enable or disable
 */
void SX1276SetAntSwLowPower( bool status ) {
	(void)status;
}

/*!
 * \brief Initializes the RF Switch I/Os pins interface
 */
void SX1276AntSwInit( void ) {
}

/*!
 * \brief De-initializes the RF Switch I/Os pins interface
 *
 * \remark Needed to decrease the power consumption in MCU low power modes
 */
void SX1276AntSwDeInit( void ) {

}

/*!
 * \brief Controls the antenna switch if necessary.
 *
 * \remark see errata note
 *
 * \param [in] opMode Current radio operating mode
 */
void SX1276SetAntSw( uint8_t opMode ) {
	(void)opMode;
}

/*!
 * \brief Checks if the given RF frequency is supported by the hardware
 *
 * \param [in] frequency RF frequency to be checked
 * \retval isSupported [true: supported, false: unsupported]
 */
bool SX1276CheckRfFrequency( uint32_t frequency ) {
    // Implement check. Currently all frequencies are supported
    return true;
}
/**  }@ */
