import { useEffect, useRef, useState } from 'react'
import {
  AppBar,
  Box,
  Chip,
  Container,
  Grid,
  Stack,
  Toolbar,
  Typography,
  useMediaQuery,
} from '@mui/material'
import DashboardIcon from '@mui/icons-material/Dashboard'
import ElectricBoltIcon from '@mui/icons-material/ElectricBolt'
import SpeedIcon from '@mui/icons-material/Speed'
import QueryStatsIcon from '@mui/icons-material/QueryStats'
import { PanelCard } from '../components/PanelCard'
import { MetricTile } from '../components/MetricTile'
import { FaultPanel } from '../components/FaultPanel'
import { ParameterPanel } from '../components/ParameterPanel'
import { StatusPanel } from '../components/StatusPanel'
import { LoggerPanel } from '../components/LoggerPanel'
import { TelemetryCharts } from '../charts/TelemetryCharts'
import { Gauges } from '../charts/Gauges'
import { MotorScene } from '../visualization/MotorScene'
import { backendClient } from '../services/backendClient'
import type { FaultItem, LogEntry, MotorParameters, SimulationState } from './types'
import { useMotorState } from '../hooks/useMotorState'

const initialParameters: MotorParameters = {
  Rs: 0.5,
  Ld: 0.001,
  Lq: 0.001,
  PolePairs: 4,
  FluxLinkage: 0.02,
  Kt: 0.1,
  Ke: 0.1,
  RotorInertia: 0.01,
  Friction: 0.1,
  LoadTorque: 0,
  Vdc: 24,
}

const initialState: SimulationState = {
  running: false,
  clients: 0,
  sample_rate_hz: 1000,
  step_index: 0,
}

