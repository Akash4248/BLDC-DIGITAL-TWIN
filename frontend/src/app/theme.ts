import { createTheme } from '@mui/material/styles'

export const darkTheme = createTheme({
  palette: {
    mode: 'dark',
    primary: { main: '#C98B4D' },
    secondary: { main: '#B7C1C8' },
    background: {
      default: '#070B0E',
      paper: '#0D1216',
    },
    text: {
      primary: '#E8EDF1',
      secondary: '#94A2AC',
    },
    success: { main: '#B7C1C8' },
    warning: { main: '#D69A55' },
    error: { main: '#FF6A4D' },
  },
  typography: {
    fontFamily:
      '"IBM Plex Sans", "Segoe UI", "Helvetica Neue", Arial, sans-serif',
    h4: {
      fontWeight: 700,
      letterSpacing: '-0.03em',
    },
    h5: {
      fontWeight: 700,
      letterSpacing: '-0.02em',
    },
    h6: {
      fontWeight: 600,
    },
    subtitle2: {
      textTransform: 'uppercase',
      letterSpacing: '0.16em',
      fontSize: '0.72rem',
    },
    caption: {
      letterSpacing: '0.12em',
      textTransform: 'uppercase',
    },
  },
  shape: {
    borderRadius: 18,
  },
  components: {
    MuiCard: {
      styleOverrides: {
        root: {
          border: '1px solid rgba(201, 139, 77, 0.12)',
          boxShadow: '0 18px 60px rgba(0, 0, 0, 0.35)',
          backdropFilter: 'blur(10px)',
        },
      },
    },
    MuiPaper: {
      styleOverrides: {
        root: {
          backgroundImage:
            'linear-gradient(180deg, rgba(255,255,255,0.02), rgba(255,255,255,0))',
        },
      },
    },
    MuiButton: {
      styleOverrides: {
        root: {
          borderRadius: 999,
          textTransform: 'none',
          fontWeight: 600,
        },
      },
    },
  },
})
