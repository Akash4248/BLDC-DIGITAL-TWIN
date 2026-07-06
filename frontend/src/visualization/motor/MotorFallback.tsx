import { Box, Stack, Typography } from '@mui/material'

export function MotorFallback() {
  return (
    <Box
      sx={{
        position: 'relative',
        height: '100%',
        minHeight: 320,
        borderRadius: 4,
        overflow: 'hidden',
        background:
          'radial-gradient(circle at 50% 40%, rgba(110,231,249,0.10), transparent 28%), linear-gradient(180deg, rgba(12,20,26,0.96), rgba(6,11,16,0.98))',
        border: '1px solid rgba(110,231,249,0.12)',
      }}
    >
      <Box
        sx={{
          position: 'absolute',
          inset: 0,
          backgroundImage:
            'linear-gradient(rgba(255,255,255,0.03) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,0.03) 1px, transparent 1px)',
          backgroundSize: '28px 28px',
          opacity: 0.35,
        }}
      />
      <Stack
        alignItems="center"
        justifyContent="center"
        sx={{
          position: 'relative',
          zIndex: 1,
          height: '100%',
          px: 3,
          textAlign: 'center',
        }}
        spacing={2}
      >
        <Box
          sx={{
            width: 180,
            height: 180,
            borderRadius: '50%',
            background:
              'radial-gradient(circle at 50% 50%, rgba(110,231,249,0.18), rgba(255,255,255,0.02) 55%, rgba(0,0,0,0.35) 100%)',
            border: '1px solid rgba(110,231,249,0.14)',
            boxShadow: '0 0 0 1px rgba(110,231,249,0.06), inset 0 0 40px rgba(110,231,249,0.08)',
          }}
        />
        <Stack spacing={0.5}>
          <Typography variant="h6">Motor asset unavailable</Typography>
          <Typography variant="body2" color="text.secondary">
            The GLB viewer could not load. The dashboard remains operational, and the twin data is still flowing.
          </Typography>
        </Stack>
      </Stack>
    </Box>
  )
}

