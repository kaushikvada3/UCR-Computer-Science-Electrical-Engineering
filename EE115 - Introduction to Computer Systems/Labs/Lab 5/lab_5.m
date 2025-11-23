% EE 115 Lab 5 - FM truncation power error and bandwidth
clear; close all; clc;

betas = [0.5 2 5 10];
Kmax = 50;
K = 0:Kmax;
err = zeros(numel(betas), numel(K));

for idx = 1:numel(betas)
    beta = betas(idx);
    J = besselj(0:Kmax, beta);              % J_n for n = 0..Kmax
    for k = 0:Kmax
        power = J(1)^2 + 2*sum(J(2:k+1).^2);% P_{u_K} from the truncated series
        err(idx, k+1) = 1 - power;          % truncation power error
    end
end

% Linear plot
figure;
hold on; grid on;
colors = lines(numel(betas));
for idx = 1:numel(betas)
    plot(K, err(idx, :), 'LineWidth', 2, 'Color', colors(idx, :));
end
xlabel('K'); ylabel('1 - P_{u_K}');
title('Truncation Power Error vs. K (Linear)');
legend('\beta = 0.5','\beta = 2.0','\beta = 5.0','\beta = 10.0','Location','northeast');
set(gca,'YLim',[0 1]);
exportgraphics(gcf,'image1.png','Resolution',300);

% dB plot
figure;
hold on; grid on;
floorVal = eps; % avoid log(0)
for idx = 1:numel(betas)
    plot(K, 10*log10(max(err(idx, :), floorVal)), ...
         'LineWidth', 2, 'Color', colors(idx, :));
end
xlabel('K'); ylabel('10 log_{10}(1 - P_{u_K}) (dB)');
title('Truncation Power Error vs. K (Log Scale)');
legend('\beta = 0.5','\beta = 2.0','\beta = 5.0','\beta = 10.0','Location','northeast');
set(gca,'YLim',[-160 0]);
exportgraphics(gcf,'image2.png','Resolution',300);

% Optional: print thresholds that appear in the report table
thresholds = [1e-2 1e-3 1e-6 1e-12];
fprintf('beta  K(@<=1e-2) K(@<=1e-3) K(@<=1e-6) K(@<=1e-12)\n');
for idx = 1:numel(betas)
    line = zeros(1, numel(thresholds));
    for t = 1:numel(thresholds)
        kHit = find(err(idx, :) <= thresholds(t), 1) - 1; % zero-based K
        line(t) = kHit;
    end
    fprintf('%4.1f %11d %11d %11d %12d\n', betas(idx), line);
end
