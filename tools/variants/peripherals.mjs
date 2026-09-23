// Verified per-model peripheral metadata, shared by board flags and pin routes.
export function peripheralFlags(device) {
  const p = device.peripherals;
  if (!p || ![1,2,4].includes(p.uart_count) || p.uart_routes.length !== p.uart_count)
    throw new Error(`Invalid UART metadata: ${device.model}`);
  for (const routes of [...p.uart_routes, p.i2c_routes, p.spi_routes]) {
    const selectors = new Set();
    for (const r of routes) {
      if (!Number.isInteger(r.selector) || r.selector < 0 || r.selector > 3 || selectors.has(r.selector))
        throw new Error(`Invalid route selector: ${device.model}`);
      selectors.add(r.selector);
      for (const [key, pin] of Object.entries(r)) if (key !== 'selector') {
        const m = /^P([0-7])\.([0-7])$/.exec(pin);
        if (!m || !(device.port_masks[+m[1]] & (1 << +m[2]))) throw new Error(`Unbonded route: ${device.model} ${pin}`);
      }
    }
  }
  return Object.entries({UART_COUNT:p.uart_count, UART_AVAILABLE_MASK:(1<<p.uart_count)-1,
    UART_TIMER_PRESCALER:+p.uart_timer_prescaler, UART_PS_BASE:p.uart_timer_prescaler_base,
    UART1_PRESCALER:+p.uart1_prescaler, SPI_PIN_SWAP:+p.spi_pin_swap, I2C_COUNT:p.i2c_count,
    WIRE_LAYOUT:p.i2c_count, SPI_COUNT:p.spi_count,
    SPI_LAYOUT:device.family.startsWith('STC12')?2:1,
    INT01_MODE:/^(STC8(?!9)|AI8|STC15)/.test(device.family)?2:1,
    ...Object.fromEntries(p.spi_dividers.map((v,i)=>[`SPI_DIV${i}`,v]))
  }).map(([k,v])=>`-DSTC_CORE_${k}=${v}`);
}
export function peripheralHeader(device) {
  peripheralFlags(device);
  const p = device.peripherals, lines=[];
  const pin = value => value ? value.replace('.', '_') : 'NOT_A_PIN';
  function route(name, routes, keys) {
    const expr = routes.map(r=>`(${keys.map(k=>`(${k}) == ${pin(r[k])}`).join(' && ')}) ? ${r.selector}u : `).join('');
    lines.push(`#define STC_VARIANT_${name}_ROUTE(${keys.join(',')}) (${expr}255u)`);
  }
  lines.push(`#define STC_VARIANT_UART_COUNT ${p.uart_count}`,`#define STC_VARIANT_I2C_COUNT ${p.i2c_count}`,`#define STC_VARIANT_SPI_COUNT ${p.spi_count}`);
  for (let i=0;i<4;i++) {
    const rs=p.uart_routes[i] || [];
    route(`UART${i+1}`,rs,['rx','tx']);
    for (const k of ['rx','tx']) lines.push(`#define PIN_SERIAL${i+1}_${k.toUpperCase()} ${pin(rs[0]?.[k])}`);
  }
  route('I2C1',p.i2c_routes,['sda','scl']);
  for (const k of ['sda','scl']) lines.push(`#define PIN_I2C1_${k.toUpperCase()} ${pin(p.i2c_routes[0]?.[k])}`);
  route('SPI1',p.spi_routes,['mosi','miso','sck']);
  for (const k of ['mosi','miso','sck','ss']) lines.push(`#define PIN_SPI1_${k.toUpperCase()} ${pin(p.spi_routes[0]?.[k])}`);
  return lines.join('\n');
}
