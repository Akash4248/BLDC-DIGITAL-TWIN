import { useMemo } from 'react'
import { Box, Button, Chip, Stack, Typography } from '@mui/material'
import RefreshIcon from '@mui/icons-material/Refresh'
import { PanelCard } from './PanelCard'
import type { LogEntry } from '../app/types'

type LoggerPanelProps = {
  entries: LogEntry[]
  loading?: boolean
  onRefresh: () => void
}

const levelColor: Record<string, 'default' | 'success' | 'warning' | 'error' | 'info'> = {
  info: 'info',
  warning: 'warning',
  error: 'error',
  debug: 'default',
}

function formatDetail(detail: Record<string, unknown>): string {
  const entries = Object.entries(detail)
  if (entries.length === 0) {
    return 'no detail'
  }

  return entries
    .map(([key, value]) => `${key}=${typeof value === 'object' ? JSON.stringify(value) : String(value)}`)
    .join('  ')
}

export function LoggerPanel({ entries, loading = false, onRefresh }: LoggerPanelProps) {
  const visibleEntries = useMemo(() => entries.slice(-12).reverse(), [entries])

  return (
    <PanelCard
      eyebrow="logger"
      title="Event stream"
      action={
        <Button size="small" startIcon={<RefreshIcon />} onClick={onRefresh} disabled={loading}>
          {loading ? 'Loading' : 'Refresh'}
        </Button>
      }
      height={420}
    >
      <Stack spacing={1} sx={{ maxHeight: 330, overflowY: 'auto', pr: 0.5 }}>
        {visibleEntries.length === 0 ? (
          <Box
            sx={{
              border: '1px dashed rgba(110,231,249,0.22)',
              borderRadius: 2,
              px: 2,
              py: 2.5,
              color: 'text.secondary',
            }}
          >
            <Typography variant="body2">No events yet. Control actions, faults, and simulation updates will appear here.</Typography>
          </Box>
        ) : (
          visibleEntries.map((entry) => (
            <Box
              key={`${entry.timestamp}-${entry.event}`}
              sx={{
                border: '1px solid rgba(255,255,255,0.06)',
                borderRadius: 2,
                px: 1.5,
                py: 1.25,
                background: 'rgba(255,255,255,0.02)',
              }}
            >
              <Stack direction="row" spacing={1} alignItems="center" sx={{ mb: 0.75, flexWrap: 'wrap' }}>
                <Chip size="small" label={entry.level} color={levelColor[entry.level.toLowerCase()] ?? 'default'} />
                <Typography variant="caption" color="text.secondary">
                  {entry.timestamp}
                </Typography>
                <Typography variant="subtitle2" sx={{ ml: 'auto' }}>
                  {entry.event}
                </Typography>
              </Stack>
              <Typography variant="body2" color="text.secondary" sx={{ fontFamily: 'ui-monospace, SFMono-Regular, Menlo, monospace' }}>
                {formatDetail(entry.detail)}
              </Typography>
            </Box>
          ))
        )}
      </Stack>
    </PanelCard>
  )
}
