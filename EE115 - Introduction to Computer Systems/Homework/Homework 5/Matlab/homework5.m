%% EE115 – Homework 5 Spectrum Sketching
clear; close all; clc;

%% TIME AXIS (t is in milliseconds)
Fs = 500e3;      % sampling freq 500 kHz
Ts = 1/Fs;
t  = 0:Ts:5e-3;  % simulate 5 ms

N = length(t);
f_full = linspace(-Fs/2, Fs/2, N);

figDir = fullfile(pwd, 'figures_hw5');
if ~exist(figDir, 'dir')
    mkdir(figDir);
end

%% ===============================================================
% (a) m(t) = cos(2πt) + 2 sin(4πt)
% ===============================================================
m = cos(2*pi*1e3*t) + 2*sin(2*pi*2e3*t);

[M,f] = compute_fft(m, Fs);

figM = figure;
plot(f, abs(M), 'LineWidth', 1.5);
title('M(f)'); xlabel('Frequency (Hz)'); ylabel('|M(f)|');
xlim([0 10e3]);
save_plot(figM, 'M_spectrum.png', figDir);

%% ===============================================================
% (b) u(t) = (m(t)+3)*cos(200π t)
% carrier = 100 kHz
% ===============================================================
carrier = cos(2*pi*100e3*t);
u = (m + 3).*carrier;

[U,f] = compute_fft(u, Fs);
U_full = fftshift(fft(u));

figU = figure;
plot(f, abs(U), 'LineWidth', 1.5);
title('U(f)'); xlabel('Frequency (Hz)'); ylabel('|U(f)|');
xlim([0 130e3]);
save_plot(figU, 'U_spectrum.png', figDir);

%% ===============================================================
% (c) Bandpass filter spectrum H(f) from problem
% 98–102 kHz: (f-98)/4
% 102–103 kHz: 1
% ===============================================================
H_pos = zeros(size(f));

idx1 = (f >= 98e3 & f <= 102e3);
H_pos(idx1) = (f(idx1)/1e3 - 98)/4;

idx2 = (f >= 102e3 & f <= 103e3);
H_pos(idx2) = 1;

V_pos = U .* H_pos;

figV = figure;
plot(f, abs(V_pos), 'LineWidth', 1.5);
title('V(f)'); xlabel('Frequency (Hz)'); ylabel('|V(f)|');
xlim([90e3 110e3]);
save_plot(figV, 'V_spectrum.png', figDir);

% Two-sided response for actual filtering
H_full = zeros(size(f_full));
abs_khz = abs(f_full)/1e3;

mask_ramp = (abs_khz >= 98) & (abs_khz <= 102);
H_full(mask_ramp) = (abs_khz(mask_ramp) - 98)/4;

mask_flat = (abs_khz > 102) & (abs_khz <= 103);
H_full(mask_flat) = 1;

V_full = U_full .* H_full;

% Impulse response of BPF (needed later)
h_time = ifft(ifftshift(H_full), 'symmetric');

%% ===============================================================
% (d) Complex envelope of h(t)
% Baseband filter = shift H(f) around 100 kHz down to 0
% ===============================================================
% Extract region around carrier
mask = (f >= 97e3 & f <= 105e3);
fb = f(mask) - 100e3;    % shift to baseband
Hb = H_pos(mask);

figHb = figure;
plot(fb, Hb, 'LineWidth', 1.5);
title('Envelope Spectrum of h(t)'); xlabel('Baseband f (Hz)'); ylabel('|H_env(f)|');
xlim([-3e3 3e3]);
save_plot(figHb, 'H_envelope_baseband.png', figDir);

%% ===============================================================
% (e) Extract v_c(t) and v_s(t)
% We must reconstruct v(t) in *time domain* first
% ===============================================================

v_time = ifft(ifftshift(V_full), 'symmetric');

% Demodulate to baseband (correct)
v_env = v_time .* exp(-1j * 2*pi*100e3 * t);

v_c = real(v_env);
v_s = -imag(v_env);

[Vc,f] = compute_fft(v_c, Fs);
[Vs,~] = compute_fft(v_s, Fs);

figVc = figure;
plot(f, abs(Vc)); title('Spectrum of v_c(t)');
xlabel('Frequency (Hz)'); ylabel('|V_c(f)|'); xlim([0 10e3]);

save_plot(figVc, 'Vc_spectrum.png', figDir);

figVs = figure;
plot(f, abs(Vs)); title('Spectrum of v_s(t)');
xlabel('Frequency (Hz)'); ylabel('|V_s(f)|'); xlim([0 10e3]);
save_plot(figVs, 'Vs_spectrum.png', figDir);

%% ===============================================================
% (f) v_c(t) vs m(t)
% (No plotting needed, but you can uncomment:)
% figure; plot(t, v_c); hold on; plot(t,m); legend("v_c","m");
% ===============================================================

%% ===============================================================
% (g) h(t) = h_c(t) cos(...) – h_s(t) sin(...)
% Compute h_c(t), h_s(t) from the envelope
% ===============================================================

h_env = h_time .* exp(-1j * 2*pi*100e3 * t);

h_c = real(h_env);
h_s = -imag(h_env);

[Hc,f2] = compute_fft(h_c, Fs);
[Hs,~]  = compute_fft(h_s, Fs);

figHc = figure;
plot(f2, abs(Hc));
title('Spectrum of h_c(t)'); xlabel('Frequency'); ylabel('|H_c(f)|'); xlim([0 10e3]);

save_plot(figHc, 'Hc_spectrum.png', figDir);

figHs = figure;
plot(f2, abs(Hs));
title('Spectrum of h_s(t)'); xlabel('Frequency'); ylabel('|H_s(f)|'); xlim([0 10e3]);
save_plot(figHs, 'Hs_spectrum.png', figDir);

%% ======================================================================
% Helper function
%% ======================================================================
function [X,f] = compute_fft(x, Fs)
    N = length(x);
    Xf = fftshift(fft(x));
    mid = floor(N/2) + 1;
    X  = Xf(mid:end);   % keep positive freqs
    max_mag = max(abs(X));
    if max_mag ~= 0
        X = X / max_mag; % normalize
    end
    f  = linspace(0, Fs/2, length(X));
end

function save_plot(figHandle, fileName, figDir)
    if ~isfolder(figDir)
        mkdir(figDir);
    end
    exportgraphics(figHandle, fullfile(figDir, fileName), 'Resolution', 300);
end
