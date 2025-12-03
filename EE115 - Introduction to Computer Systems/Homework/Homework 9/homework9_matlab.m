% Responses for EE115 Homework 9 PLL questions.
% Adjust the parameters below to match your loop design.

% Loop parameters
K  = 10;       % small-signal loop gain
Kv = 200;      % VCO gain (rad/s per volt)
a  = 500;      % filter zero for part (c)

% Excitation parameters
Delta_theta = 0.25;  % rad, phase step for part (a)
kf          = 1e3;   % Hz, frequency step magnitude for parts (b) and (c)

% Time base (long enough for exponentials to settle)
t = linspace(0, 5 / K, 600);

%% Part (a): theta_o(t) for a phase-step input
theta_o_a = Delta_theta * (1 - exp(-K * t));
figure(1);
plot(t, theta_o_a, 'LineWidth', 1.6);
xlabel('Time (s)');
ylabel('\theta_o(t) (rad)');
title('Part (a): Phase-step response');
grid on;

%% Part (b): frequency-step input (ramp in phase)
dthetao_dt_b = 2 * pi * kf * (1 - exp(-K * t));
x_b          = (2 * pi * kf / Kv) * (1 - exp(-K * t));
theta_e_b    = (2 * pi * kf / K) * (1 - exp(-K * t));

figure(2);
subplot(3, 1, 1);
plot(t, dthetao_dt_b, 'LineWidth', 1.4);
ylabel('d\theta_o/dt (rad/s)');
title('Part (b): Frequency-step response');
grid on;

subplot(3, 1, 2);
plot(t, x_b, 'LineWidth', 1.4);
ylabel('x(t) (V)');
grid on;

subplot(3, 1, 3);
plot(t, theta_e_b, 'LineWidth', 1.4);
xlabel('Time (s)');
ylabel('\theta_e(t) (rad)');
grid on;

%% Part (c): steady-state values for lead-lag filter G(s) = (s+a)/s
x_inf_c     = 2 * pi * kf / Kv;
theta_e_inf = 0;
fprintf('Part (c): x(∞) = %.4g V,  theta_e(∞) = %.4g rad\n', x_inf_c, theta_e_inf);
