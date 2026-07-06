import { Card, CardContent, Stack, Typography, Box } from '@mui/material'
import type { ReactNode } from 'react'

type PanelCardProps = {
  eyebrow: string
  title: string
  children: ReactNode
  action?: ReactNode
  height?: number | string
}

export function PanelCard({ eyebrow, title, children, action, height }: PanelCardProps) {
  return (
    <Card
      sx={{
        height: height ?? '100%',
        background:
          'linear-gradient(180deg, rgba(13,23,29,0.96), rgba(8,16,21,0.92))',
      }}
    >
      <CardContent sx={{ height: '100%' }}>
        <Stack direction="row" alignItems="flex-start" justifyContent="space-between" sx={{ mb: 2 }}>
          <Box>
            <Typography variant="subtitle2" color="text.secondary" sx={{ mb: 0.5 }}>
              {eyebrow}
            </Typography>
            <Typography variant="h6">{title}</Typography>
          </Box>
          {action}
        </Stack>
        {children}
      </CardContent>
    </Card>
  )
}

