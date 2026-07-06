import Plot from 'react-plotly.js'
import { Box, Grid, Typography } from '@mui/material'
import type { TelemetrySnapshot } from '../app/types'
import { deriveDashboardMetrics } from '../app/dashboardMath'

type GaugesProps = {
  snapshot: TelemetrySnapshot
}

function clamp(value: number, min: number, max: number) {
  return Math.max(min, Math.min(max, value))
}

function indicator(
  title: string,
  value: number,
  axis: { min: number; max: number; suffix?: string; threshold?: number; color: string },
) {
  return {
    type: 'indicator',
    mode: 'gauge+number',
    value,
    title: { text: title },
    number: { suffix: axis.suffix ?? '' },
    gauge: {
      axis: { range: [axis.min, axis.max] },
      bar: { color: axis.color },
      steps: [
        { range: [axis.min, (axis.min + axis.max) * 0.6], color: 'rgba(255,255,255,0.04)' },
        { range: [(axis.min + axis.max) * 0.6, axis.max], color: 'rgba(255,255,255,0.08)' },
      ],
      threshold: axis.threshold
        ? {
            line: { color: '#FF6B6B', width: 4 },
            thickness: 0.75,
            value: axis.threshold,
          }
        : undefined,
    },
  }
}

export function Gauges({ snapshot }: GaugesProps) {
  const metrics = deriveDashboardMetrics(snapshot)
  const phaseCurrent = metrics.current
  const rpm = metrics.rpm
  const voltage = metrics.voltage
  const efficiency = clamp(metrics.efficiency, 0, 100)
  const temperature = clamp(metrics.temperature, 20, 140)

  const gauges = [
    indicator('RPM', rpm, { min: 0, max: 6000, suffix: ' rpm', color: '#D1D7DC', threshold: 4500 }),
    indicator('Torque', metrics.torque, { min: 0, max: 10, suffix: ' N·m', color: '#C98B4D', threshold: 8 }),
    indicator('Voltage', voltage, { min: 0, max: 60, suffix: ' V', color: '#AAB4BC', threshold: 48 }),
    indicator('Current', phaseCurrent, { min: 0, max: 40, suffix: ' A', color: '#D1D7DC', threshold: 32 }),
    indicator('Efficiency', efficiency, { min: 0, max: 100, suffix: ' %', color: '#AAB4BC', threshold: 92 }),
    indicator('Temperature', temperature, { min: 20, max: 140, suffix: ' °C', color: '#FF6A4D', threshold: 95 }),
  ]

  return (
    <Grid container spacing={1.5}>
      {gauges.map((gauge, index) => (
        <Grid item xs={12} sm={6} md={4} key={index}>
          <Box
            sx={{
              height: 220,
              borderRadius: 3,
              overflow: 'hidden',
              border: '1px solid rgba(201,139,77,0.1)',
              background: 'rgba(255,255,255,0.015)',
            }}
          >
            <Plot
              data={[gauge] as never}
              layout={{
                margin: { l: 18, r: 18, t: 30, b: 10 },
                paper_bgcolor: 'rgba(0,0,0,0)',
                plot_bgcolor: 'rgba(0,0,0,0)',
                font: { color: '#B6C8D2' },
                height: 220,
              }}
              style={{ width: '100%', height: '100%' }}
              config={{ displayModeBar: false, responsive: true }}
              useResizeHandler
            />
          </Box>
        </Grid>
      ))}
    </Grid>
  )
}
