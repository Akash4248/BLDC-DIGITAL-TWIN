import { Stack, Typography, Chip, Divider } from '@mui/material'
import type { SimulationState } from '../app/types'
import { PanelCard } from './PanelCard'

type StatusPanelProps = {
  state: SimulationState
  connection: 'connecting' | 'connected' | 'disconnected' | 'mock'
}

export function StatusPanel({ state, connection }: StatusPanelProps) {
  const connectionTone: 'default' | 'success' | 'warning' =
    connection === 'connected' ? 'success' : connection === 'mock' ? 'warning' : 'default'

  return (
    <PanelCard eyebrow="status" title="Runtime status">
      <Stack spacing={1.5}>
        <Stack direction="row" spacing={1} flexWrap="wrap">
          <Chip label={connection} color={connectionTone} />
          <Chip label={state.running ? 'running' : 'stopped'} color={state.running ? 'success' : 'default'} />
          <Chip label={`${state.clients} clients`} variant="outlined" />
        </Stack>
        <Divider sx={{ borderColor: 'rgba(110, 231, 249, 0.12)' }} />
        <Stack spacing={0.75}>
          <Typography variant="body2" color="text.secondary">
            Sample rate
          </Typography>
          <Typography variant="h6">{state.sample_rate_hz} Hz</Typography>
          <Typography variant="body2" color="text.secondary">
            Step index
          </Typography>
          <Typography variant="h6">{state.step_index}</Typography>
        </Stack>
      </Stack>
    </PanelCard>
  )
}
