import React from 'react';
import { Box, Typography, Stack } from '@mui/material';
import type { FocState } from '../app/focMath';

export const DqGauges: React.FC<{ foc: FocState }> = ({ foc }) => {
  // Linear gauge for Id (Target 0, Range -10 to +10)
  // Linear gauge for Iq (Target Torque, Range -10 to +10)
  const renderGauge = (label: string, value: number, max: number, color: string) => {
    // map value from [-max, max] to [0%, 100%]
    const pct = Math.max(0, Math.min(100, ((value + max) / (2 * max)) * 100));
    
    return (
      <Box sx={{ mb: 2 }}>
        <Stack direction="row" justifyContent="space-between" alignItems="center" sx={{ mb: 0.5 }}>
          <Typography variant="body2" sx={{ color: 'text.secondary', fontWeight: 'bold' }}>{label}</Typography>
          <Typography variant="body2" sx={{ color: 'text.primary', fontFamily: 'monospace' }}>{value.toFixed(2)} A</Typography>
        </Stack>
        <Box sx={{ width: '100%', height: 24, bgcolor: '#0d1419', position: 'relative', borderRadius: 1, border: '1px solid #4b5257' }}>
          {/* Zero line */}
          <Box sx={{ position: 'absolute', left: '50%', top: 0, bottom: 0, width: 2, bgcolor: '#4b5257', zIndex: 2 }} />
          {/* Fill bar */}
          <Box 
            sx={{ 
              position: 'absolute', 
              top: 0, 
              bottom: 0, 
              left: value >= 0 ? '50%' : `${pct}%`,
              width: value >= 0 ? `${pct - 50}%` : `${50 - pct}%`,
              bgcolor: color,
              transition: 'all 0.1s linear',
              opacity: 0.8
            }} 
          />
        </Box>
        <Stack direction="row" justifyContent="space-between" sx={{ mt: 0.5 }}>
          <Typography variant="caption" sx={{ color: '#4b5257' }}>-{max}A</Typography>
          <Typography variant="caption" sx={{ color: '#4b5257' }}>0A</Typography>
          <Typography variant="caption" sx={{ color: '#4b5257' }}>+{max}A</Typography>
        </Stack>
      </Box>
    );
  };

  return (
    <Box sx={{ width: '100%', p: 1 }}>
      <Typography variant="subtitle2" sx={{ mb: 2, color: 'text.secondary' }}>
        Field Oriented Control Currents
      </Typography>
      {renderGauge('Id (Flux Current)', foc.Id, 10, '#c98b4d')}
      {renderGauge('Iq (Torque Current)', foc.Iq, 10, '#4caf50')}
      
      <Box sx={{ mt: 3, p: 2, bgcolor: 'rgba(255,255,255,0.03)', borderRadius: 2 }}>
        <Typography variant="body2" color="text.secondary">
          <strong>Efficiency insight:</strong> In a surface PMSM, keeping <code>Id</code> near zero ensures all stator current is converted directly into useful mechanical torque (<code>Iq</code>), minimizing waste heat.
        </Typography>
      </Box>
    </Box>
  );
};
