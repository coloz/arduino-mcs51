/* STC8/Ai8, STC15 and STC12 SPI master. No shared clock is changed.
 * The selector and divider table are generated per model from its manual. */
#define SPI_HW_CTRL 0u
#define SPI_HW_STAT 1u
#define SPI_HW_DATA 2u
#define SPI_HW_SWAP 3u
#if STC_CORE_SPI_COUNT && defined(__SDCC_mcs51)
#define STC_SPI_HARDWARE 1
static __sfr __at (0xcd) stc_spi_stat;
static __sfr __at (0xce) stc_spi_ctrl;
static __sfr __at (0xcf) stc_spi_data;
static __sfr __at (0xa2) stc_spi_mux;
#define SPI_HW_CONTROL(v) (stc_spi_ctrl = (v))
#define SPI_HW_STATUS() stc_spi_stat
#define SPI_HW_CLEAR() (stc_spi_stat = 0xc0u)
#define SPI_HW_SEND(v) (stc_spi_data = (v))
#define SPI_HW_RECEIVE() stc_spi_data
#define SPI_HW_MUX_READ() stc_spi_mux
#define SPI_HW_MUX_WRITE(v) (stc_spi_mux = (v))
#define SPI_HW_GATE_READ() P_SW2
#define SPI_HW_GATE_WRITE(v) (P_SW2 = (v))
#define SPI_HW_SWAP_READ() STC_XFR8(0xfbf9u)
#define SPI_HW_SWAP_WRITE(v) (STC_XFR8(0xfbf9u) = (v))
#elif defined(STC_SPI_HOST_HARDWARE_HOOKS)
#define STC_SPI_HARDWARE 1
uint8_t stc_spi_hw_read(uint8_t bus, uint8_t reg);
void stc_spi_hw_write(uint8_t bus, uint8_t reg, uint8_t value);
uint8_t stc_spi_hw_mux_read(uint8_t bus);
void stc_spi_hw_mux_write(uint8_t bus, uint8_t value);
#define SPI_HW_CONTROL(v) stc_spi_hw_write(0u, SPI_HW_CTRL, (v))
#define SPI_HW_STATUS() stc_spi_hw_read(0u, SPI_HW_STAT)
#define SPI_HW_CLEAR() stc_spi_hw_write(0u, SPI_HW_STAT, 0xc0u)
#define SPI_HW_SEND(v) stc_spi_hw_write(0u, SPI_HW_DATA, (v))
#define SPI_HW_RECEIVE() stc_spi_hw_read(0u, SPI_HW_DATA)
#define SPI_HW_MUX_READ() stc_spi_hw_mux_read(0u)
#define SPI_HW_MUX_WRITE(v) stc_spi_hw_mux_write(0u, (v))
uint8_t stc_spi_hw_gate_read(void);
void stc_spi_hw_gate_write(uint8_t value);
#define SPI_HW_GATE_READ() stc_spi_hw_gate_read()
#define SPI_HW_GATE_WRITE(v) stc_spi_hw_gate_write(v)
#define SPI_HW_SWAP_READ() stc_spi_hw_read(0u, SPI_HW_SWAP)
#define SPI_HW_SWAP_WRITE(v) stc_spi_hw_write(0u, SPI_HW_SWAP, (v))
#endif
#if STC_CORE_SPI_LAYOUT == 2
#define SPI_HW_MUX_MASK 0x20u
#define SPI_HW_MUX_SHIFT 5u
#else
#define SPI_HW_MUX_MASK 0x0cu
#define SPI_HW_MUX_SHIFT 2u
#endif
#ifdef STC_SPI_HARDWARE
static uint8_t spi_hardware, spi_saved_mux;
static unsigned long spi_clock_hz = SPI_DEFAULT_CLOCK_HZ;
#if STC_CORE_SPI_PIN_SWAP
static uint8_t spi_saved_swap;
static void spi_pin_swap(uint8_t restore)
{
    uint8_t gate = SPI_HW_GATE_READ() & 0x80u;
    SPI_HW_GATE_WRITE(SPI_HW_GATE_READ() | 0x80u);
    if (restore) SPI_HW_SWAP_WRITE((SPI_HW_SWAP_READ() & (uint8_t)~0x40u) | spi_saved_swap);
    else { spi_saved_swap = SPI_HW_SWAP_READ() & 0x40u; SPI_HW_SWAP_WRITE(SPI_HW_SWAP_READ() & (uint8_t)~0x40u); }
    SPI_HW_GATE_WRITE((SPI_HW_GATE_READ() & 0x7fu) | gate);
}
#endif
static void spi_hardware_disable(void)
{
    uint8_t enabled;
    if (!spi_hardware) return;
    enabled = spi_lock_registration();
    SPI_HW_CONTROL(0u);
    SPI_HW_MUX_WRITE((SPI_HW_MUX_READ() & (uint8_t)~SPI_HW_MUX_MASK) | spi_saved_mux);
#if STC_CORE_SPI_PIN_SWAP
    spi_pin_swap(1u);
#endif
    spi_hardware = 0u;
    spi_unlock_registration(enabled);
}
static void spi_hardware_configure(void)
{
    uint8_t speed, enabled;
    uint8_t route = STC_VARIANT_SPI1_ROUTE(spi_mosi_pin, spi_miso_pin, spi_sck_pin);
#if defined(STC_CORE_FAMILY_12) || defined(STC_CORE_FAMILY_15)
    /* These manuals call CPHA=0 + SSIG=1 undefined. Keep GPIO for modes
     * 0/2 until this master-mode restriction is resolved on real silicon. */
    if (!(spi_data_mode & 1u)) { spi_hardware_disable(); return; }
#endif
    /* These powers-of-two divisions fold at build time on SDCC. */
    if (spi_clock_hz >= (F_CPU + STC_CORE_SPI_DIV0 - 1UL) / STC_CORE_SPI_DIV0) speed = 0u;
    else if (spi_clock_hz >= (F_CPU + STC_CORE_SPI_DIV1 - 1UL) / STC_CORE_SPI_DIV1) speed = 1u;
    else if (spi_clock_hz >= (F_CPU + STC_CORE_SPI_DIV2 - 1UL) / STC_CORE_SPI_DIV2) speed = 2u;
#if STC_CORE_SPI_DIV3
    else if (spi_clock_hz >= (F_CPU + STC_CORE_SPI_DIV3 - 1UL) / STC_CORE_SPI_DIV3) speed = 3u;
#endif
    else { spi_hardware_disable(); return; }
    if (route == 255u) { spi_hardware_disable(); return; }
    enabled = spi_lock_registration();
    if (!spi_hardware) {
        spi_saved_mux = SPI_HW_MUX_READ() & SPI_HW_MUX_MASK;
#if STC_CORE_SPI_PIN_SWAP
        spi_pin_swap(0u);
#endif
    }
    SPI_HW_CONTROL(0u);
    SPI_HW_MUX_WRITE((SPI_HW_MUX_READ() & (uint8_t)~SPI_HW_MUX_MASK) | (route << SPI_HW_MUX_SHIFT));
    SPI_HW_CLEAR();
    SPI_HW_CONTROL(0xd0u | (spi_bit_order == LSBFIRST ? 0x20u : 0u) | (spi_data_mode << 2) | speed);
    spi_hardware = 1u;
    spi_unlock_registration(enabled);
}
static uint8_t spi_hardware_transfer(uint8_t value)
{
    uint16_t budget = 4096u;
    uint8_t received;
    SPI_HW_CLEAR(); SPI_HW_SEND(value);
    while (!(SPI_HW_STATUS() & 0x80u)) {
        if (!--budget) {
            spi_config_error = STC_SPI_TIMEOUT; spi_hardware_disable(); return 0xffu;
        }
    }
    if (SPI_HW_STATUS() & 0x40u) {
        spi_config_error = STC_SPI_BUSY; SPI_HW_CLEAR(); return 0xffu;
    }
    received = SPI_HW_RECEIVE(); SPI_HW_CLEAR(); return received;
}
#endif
