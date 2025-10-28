% ====== Problem 2: m(t) = 2*sinc(2t) ======
% Parts (b) and (c): sketch M(f), \hat{M}(f), m(t), \hat{m}(t)
% Conventions: sinc(x) in MATLAB is sin(pi*x)/(pi*x)
% Pair used: F{sinc(a t)} = (1/a) rect(f/a)
% Here: F{2*sinc(2t)} = 2*(1/2) rect(f/2) = rect(f/2)

clear; close all; clc;

% ----------------- helpers -----------------
rect = @(x) double(abs(x) < 0.5);   % 1 for |x|<1/2, 0 else (ignore boundary)

% ----------------- frequency-domain plots -----------------
f = linspace(-3, 3, 4001);

M  = rect(f/2);                      % height 1 on |f|<1
Mhat_im = -sign(f) .* M;             % \hat{M}(f) = -j*sgn(f)*M(f)
Mhat_re = zeros(size(f));            % purely imaginary

% (b) Sketch M(f) and \hat{M}(f)
figure('Name','(b) Spectra M(f) and \hat{M}(f)');

subplot(2,2,1);
plot(f, real(M), 'LineWidth', 1.6); grid on;
ylim([-0.1 1.1]); xlim([min(f) max(f)]);
title('M(f): Real'); xlabel('f (Hz)'); ylabel('Amplitude');

subplot(2,2,2);
plot(f, imag(M), 'LineWidth', 1.6); grid on;
ylim([-0.1 0.1]); xlim([min(f) max(f)]);
title('M(f): Imag (zero)'); xlabel('f (Hz)'); ylabel('Amplitude');

subplot(2,2,3);
plot(f, Mhat_re, 'LineWidth', 1.6); grid on;
ylim([-0.1 0.1]); xlim([min(f) max(f)]);
title('\hat{M}(f): Real (zero)'); xlabel('f (Hz)'); ylabel('Amplitude');

subplot(2,2,4);
plot(f, Mhat_im, 'LineWidth', 1.6); grid on;
ylim([-1.2 1.2]); xlim([min(f) max(f)]);
title('\hat{M}(f): Imag = -sgn(f)\cdot M(f)'); xlabel('f (Hz)'); ylabel('Amplitude');

% ----------------- time-domain plots -----------------
t = linspace(-5, 5, 20001);
m = 2 * sinc(2*t);                   % m(t)

% \hat{m}(t) from the inverse transform in your writeup:
% \hat{m}(t) = [1 - cos(2*pi*t)] / (pi*t), with value 0 at t=0 by continuity
mhat = (1 - cos(2*pi*t)) ./ (pi*t);
mhat(t==0) = 0;                      % define the removable singularity

% (Optional) numeric check using discrete Hilbert transform
% hnum = imag(hilbert(m));

figure('Name','(c) Time signals m(t) and \hat{m}(t)');
subplot(2,1,1);
plot(t, m, 'LineWidth', 1.4); grid on; xlim([min(t) max(t)]);
title('m(t) = 2\sinc(2t)'); xlabel('t'); ylabel('Amplitude');

subplot(2,1,2);
plot(t, mhat, 'LineWidth', 1.4); grid on; xlim([min(t) max(t)]);
title('\hat{m}(t) = \left[1 - \cos(2\pi t)\right]/(\pi t), \ \hat{m}(0)=0');
xlabel('t'); ylabel('Amplitude');

% If you want to visually confirm the analytic \hat{m}(t):
% hold on; plot(t, hnum, '--'); legend('analytic','numeric hilbert'); hold off;