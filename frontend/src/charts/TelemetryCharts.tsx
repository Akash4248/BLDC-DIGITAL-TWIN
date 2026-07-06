import { Button, Stack, Typography } from '@mui/material'
import FileDownloadIcon from '@mui/icons-material/FileDownload'
import Plot from 'react-plotly.js'
import { useMemo } from 'react'
import type { TelemetrySnapshot } from '../app/types'

type TelemetryChartsProps = {
  history: TelemetrySnapshot[]
}

function toCsv(history: TelemetrySnapshot[]) {
  const header = [
    'index',
    'timestamp_us',
    'duty_a',
    'duty_b',
    'duty_c',
    'vdc',
    'ia',
    'ib',
    'ic',
    'speed',
    'torque',
    'rotor_angle',
  ]
  const rows = history.map((item, index) =>
    [
      index,
      item.telemetry.timestamp_us,
      item.telemetry.duty_a,
      item.telemetry.duty_b,
      item.telemetry.duty_c,
      item.telemetry.vdc,
      item.current.ia,
      item.current.ib,
      item.current.ic,
      item.speed,
      item.torque,
      item.rotor_angle,
    ].join(','),
  )
  return [header.join(','), ...rows].join('\n')
}

function downloadCsv(history: TelemetrySnapshot[]) {
  const blob = new Blob([toCsv(history)], { type: 'text/csv;charset=utf-8;' })
  const url = URL.createObjectURL(blob)
  const anchor = document.createElement('a')
  anchor.href = url
  anchor.download = `bldc-telemetry-${Date.now()}.csv`
  anchor.click()
  URL.revokeObjectURL(url)
}

const commonLayout = {
  margin: { l: 42, r: 16, t: 20, b: 38 },
  paper_bgcolor: 'rgba(0,0,0,0)',
  plot_bgcolor: 'rgba(0,0,0,0)',
  font: { color: '#B6C8D2' },
  xaxis: { gridcolor: 'rgba(255,255,255,0.06)', zeroline: false },
  yaxis: { gridcolor: 'rgba(255,255,255,0.06)', zeroline: false },
  legend: { orientation: 'h' as const, y: 1.15 },
}

export function TelemetryCharts({ history }: TelemetryChartsProps) {
  const x = useMemo(() => history.map((_, index) => index), [history])

  const currentTrace = [
    {
      x,
      y: history.map((item) => item.current.ia),
      type: 'scatter',
      mode: 'lines',
      name: 'Ia',
      line: { color: '#6EE7F9', width: 2 },
    },
    {
      x,
      y: history.map((item) => item.current.ib),
      type: 'scatter',
      mode: 'lines',
      name: 'Ib',
      line: { color: '#FFB86B', width: 2 },
    },
    {
      x,
      y: history.map((item) => item.current.ic),
      type: 'scatter',
      mode: 'lines',
      name: 'Ic',
      line: { color: '#5FD38A', width: 2 },
    },
  ]

  const speedTorqueTrace = [
    {
      x,
      y: history.map((item) => item.speed),
      type: 'scatter',
      mode: 'lines',
      name: 'Speed',
      line: { color: '#6EE7F9', width: 2 },
      yaxis: 'y1',
    },
    {
      x,
      y: history.map((item) => item.torque),
      type: 'scatter',
      mode: 'lines',
      name: 'Torque',
      line: { color: '#FFB86B', width: 2 },
      yaxis: 'y2',
    },
  ]

  const rotorDutyTrace = [
    {
      x,
      y: history.map((item) => item.rotor_angle),
      type: 'scatter',
      mode: 'lines',
      name: 'Rotor angle',
      line: { color: '#FFB86B', width: 2 },
    },
    {
      x,
      y: history.map((item) => item.telemetry.duty_a),
      type: 'scatter',
      mode: 'lines',
      name: 'Duty A',
      line: { color: '#6EE7F9', width: 2, dash: 'solid' },
    },
    {
      x,
      y: history.map((item) => item.telemetry.duty_b),
      type: 'scatter',
      mode: 'lines',
      name: 'Duty B',
      line: { color: '#FFB86B', width: 2, dash: 'dot' },
    },
    {
      x,
      y: history.map((item) => item.telemetry.duty_c),
      type: 'scatter',
      mode: 'lines',
      name: 'Duty C',
      line: { color: '#5FD38A', width: 2, dash: 'dash' },
    },
  ]

  return (
    <Stack spacing={2}>
      <Stack direction="row" justifyContent="space-between" alignItems="center" flexWrap="wrap" spacing={1}>
        <Typography variant="body2" color="text.secondary">
          Zoom, pan, and export are enabled. Use the built-in plot controls for detail inspection.
        </Typography>
        <Button
          variant="outlined"
          size="small"
          startIcon={<FileDownloadIcon />}
          onClick={() => downloadCsv(history)}
        >
          Export CSV
        </Button>
      </Stack>

      <Plot
        data={currentTrace as never}
        layout={{
          ...commonLayout,
          title: { text: 'Phase currents' },
        }}
        style={{ width: '100%', height: 280 }}
        config={{
          displayModeBar: true,
          displaylogo: false,
          responsive: true,
          scrollZoom: true,
        }}
        useResizeHandler
      />

      <Plot
        data={speedTorqueTrace as never}
        layout={{
          ...commonLayout,
          title: { text: 'Speed and torque' },
          yaxis2: {
            overlaying: 'y',
            side: 'right',
            gridcolor: 'rgba(255,255,255,0.06)',
            zeroline: false,
            showgrid: false,
          },
        }}
        style={{ width: '100%', height: 280 }}
        config={{
          displayModeBar: true,
          displaylogo: false,
          responsive: true,
          scrollZoom: true,
        }}
        useResizeHandler
      />

      <Plot
        data={rotorDutyTrace as never}
        layout={{
          ...commonLayout,
          title: { text: 'Rotor angle and duty cycle' },
        }}
        style={{ width: '100%', height: 280 }}
        config={{
          displayModeBar: true,
          displaylogo: false,
          responsive: true,
          scrollZoom: true,
        }}
        useResizeHandler
      />
    </Stack>
  )
}

