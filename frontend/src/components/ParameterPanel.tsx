import { Button, Grid, Stack, TextField } from '@mui/material'
import type { MotorParameters } from '../app/types'
import { PanelCard } from './PanelCard'

type ParameterPanelProps = {
  parameters: MotorParameters
  onChange: (next: MotorParameters) => void
  onSave: () => void
}

const fields: Array<{ key: keyof MotorParameters; label: string; step: string }> = [
  { key: 'Rs', label: 'Rs', step: '0.01' },
  { key: 'Ld', label: 'Ld', step: '0.0001' },
  { key: 'Lq', label: 'Lq', step: '0.0001' },
  { key: 'Kt', label: 'Kt', step: '0.0001' },
  { key: 'Ke', label: 'Ke', step: '0.0001' },
  { key: 'RotorInertia', label: 'J', step: '0.00001' },
  { key: 'Friction', label: 'Friction', step: '0.0001' },
  { key: 'LoadTorque', label: 'Load', step: '0.01' },
  { key: 'PolePairs', label: 'Pole pairs', step: '1' },
  { key: 'Vdc', label: 'Vdc', step: '0.1' },
]

export function ParameterPanel({ parameters, onChange, onSave }: ParameterPanelProps) {
  return (
    <PanelCard
      eyebrow="parameters"
      title="Parameter editor"
      action={
        <Button size="small" variant="contained" onClick={onSave}>
          Apply
        </Button>
      }
    >
      <Grid container spacing={1.5}>
        {fields.map((field) => (
          <Grid item xs={12} sm={6} key={field.key}>
            <TextField
              size="small"
              fullWidth
              label={field.label}
              type="number"
              inputProps={{ step: field.step }}
              value={parameters[field.key]}
              onChange={(event) =>
                onChange({
                  ...parameters,
                  [field.key]:
                    field.key === 'PolePairs'
                      ? Number.parseInt(event.target.value || '0', 10)
                      : Number(event.target.value),
                })
              }
            />
          </Grid>
        ))}
      </Grid>
    </PanelCard>
  )
}
