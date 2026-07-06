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
  primary: '#6EE7F9',
  secondary: '#FFB86B',
  success: '#5FD38A',
  warning: '#FFB347',
  error: '#FF6B6B',
}

export function MetricTile({ label, value, suffix, tone = 'primary', icon }: MetricTileProps) {
  return (
    <Paper
      variant="outlined"
      sx={{
        p: 1.75,
        borderColor: 'rgba(110, 231, 249, 0.12)',
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

