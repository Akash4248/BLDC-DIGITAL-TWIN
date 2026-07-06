import { Paper, Stack, Typography } from '@mui/material'
import type { ReactNode } from 'react'

type MetricTileProps = {
  label: string
  value: string
  suffix?: string
  tone?: 'primary' | 'secondary' | 'success' | 'warning' | 'error'
  icon?: ReactNode
}

const toneColors: Record<NonNullable<MetricTileProps['tone']>, string> = {
  primary: '#D1D7DC',
  secondary: '#C98B4D',
  success: '#AAB4BC',
  warning: '#D69A55',
  error: '#FF6A4D',
}

export function MetricTile({ label, value, suffix, tone = 'primary', icon }: MetricTileProps) {
  return (
    <Paper
      variant="outlined"
      sx={{
        p: 1.75,
        borderColor: 'rgba(201, 139, 77, 0.12)',
        background: 'rgba(255,255,255,0.02)',
      }}
    >
      <Stack direction="row" justifyContent="space-between" alignItems="center" spacing={1}>
        <Typography variant="caption" color="text.secondary">
          {label}
        </Typography>
        {icon}
      </Stack>
      <Stack direction="row" alignItems="baseline" spacing={0.5} sx={{ mt: 0.5 }}>
        <Typography variant="h5" sx={{ color: toneColors[tone], lineHeight: 1 }}>
          {value}
        </Typography>
        {suffix ? (
          <Typography variant="body2" color="text.secondary">
            {suffix}
          </Typography>
        ) : null}
      </Stack>
    </Paper>
  )
}
