import { createTheme } from '@mui/material/styles'

export const darkTheme = createTheme({
  palette: {
    mode: 'dark',
    primary: { main: '#6EE7F9' },
    secondary: { main: '#FFB86B' },
    background: {
      default: '#081015',
      paper: '#0D171D',
    },
    text: {
      primary: '#E7F1F6',
      secondary: '#86A4B2',
    },
    success: { main: '#5FD38A' },
    warning: { main: '#FFB347' },
    error: { main: '#FF6B6B' },
  },
  typography: {
    fontFamily:
      '"IBM Plex Sans", "Inter", "Segoe UI", "Helvetica Neue", Arial, sans-serif',
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
          border: '1px solid rgba(126, 210, 233, 0.12)',
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