export default function App() {
  const isMobile = useMediaQuery('(max-width:900px)')
  const { snapshot, motorState, history, connection, connectedClients } = useMotorState()
  const [state, setState] = useState<SimulationState>(initialState)
  const [parameters, setParameters] = useState<MotorParameters>(initialParameters)
  const [faults, setFaults] = useState<FaultItem[]>([])
  const [logs, setLogs] = useState<LogEntry[]>([])
  const [logsLoading, setLogsLoading] = useState(false)
  const parameterTimer = useRef<number | null>(null)

  useEffect(() => {
    let mounted = true
    backendClient
      .getState()
      .then((next) => {
        if (!mounted) return
        setState({ ...next, clients: connectedClients })
        setFaults(next.faults ?? [])
      })
      .catch(() => {
        setState((current) => ({ ...current, clients: connectedClients }))
      })
    return () => {
      mounted = false
    }
  }, [connectedClients])

  useEffect(() => {
    setState((current) => ({ ...current, clients: connectedClients }))
  }, [connectedClients])

  useEffect(() => {
    let mounted = true

    const refreshLogs = () => {
      setLogsLoading(true)
      backendClient
        .logs()
        .then((next) => {
          if (mounted) {
            setLogs(next)
          }
        })
        .catch(() => undefined)
        .finally(() => {
          if (mounted) {
            setLogsLoading(false)
          }
        })
    }

    refreshLogs()
    const interval = window.setInterval(refreshLogs, 2000)

    return () => {
      mounted = false
      window.clearInterval(interval)
    }
  }, [])

  useEffect(() => {
    if (parameterTimer.current !== null) {
      window.clearTimeout(parameterTimer.current)
    }

    parameterTimer.current = window.setTimeout(() => {
      backendClient.updateParameters(parameters).catch(() => undefined)
    }, 150)

    return () => {
      if (parameterTimer.current !== null) {
        window.clearTimeout(parameterTimer.current)
        parameterTimer.current = null
      }
    }
  }, [parameters])

  const speedRpm = snapshot.speed * 9.5493
  const currentMagnitude = Math.max(
    Math.abs(snapshot.current.ia),
    Math.abs(snapshot.current.ib),
    Math.abs(snapshot.current.ic),
  )
  const connectionTone: 'default' | 'success' | 'warning' =
    connection === 'connected' ? 'success' : connection === 'mock' ? 'warning' : 'default'

  function clearFaults() {
    backendClient.clearFaults().then((response) => setFaults(response.faults)).catch(() => undefined)
  }

  function toggleFault(faultType: string, enabled: boolean) {
    backendClient
      .injectFault({ fault_type: faultType, enabled, value: null })
      .then((response) => setFaults(response.faults))
      .catch(() => undefined)
  }

  function refreshLogs() {
    setLogsLoading(true)
    backendClient
      .logs()
      .then((next) => setLogs(next))
      .catch(() => undefined)
      .finally(() => setLogsLoading(false))
  }

  return (
    <Box sx={{ minHeight: '100vh', position: 'relative', overflow: 'hidden' }}>
      <Box
        sx={{
          position: 'fixed',
          inset: 0,
          background:
            'radial-gradient(circle at 20% 20%, rgba(110,231,249,0.12), transparent 30%), radial-gradient(circle at 85% 15%, rgba(255,184,107,0.12), transparent 22%), linear-gradient(180deg, #071017 0%, #061116 100%)',
          pointerEvents: 'none',
        }}
      />
      <Box
        sx={{
          position: 'fixed',
          inset: 0,
          opacity: 0.35,
          pointerEvents: 'none',
          backgroundImage:
            'linear-gradient(rgba(255,255,255,0.03) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,0.03) 1px, transparent 1px)',
          backgroundSize: '36px 36px',
          maskImage:
            'linear-gradient(180deg, rgba(0,0,0,0.8), rgba(0,0,0,0.05) 70%, transparent)',
        }}
      />
      <AppBar
        position="sticky"
        elevation={0}
        sx={{
          background: 'rgba(5,10,14,0.78)',
          backdropFilter: 'blur(18px)',
          borderBottom: '1px solid rgba(110,231,249,0.08)',
        }}
      >
        <Toolbar sx={{ gap: 2, flexWrap: 'wrap', py: 1 }}>
          <Stack direction="row" alignItems="center" spacing={1.25} sx={{ flexGrow: 1 }}>
            <Box
              sx={{
                width: 38,
                height: 38,
                borderRadius: '50%',
                background:
                  'radial-gradient(circle at 30% 30%, #6EE7F9, #1E404A 55%, #081015 100%)',
                boxShadow: '0 0 0 1px rgba(110,231,249,0.18), 0 0 28px rgba(110,231,249,0.25)',
              }}
            />
            <Box>
              <Typography variant="caption" color="text.secondary">
                industrial digital twin
              </Typography>
              <Typography variant="h6" sx={{ lineHeight: 1.1 }}>
                BLDC / PMSM Control Room
              </Typography>
            </Box>
          </Stack>
          <Stack direction="row" spacing={1} flexWrap="wrap">
            <Chip icon={<DashboardIcon />} label={connection} color={connectionTone} />
            <Chip icon={<ElectricBoltIcon />} label={`${state.sample_rate_hz} Hz loop`} variant="outlined" />
            <Chip icon={<QueryStatsIcon />} label={`${state.clients} clients`} variant="outlined" />
          </Stack>
        </Toolbar>
      </AppBar>

      <Container maxWidth="xl" sx={{ py: 3, position: 'relative', zIndex: 1 }}>
        <Grid container spacing={2.5}>
          <Grid item xs={12} lg={8}>
            <PanelCard eyebrow="motor" title="Rotor field and phase state" height={isMobile ? 420 : 620}>
              <Box sx={{ height: isMobile ? 340 : 500, borderRadius: 4, overflow: 'hidden' }}>
                <MotorScene snapshot={motorState} />
              </Box>
              <Grid container spacing={1.5} sx={{ mt: 1 }}>
                <Grid item xs={12} sm={4}>
                  <MetricTile
                    label="Speed"
                    value={speedRpm.toFixed(0)}
                    suffix="rpm"
                    tone="primary"
                    icon={<SpeedIcon fontSize="small" />}
                  />
                </Grid>
                <Grid item xs={12} sm={4}>
                  <MetricTile label="Torque" value={snapshot.torque.toFixed(2)} suffix="N·m" tone="secondary" />
                </Grid>
                <Grid item xs={12} sm={4}>
                  <MetricTile label="Current" value={currentMagnitude.toFixed(1)} suffix="A peak" tone="success" />
                </Grid>
              </Grid>
            </PanelCard>
          </Grid>

          <Grid item xs={12} lg={4}>
            <Stack spacing={2.5} sx={{ height: '100%' }}>
              <StatusPanel state={{ ...state, clients: connectedClients }} connection={connection} />
              <FaultPanel faults={faults} onClear={clearFaults} onToggle={toggleFault} />
            </Stack>
          </Grid>

          <Grid item xs={12} lg={8}>
            <PanelCard eyebrow="graphs" title="Live traces" height={940}>
              <TelemetryCharts history={history} />
            </PanelCard>
          </Grid>

          <Grid item xs={12} lg={4}>
            <Stack spacing={2.5}>
              <PanelCard eyebrow="gauges" title="Realtime gauges">
                <Gauges snapshot={snapshot} />
              </PanelCard>
              <ParameterPanel
                parameters={parameters}
                onChange={setParameters}
                onSave={() => backendClient.updateParameters(parameters).catch(() => undefined)}
              />
              <LoggerPanel entries={logs} loading={logsLoading} onRefresh={refreshLogs} />
            </Stack>
          </Grid>

          <Grid item xs={12} lg={6}>
            <PanelCard eyebrow="summary" title="Current snapshot">
              <Stack spacing={1.25}>
                <MetricTile label="Ia" value={snapshot.current.ia.toFixed(2)} suffix="A" tone="primary" />
                <MetricTile label="Ib" value={snapshot.current.ib.toFixed(2)} suffix="A" tone="secondary" />
                <MetricTile label="Ic" value={snapshot.current.ic.toFixed(2)} suffix="A" tone="success" />
                <MetricTile label="Duty A" value={snapshot.telemetry.duty_a.toFixed(3)} tone="primary" />
                <MetricTile label="Duty B" value={snapshot.telemetry.duty_b.toFixed(3)} tone="secondary" />
                <MetricTile label="Duty C" value={snapshot.telemetry.duty_c.toFixed(3)} tone="success" />
              </Stack>
            </PanelCard>
          </Grid>

          <Grid item xs={12} lg={6}>
            <PanelCard eyebrow="status" title="Electrical and sensor flags">
              <Stack direction="row" spacing={1} flexWrap="wrap">
                <Chip label={`Hall ${snapshot.hall.a}${snapshot.hall.b}${snapshot.hall.c}`} color="primary" variant="outlined" />
                <Chip label={`Encoder ${snapshot.encoder.a}${snapshot.encoder.b}`} color="secondary" variant="outlined" />
                <Chip label={snapshot.encoder.index ? 'Index high' : 'Index low'} color={snapshot.encoder.index ? 'warning' : 'default'} />
              </Stack>
              <Typography variant="body2" color="text.secondary" sx={{ mt: 1.5 }}>
                The dashboard is responsive down to mobile widths. On narrow screens the motor panel leads, with control and status panels collapsing into a single column.
              </Typography>
            </PanelCard>
          </Grid>
        </Grid>
      </Container>
    </Box>
  )
}
