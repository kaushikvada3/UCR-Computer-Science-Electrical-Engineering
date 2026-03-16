% EE 115 Lab 3: DSB-SC Demodulator Simulation
% This script generates the message, modulated, and demodulated signals,
% examines their spectra, and evaluates low-pass filter choices per the lab.

clear; close all; clc;

%% Signal parameters
fs = 50e3;                 % sampling rate in Hz
Ts = 1 / fs;               % sampling interval in seconds
Ts_ms = Ts * 1e3;          % sampling interval in milliseconds
fc = 10e3;                 % carrier frequency in Hz
N_dft = 64;                % samples considered in the DFT step
n_dft = 0:(N_dft - 1);     % sample indices for the first 64 samples

% LPF bandwidths W to evaluate (in kHz)
W_choices = [0.6, 1.0, 2.5];
L_vals = ceil(8 ./ (Ts_ms .* W_choices));    % filter lengths per Eq. (5)
max_n = max([N_dft - 1, L_vals]);            % ensure we have enough samples

% Discrete-time grid for all signals
n_all = 0:max_n;
t_all_s = n_all * Ts;
t_all_ms = t_all_s * 1e3;

%% Message and modulated signals
m_all = zeros(size(n_all));
msg_support = t_all_ms <= 1;                 % m(t) = sin(pi t) for 0 <= t <= 1 ms
m_all(msg_support) = sin(pi * t_all_ms(msg_support));

carrier = cos(2 * pi * fc * t_all_s);
u_all = m_all .* carrier;                    % transmitted DSB-SC signal
v_all = 2 * u_all .* carrier;                % mixer output inside demodulator

%% Step 1: Plot m[n], u[n], v[n] for n = 0,...,63
fprintf('Sampling interval Ts = %.2f microseconds per sample.\\n', Ts * 1e6);

n_plot = n_dft;
t_plot_ms = n_plot * Ts_ms;

figure('Name', 'Time-Domain Signals', 'NumberTitle', 'off');
tiledlayout(3, 1, 'TileSpacing', 'compact');
sgtitle({'Question 1: Time-Domain Signals', ...
         sprintf('n increment = %.2f microseconds per sample', Ts * 1e6)});

nexttile;
stem(n_plot, m_all(n_plot + 1), 'filled', 'LineWidth', 1.0);
grid on;
xlabel('n (samples)');
ylabel('m[n]');
title('Message Samples m[n]');

nexttile;
stem(n_plot, u_all(n_plot + 1), 'filled', 'LineWidth', 1.0);
grid on;
xlabel('n (samples)');
ylabel('u[n]');
title('DSB-SC Signal u[n]');

nexttile;
stem(n_plot, v_all(n_plot + 1), 'filled', 'LineWidth', 1.0);
grid on;
xlabel('n (samples)');
ylabel('v[n]');
title('Mixer Output v[n]');

%% Step 2: Amplitude spectra of m[n], u[n], v[n]
M = fft(m_all(1:N_dft), N_dft);
U = fft(u_all(1:N_dft), N_dft);
V = fft(v_all(1:N_dft), N_dft);

k_plot = -N_dft/2:(N_dft/2 - 1);                     % corresponds to -32 <= k <= 31
freq_axis_khz = (k_plot * fs / N_dft) / 1e3;         % frequency axis in kHz

M_mag = abs(fftshift(M));
U_mag = abs(fftshift(U));
V_mag = abs(fftshift(V));

figure('Name', 'Amplitude Spectra', 'NumberTitle', 'off');
tiledlayout(3, 1, 'TileSpacing', 'compact');
sgtitle({'Question 2: Amplitude Spectra', ...
         'Use these plots to discuss message bandwidth and shifted replicas'});

nexttile;
stem(freq_axis_khz, M_mag, 'filled', 'LineWidth', 1.0);
grid on;
xlabel('Frequency (kHz)');
ylabel('|M[k]|');
title('Amplitude Spectrum of m[n]');

nexttile;
stem(freq_axis_khz, U_mag, 'filled', 'LineWidth', 1.0);
grid on;
xlabel('Frequency (kHz)');
ylabel('|U[k]|');
title('Amplitude Spectrum of u[n]');

nexttile;
stem(freq_axis_khz, V_mag, 'filled', 'LineWidth', 1.0);
grid on;
xlabel('Frequency (kHz)');
ylabel('|V[k]|');
title('Amplitude Spectrum of v[n]');

%% Step 3 & 4: Low-pass filtering for different choices of W
for idx = 1:numel(W_choices)
    W = W_choices(idx);                % LPF one-sided bandwidth in kHz
    L = L_vals(idx);                   % filter length corresponding to W
    n_h = 0:L;
    t_h_ms = n_h * Ts_ms;

    % Continuous-time impulse response samples (causal windowed-sinc)
    h_cont = zeros(size(t_h_ms));
    support_mask = t_h_ms <= (8 / W) + 1e-12;
    t_supported = t_h_ms(support_mask);
    window = 0.5 + 0.5 * cos(pi * W / 4 .* (t_supported - 4 / W));
    h_supported = W * sinc(W * t_supported - 4) .* window;
    h_cont(support_mask) = h_supported;

    % Discrete-time impulse response per Eq. (5)
    h_tilde = (1 / W) * h_cont;

    % Discrete convolution x[n] = sum_{l=0}^L h_tilde[l] v[n - l]
    x_full = conv(v_all, h_tilde, 'full');
    x = x_full(1:(L + 1));             % retain n = 0,...,L
    n_x = 0:L;
    t_x_ms = n_x * Ts_ms;

    % Reference message samples for comparison
    m_ref = m_all(1:(L + 1));

    % Filter group delay (center of the windowed pulse)
    delay_ms = 4 / W;
    delay_samples = round(delay_ms / Ts_ms);

    if delay_samples < numel(x)
        x_aligned = [x(delay_samples + 1:end), zeros(1, delay_samples)];
    else
        x_aligned = zeros(size(x));
    end

    alignment_error = x_aligned - m_ref;
    rms_error = sqrt(mean(alignment_error .^ 2));

    fprintf(['W = %.1f kHz -> filter length L = %d samples, approx. delay = %d samples ' ...
             '(%.2f ms), aligned RMS error = %.3e\\n'], ...
            W, L, delay_samples, delay_samples * Ts_ms, rms_error);

    figure('Name', sprintf('LPF Output vs Message (W = %.1f kHz)', W), 'NumberTitle', 'off');
    tiledlayout(2, 1, 'TileSpacing', 'compact');
    sgtitle({sprintf('Question 3 & 4: LPF Output (W = %.1f kHz)', W), ...
             'Evaluate demodulation fidelity before and after delay alignment'});

    nexttile;
    plot(t_x_ms, m_ref, 'LineWidth', 1.2); hold on;
    plot(t_x_ms, x, '--', 'LineWidth', 1.2);
    hold off; grid on;
    xlabel('Time (ms)');
    ylabel('Amplitude');
    title(sprintf('x[n] vs. m[n] (W = %.1f kHz)', W));
    legend('m[n]', 'x[n]', 'Location', 'best');

    nexttile;
    plot(t_x_ms, m_ref, 'LineWidth', 1.2); hold on;
    plot(t_x_ms, x_aligned, '--', 'LineWidth', 1.2);
    hold off; grid on;
    xlabel('Time (ms)');
    ylabel('Amplitude');
    title(sprintf('x[n] aligned with m[n] (Delay = %.2f ms)', delay_samples * Ts_ms));
    legend('m[n]', 'x[n] (aligned)', 'Location', 'best');
end
