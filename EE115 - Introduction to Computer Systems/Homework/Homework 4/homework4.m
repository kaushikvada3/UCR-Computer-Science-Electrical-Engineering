%% EE 115 Homework 4 – Spectral Sketch Helper
% Generates the spectra requested in Questions 1(a) and 2(a).
% Run the entire script to reproduce the plots used to sketch/draw the spectra.

%% Question 1(a): Spectrum of x_out(t) after the nonlinear device
alpha = 1.0;            % Adjust to match the nonlinear device gain α (>0)
beta = 0.5;             % Adjust to match the nonlinear device gain β (>0)
B = 1.5e3;              % Bandwidth of m(t) in Hz
fc = 4 * B;             % Carrier frequency for the AM stage
f = linspace(-12e3, 12e3, 6001);   % Frequency axis for plotting (Hz)

% Spectral building blocks
M_base = (1 / B) * double(abs(f) <= B);                    % M(f)
M_squared = (1 / B^2) * max(0, (2 * B - abs(f)));          % Convolution M(f)*M(f)
M_shift_pos = (1 / B) * double(abs(f - fc) <= B);          % M(f - fc)
M_shift_neg = (1 / B) * double(abs(f + fc) <= B);          % M(f + fc)

% Continuous-valued spectrum (everything except the spectral lines)
S_continuous = alpha * (M_squared + M_shift_pos + M_shift_neg) - beta * M_base;

% Discrete spectral lines (Dirac impulses)
impulse_freq = [-2 * fc, -fc, 0, fc, 2 * fc] / 1e3;        % Display in kHz
impulse_amp = [alpha / 4, -beta / 2, alpha / 2, -beta / 2, alpha / 4];

% Plot the components
figure('Name', 'Question 1(a) Spectrum');
tiledlayout(2, 1);

nexttile;
plot(f / 1e3, alpha * M_squared, '--', 'LineWidth', 1.1, 'DisplayName', '\alpha(M * M)');
hold on;
plot(f / 1e3, alpha * M_shift_pos, '--', 'LineWidth', 1.1, 'DisplayName', '\alpha M(f - f_c)');
plot(f / 1e3, alpha * M_shift_neg, '--', 'LineWidth', 1.1, 'DisplayName', '\alpha M(f + f_c)');
plot(f / 1e3, -beta * M_base, '--', 'LineWidth', 1.1, 'DisplayName', '-\beta M(f)');
plot(f / 1e3, S_continuous, 'k', 'LineWidth', 1.6, 'DisplayName', 'Total continuous part');
grid on;
xlabel('Frequency (kHz)');
ylabel('Amplitude');
title('Continuous component of X_{out}(f)');
legend('Location', 'northwest');
xlim([-12, 12]);

nexttile;
stem(impulse_freq, impulse_amp, 'filled', 'LineWidth', 1.4);
grid on;
xlabel('Frequency (kHz)');
ylabel('Impulse weight');
title('Spectral lines (Dirac impulses) in X_{out}(f)');
xlim([-12, 12]);

%% Question 2(a): Spectrum of u_2(t) after the bandpass filter
fc2 = 100;                                    % Carrier from u_1(t) = m(t) cos(200 pi t)
f_axis = linspace(-140, 140, 4001);           % Frequency axis (Hz)

% Spectrum of the DSB-SC signal u1(t)
U1 = 0.5 * (triangularSpectrum(f_axis - fc2) + triangularSpectrum(f_axis + fc2));

% Ideal bandpass filter |f - 105| <= 5 or |f + 105| <= 5
H = double((abs(f_axis - 105) <= 5) | (abs(f_axis + 105) <= 5));
U2 = U1 .* H;

figure('Name', 'Question 2(a) Spectrum');
plot(f_axis, U1, '--', 'LineWidth', 1.0, 'DisplayName', '|U_1(f)|');
hold on;
plot(f_axis, U2, 'LineWidth', 1.8, 'DisplayName', '|U_2(f)|');
grid on;
xlabel('Frequency (Hz)');
ylabel('Amplitude');
title('Spectrum after bandpass filtering');
legend('Location', 'north');
xlim([-130, 130]);

%% Helper: triangular baseband spectrum corresponding to M(f)
function spec = triangularSpectrum(freq)
    spec = zeros(size(freq));
    idx_neg = (freq >= -10) & (freq <= 0);
    spec(idx_neg) = 10 + freq(idx_neg);
    idx_pos = (freq >= 0) & (freq <= 10);
    spec(idx_pos) = 10 - freq(idx_pos);
end
