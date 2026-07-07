export class MockEngine {
  private lastTime: number = 0;
  
  // State
  private angle: number = 0;
  private speed: number = 0; // rad/s
  private integralSpeedError: number = 0;

  // Motor params
  private polePairs = 4;
  private Rs = 0.5;
  private L = 0.001; // Ld = Lq
  private fluxLinkage = 0.02;
  private inertia = 0.01;
  private friction = 0.005;
  
  // PI tuning
  private kp = 0.1;
  private ki = 0.5;

  private currentSequence = 0;

  public step(dt: number, loadTorque: number, targetRpm: number = 3000) {
    const targetSpeed = targetRpm * (2 * Math.PI / 60);
    
    // 1. FOC Speed Controller (PI)
    const error = targetSpeed - this.speed;
    this.integralSpeedError += error * dt;
    
    // Anti-windup
    this.integralSpeedError = Math.max(-50, Math.min(50, this.integralSpeedError));
    
    let iqRef = (this.kp * error) + (this.ki * this.integralSpeedError);
    const maxIq = 15;
    iqRef = Math.max(-maxIq, Math.min(maxIq, iqRef));
    
    const idRef = 0;

    // 2. Physics: Torque Generation
    // Te = 1.5 * p * flux * iq (assuming id = 0 and Ld=Lq)
    const electromagneticTorque = 1.5 * this.polePairs * this.fluxLinkage * iqRef;
    
    // 3. Physics: Mechanical Equation
    const frictionTorque = this.speed * this.friction;
    const netTorque = electromagneticTorque - loadTorque - frictionTorque;
    
    const accel = netTorque / this.inertia;
    this.speed += accel * dt;
    this.angle += this.speed * dt;
    
    // Wrap angle
    this.angle = this.angle % (Math.PI * 2);
    if (this.angle < 0) this.angle += Math.PI * 2;
    
    const electricalAngle = (this.angle * this.polePairs) % (Math.PI * 2);
    
    // 4. Compute Currents (Inverse Park / Clarke)
    // Id = 0, Iq = iqRef -> Ialpha, Ibeta
    const Ialpha = idRef * Math.cos(electricalAngle) - iqRef * Math.sin(electricalAngle);
    const Ibeta  = idRef * Math.sin(electricalAngle) + iqRef * Math.cos(electricalAngle);
    
    const ia = Ialpha;
    const ib = -0.5 * Ialpha + (Math.sqrt(3)/2) * Ibeta;
    const ic = -0.5 * Ialpha - (Math.sqrt(3)/2) * Ibeta;

    // 5. Compute Voltages & Duty Cycles (Steady state equations)
    const omega_e = this.speed * this.polePairs;
    const vq = iqRef * this.Rs + omega_e * this.fluxLinkage;
    const vd = -omega_e * this.L * iqRef; // Rs * Id is 0
    
    const Valpha = vd * Math.cos(electricalAngle) - vq * Math.sin(electricalAngle);
    const Vbeta  = vd * Math.sin(electricalAngle) + vq * Math.cos(electricalAngle);
    
    const va = Valpha;
    const vb = -0.5 * Valpha + (Math.sqrt(3)/2) * Vbeta;
    const vc = -0.5 * Valpha - (Math.sqrt(3)/2) * Vbeta;
    
    const vdc = 24.0;
    // Map to duty cycles (SVPWM simplified to SPWM for mock)
    const duty_a = Math.max(0, Math.min(1, (va / vdc) + 0.5));
    const duty_b = Math.max(0, Math.min(1, (vb / vdc) + 0.5));
    const duty_c = Math.max(0, Math.min(1, (vc / vdc) + 0.5));

    this.currentSequence++;
    
    return {
      telemetry: {
        sequence: this.currentSequence,
        timestamp_us: this.currentSequence * 40000, // dt = 40ms
        duty_a,
        duty_b,
        duty_c,
        vdc
      },
      current: {
        ia, ib, ic,
        adc_a: Math.round((ia + 12) * 120),
        adc_b: Math.round((ib + 12) * 120),
        adc_c: Math.round((ic + 12) * 120),
      },
      speed: this.speed, // rad/s
      torque: electromagneticTorque,
      rotor_angle: this.angle,
      hall: { a: 0, b: 0, c: 0 },
      encoder: { a: 0, b: 0, index: 0 },
      temperature: 40 + Math.abs(iqRef) * 0.5,
    };
  }
}

export const globalMockEngine = new MockEngine();
