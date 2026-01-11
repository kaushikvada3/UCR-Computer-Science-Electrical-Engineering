
import json
import os

STATS_FILE = 'report/medical_stats.json'
OUTPUT_TEX = 'report/medical_report.tex'

CATEGORY_ORDER = [
    'Cardiovascular',
    'Respiratory',
    'Vitals & Body Measurements',
    'Sleep',
    'Activity & Mobility',
    'Nutrition',
    'Audio & Environmental',
    'Other'
]

WINDOW_ORDER = ['1W', '1M', '3M', '6M', '1Y', 'All']

def load_stats():
    with open(STATS_FILE, 'r') as f:
        return json.load(f)

def escape_latex(text):
    if text is None: return "N/A"
    return str(text).replace('_', r'\_').replace('%', r'\%').replace('&', r'\&').replace('#', r'\#')

def generate_tex(stats):
    tex_content = r"""
\documentclass{article}
\usepackage[utf8]{inputenc}
\usepackage[landscape, margin=0.5in]{geometry} % Landscape for big tables
\usepackage{booktabs}
\usepackage{graphicx}
\usepackage{hyperref}
\usepackage{float}
\usepackage{xcolor}
\usepackage{longtable}
\usepackage{subcaption}
\usepackage[section]{placeins}
\usepackage{array}

\title{\textbf{EXTREMELY DETAILED MEDICAL HEALTH RECORD}}
\author{Patient Data Analysis}
\date{\today}

\begin{document}

\maketitle

\section*{Analysis Overview}
This report provides a maximum-density statistical breakdown of all health metrics.
It includes detailed statistics (Min, Max, Avg, Start, End, Trend) for \textbf{every time window} available.
Charts are presented in high resolution.

\tableofcontents
\newpage
"""

    by_category = {c: [] for c in CATEGORY_ORDER}
    for metric, data in stats.items():
        cat = data.get('category', 'Other')
        if cat not in by_category:
            by_category['Other'].append(metric)
        else:
            by_category[cat].append(metric)

    for cat in CATEGORY_ORDER:
        metrics = sorted(by_category[cat])
        if not metrics: continue
        
        tex_content += f"\\section{{{escape_latex(cat)}}}\n"
        
        for metric_name in metrics:
            data = stats[metric_name]
            unit = escape_latex(data.get('unit', ''))
            safe_name = escape_latex(metric_name)
            
            tex_content += f"\\subsection{{{safe_name}}}\n"
            
            # --- DETAILED STATS TABLE ---
            tex_content += r"\begin{table}[H]" + "\n"
            tex_content += r"\centering" + "\n"
            tex_content += r"\resizebox{\textwidth}{!}{" + "\n"
            tex_content += r"\begin{tabular}{|l|c|c|c|c|c|c|}" + "\n"
            tex_content += r"\hline" + "\n"
            tex_content += r"\textbf{Window} & \textbf{Min} & \textbf{Max} & \textbf{Avg} & \textbf{Start} & \textbf{End} & \textbf{Trend Slope} \\ \hline" + "\n"
            
            windows = data.get('windows', {})
            
            for w in WINDOW_ORDER:
                if w in windows:
                    info = windows[w]
                    min_v = f"{info['min']:.2f}"
                    max_v = f"{info['max']:.2f}"
                    avg_v = f"{info['avg']:.2f}"
                    start_v = f"{info['start_val']:.2f}"
                    end_v = f"{info['end_val']:.2f}"
                    slope = f"{info['trend_slope']:.5f}"
                    
                    tex_content += f"{w} & {min_v} {unit} & {max_v} {unit} & {avg_v} {unit} & {start_v} & {end_v} & {slope} \\\\ \\hline\n"
                else:
                    tex_content += f"{w} & - & - & - & - & - & - \\\\ \\hline\n"

            tex_content += r"\end{tabular}" + "\n"
            tex_content += r"}" + "\n"
            tex_content += f"\\caption{{Detailed Statistics for {safe_name}}}" + "\n"
            tex_content += r"\end{table}" + "\n"

            # --- PLOTS ---
            # User wants BIG graphs. Let's do 2 per page, full width stacked.
            cat_folder = cat.replace(' ', '_').replace('&', 'and')
            clean_metric_name = metric_name.lower().replace(' ', '_')
            
            plot_found = False
            for w in WINDOW_ORDER:
                filename = f"{clean_metric_name}_{w}.png"
                path = f"plots_medical/{cat_folder}/{filename}"
                
                if os.path.exists(f"report/{path}"):
                    plot_found = True
                    tex_content += r"\begin{figure}[H]" + "\n"
                    tex_content += r"\centering" + "\n"
                    # Full width image
                    tex_content += f"\\includegraphics[width=0.95\\textwidth]{{{path}}}\n"
                    # tex_content += f"\\caption{{{safe_name} - {w}}}" + "\n"
                    tex_content += r"\end{figure}" + "\n"
            
            if not plot_found:
                tex_content += "\\textit{No charts available (insufficient data).}\n"

            tex_content += r"\clearpage" + "\n"

    tex_content += r"\end{document}"
    return tex_content

if __name__ == "__main__":
    if not os.path.exists(STATS_FILE):
        print("Stats file not found.")
        exit(1)
        
    stats = load_stats()
    tex = generate_tex(stats)
    
    with open(OUTPUT_TEX, 'w') as f:
        f.write(tex)
    print(f"Generated {OUTPUT_TEX}")
