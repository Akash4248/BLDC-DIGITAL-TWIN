import React from 'react';
import { Box, Typography, Stack } from '@mui/material';
import type { TelemetrySnapshot } from '../app/types';

interface OscilloscopeProps {
  history: TelemetrySnapshot[];
}

export const GateOscilloscope: React.FC<OscilloscopeProps> = ({ history }) => {
  // We will draw 6 square waves for AH, AL, BH, BL, CH, CL
  // Since we don't have the microsecond exact switching, we can approximate the duty cycle
  // as a PWM waveform or simply show the duty cycle as a continuous value.
  // The user asked for "PWM Waveforms (Live oscilloscope. AH ████ BL ██ CH ████)".
  // We can simulate a high-frequency PWM visual by thresholding a fast sawtooth, 
  // but just plotting the duty cycle as a block width per time step is easier.
  // Actually, since this is an educational twin, let's draw actual square waves 
  // for the current snapshot's duty cycle.

  const currentPwm = history[history.length - 1]?.pwm ?? { ah: 0, al: 0, bh: 0, bl: 0, ch: 0, cl: 0 };

  const renderPwmBar = (label: string, duty: number, color: string) => {
    // duty is 0.0 to 1.0
    const widthPct = Math.max(0, Math.min(100, duty * 100));
    return (
      <Box sx={{ display: 'flex', alignItems: 'center', mb: 1 }}>
        <Typography sx={{ width: 30, color: 'text.secondary', fontSize: 12 }}>{label}</Typography>
        <Box sx={{ flexGrow: 1, height: 16, bgcolor: '#0d1419', position: 'relative', overflow: 'hidden', borderRadius: 1 }}>
          <Box sx={{ position: 'absolute', left: 0, top: 0, bottom: 0, width: `${widthPct}%`, bgcolor: color, transition: 'width 0.1s linear' }} />
        </Box>
        <Typography sx={{ width: 40, textAlign: 'right', color: 'text.secondary', fontSize: 12 }}>
          {widthPct.toFixed(0)}%
        </Typography>
      </Box>
    );
  };

  return (
    <Box sx={{ width: '100%' }}>
      <Typography variant="subtitle2" sx={{ mb: 2, color: 'text.secondary' }}>
        Live PWM Gate Pulses
      </Typography>
      <Stack spacing={0.5}>
        {renderPwmBar('AH', currentPwm.ah, '#c98b4d')}
        {renderPwmBar('AL', currentPwm.al, '#4b5257')}
        {renderPwmBar('BH', currentPwm.bh, '#c98b4d')}
        {renderPwmBar('BL', currentPwm.bl, '#4b5257')}
        {renderPwmBar('CH', currentPwm.ch, '#c98b4d')}
        {renderPwmBar('CL', currentPwm.cl, '#4b5257')}
      </Stack>
    </Box>
  );
};
