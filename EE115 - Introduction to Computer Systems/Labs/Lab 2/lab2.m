%% EE115 Lab 2 - Envelope Detector Analysis
% This script walks through the design calculations and simulations
% requested in the lab handout. Run the whole file to generate the
% numerical answers and figures for Tasks 1-8.

clear;
close all;
clc;

%% Given parameters
fc = 10e6;              % Carrier frequency (Hz)
B = 10e3;               % Message bandwidth (Hz)
Rs = 1e-3;              % Diode on resistance (Ohm)
Rl = 5;                 % Load resistance (Ohm)

% Quantify "much less than" and "much greater than" for practical design.
muchLessFactor = 0.1;   % 10x smaller
muchGreaterFactor = 10; % 10x larger

fprintf('Carrier frequency fc = %.2e Hz\n', fc);
fprintf('Message bandwidth B = %.2e Hz\n', B);
fprintf('Rs = %.2e Ohm, Rl = %.2f Ohm\n\n', Rs, Rl);

%% Task 1: Proper range for Rs*C
RsC_upper = muchLessFactor / fc;
fprintf('Task 1: Rs*C should be well below 1/fc.\n');
fprintf('    Suggested upper bound: Rs*C <= %.3e s\n\n', RsC_upper);

%% Task 2: Proper range for Rl*C
RlC_lower = muchGreaterFactor / fc;
RlC_upper = muchLessFactor / B;
fprintf('Task 2: Rl*C must fall between the carrier and message time scales.\n');
fprintf('    Suggested range: %.3e s <= Rl*C <= %.3e s\n\n', RlC_lower, RlC_upper);

%% Task 3: Choose C that meets all conditions (given Rs and Rl)
C_lower = RlC_lower / Rl;
C_upper = min(RlC_upper / Rl, RsC_upper / Rs);
C_nominal = sqrt(C_lower * C_upper); % Geometric mean for a representative value

fprintf('Task 3: Feasible capacitor range based on the above constraints.\n');
fprintf('    %.3e F <= C <= %.3e F\n', C_lower, C_upper);
fprintf('    Using C_nominal = %.3e F for subsequent simulations.\n\n', C_nominal);

C_values = [C_lower, C_nominal, C_upper];
C_labels = ["C_{min}", "C_{nom}", "C_{max}"];

tau_charge = (Rs .* Rl .* C_values) ./ (Rs + Rl); % Effective RC when diode conducts
tau_discharge = Rl .* C_values;                   % RC when diode is off
RsC_values = Rs .* C_values;
RlC_values = Rl .* C_values;

task3_table = table(C_values.', RsC_values.', RlC_values.', tau_charge.', tau_discharge.', ...
    'VariableNames', {'C_F', 'RsC_s', 'RlC_s', 'tau_charge_s', 'tau_discharge_s'}, ...
    'RowNames', cellstr(C_labels.'));
disp('Summary of time constants for selected capacitor values:');
disp(task3_table);

%% Task 4: Equivalent circuits in charging and discharging modes
fprintf('Task 4:\n');
fprintf('    Charging mode: input step drives Rs in series with the parallel of C and Rl.\n');
fprintf('        Diode conducts (modeled by Rs), so the capacitor charges through Rs.\n');
fprintf('    Discharging mode: diode open-circuits, leaving only Rl and C in parallel.\n');
fprintf('        The capacitor voltage decays through Rl with time constant Rl*C.\n\n');

%% Task 5: Step response during charging (vo(t) while diode conducts)
vin_final = 1; % Unit-step magnitude
v_inf = vin_final * (Rl / (Rs + Rl));
t_charge = linspace(0, 10 * max(tau_charge), 2000);

figure('Name', 'Task 5: Charging Response');
hold on;
for idx = 1:numel(C_values)
    vo_charge = v_inf * (1 - exp(-t_charge / tau_charge(idx)));
    plot(t_charge * 1e6, vo_charge, 'DisplayName', sprintf('%s (C=%.2e F)', C_labels(idx), C_values(idx)));
end
grid on;
xlabel('Time (microseconds)');
ylabel('v_o(t) (V)');
title('Charging response to v_i(t) = u_1(t)');
legend('Location', 'southeast');
hold off;

fprintf('Task 5: Larger Rs*C (from larger C) produces a slower rise toward %.3f V.\n\n', v_inf);

%% Task 6: Free response during discharging (diode off)
v0 = 1; % Initial capacitor voltage
t_discharge = linspace(0, 5 * max(tau_discharge), 1000);

figure('Name', 'Task 6: Discharging Response');
hold on;
for idx = 1:numel(C_values)
    vo_discharge = v0 * exp(-t_discharge / tau_discharge(idx));
    plot(t_discharge * 1e3, vo_discharge, 'DisplayName', sprintf('%s (C=%.2e F)', C_labels(idx), C_values(idx)));
end
grid on;
xlabel('Time (milliseconds)');
ylabel('v_o(t) (V)');
title('Free response with diode open (v_o(0) = V)');
legend('Location', 'northeast');
hold off;

fprintf('Task 6: Increasing Rl*C stretches the decay, keeping v_o(t) near V longer.\n\n');

%% Task 7: Envelope of the AM waveform
A = 1;
a_mod = 0.5;
B_mod = 20e3;                 % Bandwidth of m_n(t)
time_window = 1e-3;           % Observation window (s)
fc_plot = 80e3;               % Carrier frequency for Task 8 (used to pick sampling rate)
f_max = fc_plot + B_mod;      % Highest significant frequency component
Fs = max(20 * f_max, 10 * B_mod); % Sampling rate (Hz), safely above 2*f_max
dt = 1 / Fs;
N = round(time_window / dt) + 1;
t = linspace(-time_window / 2, time_window / 2, N);

mn_arg = B_mod * t;
mn = ones(size(mn_arg));
nonzero = abs(mn_arg) > eps;
mn(nonzero) = sin(pi * mn_arg(nonzero)) ./ (pi * mn_arg(nonzero));
envelope = A * (1 + a_mod * mn);

figure('Name', 'Task 7: Envelope Only');
plot(t * 1e3, envelope, 'LineWidth', 1.25);
grid on;
xlabel('Time (milliseconds)');
ylabel('Envelope amplitude');
title('Envelope of x(t) for -0.5 ms < t < 0.5 ms');

fprintf('Task 7: Envelope computed as A*(1 + a_{mod}*m_n(t)).\n');
fprintf('    Samples generated with Fs = %.2e Hz (> 2*f_{max} = %.2e Hz).\n\n', Fs, 2 * f_max);

%% Task 8: Modulated waveform and comparison with its envelope
x_t = envelope .* cos(2 * pi * fc_plot * t);

figure('Name', 'Task 8: Signal and Envelope');
plot(t * 1e3, x_t, 'DisplayName', 'x(t)');
hold on;
plot(t * 1e3, envelope, 'k', 'LineWidth', 1.25, 'DisplayName', 'Envelope');
plot(t * 1e3, -envelope, 'k--', 'LineWidth', 1.0, 'DisplayName', '-Envelope');
hold off;
grid on;
xlabel('Time (milliseconds)');
ylabel('Amplitude');
title(sprintf('x(t) with f_c = %.0f kHz and its envelope', fc_plot / 1e3));
legend('Location', 'southwest');

fprintf('Task 8: Carrier frequency updated to %.2f kHz. x(t) aligns with its envelope bounds.\n', fc_plot / 1e3);
