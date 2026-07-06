import { describe, expect, it } from 'vitest'
import { deriveDashboardMetrics } from './dashboardMath'

describe('deriveDashboardMetrics', () => {
  it('derives expected dashboard values from snapshot data', () => {
    const metrics = deriveDashboardMetrics({
      telemetry: {
        sequence: 1,
        timestamp_us: 1000,
        duty_a: 0.5,
        duty_b: 0.5,
        duty_c: 0.5,
        vdc: 24,
      },
      current: { ia: 2, ib: -1, ic: 0.5, adc_a: 0, adc_b: 0, adc_c: 0 },
      speed: 100,
      torque: 1.5,
      rotor_angle: 0.25,
      hall: { a: 1, b: 0, c: 1 },
      encoder: { a: 1, b: 0, index: 1 },
    })

    expect(metrics.rpm).toBeCloseTo(954.93, 2)
    expect(metrics.torque).toBeCloseTo(1.5, 6)
    expect(metrics.voltage).toBeCloseTo(24, 6)
    expect(metrics.current).toBeCloseTo(2, 6)
    expect(metrics.efficiency).toBeGreaterThanOrEqual(0)
    expect(metrics.temperature).toBeGreaterThan(20)
  })
})

