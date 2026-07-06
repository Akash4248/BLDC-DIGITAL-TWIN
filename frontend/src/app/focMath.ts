export interface FocState {
  Ialpha: number;
  Ibeta: number;
  Id: number;
  Iq: number;
  Valpha: number;
  Vbeta: number;
  Vd: number;
  Vq: number;
  sector: number;
  theta: number;
}

const SQRT3_2 = Math.sqrt(3) / 2;
const SQRT3 = Math.sqrt(3);

export function computeFocState(
  Ia: number,
  Ib: number,
  Ic: number,
  theta: number,
  DutyA: number,
  DutyB: number,
  DutyC: number,
  Vdc: number
): FocState {
  // 1. Clarke Transform (Current)
  // Ialpha = (2/3) * (Ia - 0.5*Ib - 0.5*Ic)
  // Ibeta = (2/3) * (SQRT3_2 * Ib - SQRT3_2 * Ic)
  // Power invariant form or simplified? Let's use standard amplitude invariant:
  const Ialpha = Ia;
  const Ibeta = (1 / SQRT3) * (Ia + 2 * Ib);

  // 2. Park Transform (Current)
  const cosTheta = Math.cos(theta);
  const sinTheta = Math.sin(theta);
  
  const Id = Ialpha * cosTheta + Ibeta * sinTheta;
  const Iq = -Ialpha * sinTheta + Ibeta * cosTheta;

  // 3. Reconstruct Phase Voltages from Duty Cycles
  const Va_n = (DutyA - 0.5) * Vdc;
  const Vb_n = (DutyB - 0.5) * Vdc;
  const Vc_n = (DutyC - 0.5) * Vdc;
  
  const v_neutral = (Va_n + Vb_n + Vc_n) / 3;
  const Va = Va_n - v_neutral;
  const Vb = Vb_n - v_neutral;

  // 4. Clarke Transform (Voltage)
  const Valpha = Va;
  const Vbeta = (1 / SQRT3) * (Va + 2 * Vb);

  // 5. Park Transform (Voltage)
  const Vd = Valpha * cosTheta + Vbeta * sinTheta;
  const Vq = -Valpha * sinTheta + Vbeta * cosTheta;

  // 6. SVPWM Sector calculation based on Valpha, Vbeta
  let angle = Math.atan2(Vbeta, Valpha);
  if (angle < 0) {
    angle += 2 * Math.PI;
  }
  
  let sector = Math.floor(angle / (Math.PI / 3)) + 1;
  if (sector > 6) sector = 1;

  return {
    Ialpha,
    Ibeta,
    Id,
    Iq,
    Valpha,
    Vbeta,
    Vd,
    Vq,
    sector,
    theta
  };
}
