import { Button, Chip, Divider, FormControlLabel, Stack, Switch, Typography } from '@mui/material'
import type { FaultItem } from '../app/types'
import { PanelCard } from './PanelCard'

type FaultPanelProps = {
  faults: FaultItem[]
  onClear: () => void
  onToggle: (faultType: string, enabled: boolean) => void
}

const faultLabels: Record<string, string> = {
  open_phase: 'Open phase',
  hall_failure: 'Hall failure',
  encoder_failure: 'Encoder failure',
  current_sensor_offset: 'Current sensor offset',
  locked_rotor: 'Locked rotor',
  high_load: 'High load',
  low_voltage: 'Low voltage',
  short_circuit: 'Short circuit',
}

const orderedFaults = Object.keys(faultLabels)

export function FaultPanel({ faults, onClear, onToggle }: FaultPanelProps) {
  const faultMap = new Map(faults.map((fault) => [fault.fault_type, fault]))

  return (
    <PanelCard
      eyebrow="faults"
      title="Fault injection"
      action={
        <Button size="small" variant="outlined" onClick={onClear}>
          Clear all
        </Button>
      }
    >
      <Stack spacing={1.25}>
        <Typography variant="body2" color="text.secondary">
          Toggle a fault to enable or disable it. Defaults are applied automatically when no value is provided.
        </Typography>
        <Divider sx={{ borderColor: 'rgba(110,231,249,0.12)' }} />
        {orderedFaults.map((faultType) => {
          const fault = faultMap.get(faultType)
          const enabled = fault?.enabled ?? false
          return (
            <Stack key={faultType} direction="row" alignItems="center" justifyContent="space-between" spacing={1}>
              <FormControlLabel
                control={
                  <Switch
                    checked={enabled}
                    onChange={(_, checked) => onToggle(faultType, checked)}
                    color="primary"
                  />
                }
                label={faultLabels[faultType]}
              />
              <Chip
                size="small"
                label={enabled ? 'active' : 'idle'}
                color={enabled ? 'warning' : 'default'}
                variant={enabled ? 'filled' : 'outlined'}
              />
            </Stack>
          )
        })}
      </Stack>
    </PanelCard>
  )
}

