% lab6_plots.m — generate theta_e(t) plots for selected (K, a) pairs
clear; clc; close all;

kf = 1;          % replace with lab-specified value if different
t_end = 5;       % extend if the response has not settled
N = 2000;
t = linspace(0, t_end, N);

params_K = [0.5 1; 1 1; 1.5 1];   % vary K, fixed a=1
params_a = [1 0.5; 1 1; 1 2];     % vary a, fixed K=1

plot_family(params_K, t, kf, 'Theta\_e(t) with a=1, varying K', 'theta_e_vary_K');
plot_family(params_a, t, kf, 'Theta\_e(t) with K=1, varying a', 'theta_e_vary_a');

function plot_family(params, t, kf, titleText, fname)
  figure('Name', titleText);
  hold on; grid on;
  for i = 1:size(params,1)
    K = params(i,1);
    a = params(i,2);
    disc = 4*K*a - K^2;
    if disc <= 0
      warning('Skipping K=%.3g, a=%.3g (4Ka-K^2 <= 0)', K, a);
      continue;
    end
    omega_d = sqrt(disc);
    theta_e = 4*pi*kf/omega_d .* exp(-0.5*K*t) .* sin(omega_d*t);
    plot(t, theta_e, 'DisplayName', sprintf('K=%.2f, a=%.2f', K, a));

    % Quick metrics for discussion
    peak_val = max(theta_e);
    within = abs(theta_e) <= 0.02*peak_val;
    last_violation = find(~within, 1, 'last');
    if isempty(last_violation)
      t_settle = t(1);
    elseif last_violation < numel(t)
      t_settle = t(last_violation+1);
    else
      t_settle = NaN;
    end
    [~, locs] = findpeaks(theta_e, t);
    if numel(locs) >= 2
      osc_freq = 1/mean(diff(locs));
    else
      osc_freq = NaN;
    end
    fprintf('K=%.2f, a=%.2f | peak=%.3f, t_settle~=%.3f, f_osc~=%.3f\n', ...
            K, a, peak_val, t_settle, osc_freq);
  end
  xlabel('Time (s)');
  ylabel('\theta_e(t) (rad)');
  title(titleText);
  legend('Location','northeast');
  print(fname, '-dpng', '-r300');
end
