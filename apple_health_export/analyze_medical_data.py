
import json
import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime, timedelta
from scipy import stats
import warnings

# Suppress warnings
warnings.filterwarnings("ignore")

INPUT_JSON = 'full_health_data.json'
OUTPUT_DIR = 'report/plots_medical'
STATS_OUTPUT = 'report/medical_stats.json'

CATEGORIES = {
    'Cardiovascular': [
        'HeartRate', 'RestingHeartRate', 'HeartRateVariabilitySDNN', 'VO2Max', 
        'WalkingHeartRateAverage', 'HeartRateRecoveryOneMinute'
    ],
    'Respiratory': [
        'RespiratoryRate', 'OxygenSaturation'
    ],
    'Vitals & Body Measurements': [
        'BodyMass', 'Height', 'BodyTemperature', 'BloodPressureSystolic', 'BloodPressureDiastolic', 'BodyFatPercentage', 'LeanBodyMass', 'WaistCircumference'
    ],
    'Activity & Mobility': [
        'StepCount', 'DistanceWalkingRunning', 'ActiveEnergyBurned', 'AppleExerciseTime', 
        'FlightsClimbed', 'WalkingSpeed', 'WalkingStepLength', 'WalkingAsymmetryPercentage',
        'WalkingDoubleSupportPercentage', 'StairAscentSpeed', 'StairDescentSpeed',
        'RunningSpeed', 'RunningPower', 'RunningStrideLength', 'RunningVerticalOscillation',
        'RunningGroundContactTime', 'SixMinuteWalkTestDistance'
    ],
    'Sleep': [
        'SleepAnalysis', 'SleepDurationGoal'
    ],
    'Audio & Environmental': [
        'HeadphoneAudioExposure', 'EnvironmentalAudioExposure', 'TimeInDaylight', 
        'UnderwaterDepth', 'WaterTemperature'
    ],
    'Nutrition': [
        'DietaryEnergyConsumed', 'DietaryProtein', 'DietaryCarbohydrates', 'DietaryFatTotal', 'DietaryWater'
    ]
}

TIME_WINDOWS = {
    '1W': timedelta(days=7),
    '1M': timedelta(days=30),
    '3M': timedelta(days=90),
    '6M': timedelta(days=180),
    '1Y': timedelta(days=365),
    'All': None
}

def get_category(metric_name):
    for cat, metrics in CATEGORIES.items():
        # loose matching
        for m in metrics:
            if m.lower() in metric_name.lower().replace("identifier", ""):
                 return cat
    return 'Other'

def parse_data():
    print("Loading data...")
    with open(INPUT_JSON, 'r') as f:
        raw_data = json.load(f)
    return raw_data

def analyze_trend(dates, values):
    # Linear Regression
    if len(values) < 2: return None
    
    # Convert dates to ordinal
    x = [d.toordinal() for d in dates]
    y = values

    if len(set(x)) < 2:
        return None
    
    try:
        slope, intercept, r_value, p_value, std_err = stats.linregress(x, y)
    except ValueError:
        return None
    
    return {
        'slope': slope,
        'intercept': intercept,
        'r_squared': r_value**2,
        'direction': 'Increasing' if slope > 0 else 'Decreasing',
        'magnitude': abs(slope * 365) # projected yearly change
    }

def plot_metric(name, data, category):
    # Convert to objects
    ts_data = []
    unit = data[0].get('unit', '')
    
    for d in data:
        try:
            if d['value'] is None: continue
            dt = datetime.strptime(d['date'], '%Y-%m-%d %H:%M:%S')
            val = float(d['value'])
            ts_data.append((dt, val))
        except:
            continue
            
    if not ts_data: return
    
    ts_data.sort(key=lambda x: x[0])
    full_dates = [x[0] for x in ts_data]
    full_values = [x[1] for x in ts_data]
    
    last_date = full_dates[-1]
    
    cat_dir = os.path.join(OUTPUT_DIR, category.replace(' ', '_').replace('&', 'and'))
    if not os.path.exists(cat_dir):
        os.makedirs(cat_dir)
        
    stats_summary = {}

    for window_name, delta in TIME_WINDOWS.items():
        # Slice data
        if delta:
            start_date = last_date - delta
            subset = [(d, v) for d, v in ts_data if d >= start_date]
        else:
            subset = ts_data
            
        if not subset:
            continue
            
        dates = [x[0] for x in subset]
        values = [x[1] for x in subset]
        
        # Plot
        plt.figure(figsize=(10, 6))
        
        # Scatter for actual points
        plt.scatter(dates, values, alpha=0.5, s=15, label='Measurements', color='#6366f1')
        
        # Moving Average (if enough points)
        if len(values) > 10:
            window_size = max(2, int(len(values) * 0.05)) # 5% window
            moving_avg = np.convolve(values, np.ones(window_size)/window_size, mode='valid')
            ma_dates = dates[len(dates)-len(moving_avg):]
            plt.plot(ma_dates, moving_avg, color='#10b981', linewidth=2, label=f'Moving Avg (n={window_size})')
            
        # Linear Regression Trend Line
        trend = analyze_trend(dates, values)
        if trend:
            x_ord = np.array([d.toordinal() for d in dates])
            y_pred = trend['slope'] * x_ord + trend['intercept']
            plt.plot(dates, y_pred, color='#ef4444', linestyle='--', linewidth=1.5, 
                     label=f"Trend: {trend['direction']} (R²={trend['r_squared']:.2f})")
            
            stats_summary[window_name] = {
                'min': min(values),
                'max': max(values),
                'avg': np.mean(values),
                'start_val': values[0],
                'end_val': values[-1],
                'trend_slope': trend['slope']
            }

        plt.title(f"{name} - {window_name}")
        plt.xlabel('Date')
        plt.ylabel(unit)
        plt.legend()
        plt.grid(True, alpha=0.2)
        plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m-%d'))
        plt.gcf().autofmt_xdate()
        
        # Annotate Max/Min
        max_idx = np.argmax(values)
        min_idx = np.argmin(values)
        
        plt.annotate(f'Max: {values[max_idx]:.2f}', xy=(dates[max_idx], values[max_idx]), 
                     xytext=(10, 10), textcoords='offset points', arrowprops=dict(arrowstyle="->"))
                     
        plt.annotate(f'Min: {values[min_idx]:.2f}', xy=(dates[min_idx], values[min_idx]), 
                     xytext=(10, -20), textcoords='offset points', arrowprops=dict(arrowstyle="->"))

        clean_name = name.lower().replace(' ', '_')
        filename = f"{clean_name}_{window_name}.png"
        save_path = os.path.join(cat_dir, filename)
        plt.savefig(save_path, dpi=120, bbox_inches='tight')
        plt.close()
        
    return stats_summary

def main():
    raw_data = parse_data()
    
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)
        
    medical_stats = {}
        
    print(f"Analyzing {len(raw_data)} metrics...")
    
    for metric_name, data in raw_data.items():
        if not data: continue
        
        cat = get_category(metric_name)
        print(f"Processing {metric_name} ({cat})...")
        
        # Skip non-numeric for now usually, but parse_data checks value conversion
        summary = plot_metric(metric_name, data, cat)
        
        if summary:
            medical_stats[metric_name] = {
                'category': cat,
                'unit': data[0].get('unit', ''),
                'windows': summary
            }
            
    with open(STATS_OUTPUT, 'w') as f:
        json.dump(medical_stats, f, indent=2)
        
    print("Analysis Complete.")

if __name__ == "__main__":
    main()
