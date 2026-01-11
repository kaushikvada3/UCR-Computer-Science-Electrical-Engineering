
import xml.etree.ElementTree as ET
import json
import os
from datetime import datetime
from collections import defaultdict
import matplotlib.pyplot as plt
import matplotlib.dates as mdates

# Configuration
XML_FILE = 'export.xml'
OUTPUT_JSON = 'full_health_data.json'
PLOTS_DIR = 'report/plots'

def parse_date(date_str):
    try:
        return datetime.strptime(date_str, '%Y-%m-%d %H:%M:%S %z')
    except ValueError:
        return None

def clean_type_name(type_str):
    # Remove HKQuantityTypeIdentifier / HKCategoryTypeIdentifier prefixes
    if type_str.startswith("HKQuantityTypeIdentifier"):
        return type_str.replace("HKQuantityTypeIdentifier", "")
    if type_str.startswith("HKCategoryTypeIdentifier"):
        return type_str.replace("HKCategoryTypeIdentifier", "")
    if type_str.startswith("HKDataType"):
        return type_str.replace("HKDataType", "")
    return type_str

def process_health_data(xml_file):
    print(f"Processing {xml_file} (Exhaustive Mode)...")
    
    # Structure: metrics[type_name] = [ {date, value, unit, source} ]
    all_metrics = defaultdict(list)
    
    context = ET.iterparse(xml_file, events=('end',))
    
    count = 0
    for event, elem in context:
        if elem.tag == 'Record':
            record_type = elem.get('type')
            clean_name = clean_type_name(record_type)
            
            start_date = elem.get('startDate')
            value = elem.get('value')
            unit = elem.get('unit', '')
            
            # Try to convert value to float if possible
            try:
                numeric_value = float(value)
            except (ValueError, TypeError):
                numeric_value = value # Keep as string if not numeric (e.g. sleep analysis values)

            # Standardize date
            dt = parse_date(start_date)
            if dt:
                date_str = dt.strftime('%Y-%m-%d %H:%M:%S')
                
                entry = {
                    'date': date_str,
                    'value': numeric_value,
                    'unit': unit
                }
                
                all_metrics[clean_name].append(entry)

            elem.clear()
            count += 1
            if count % 100000 == 0:
                print(f"Processed {count} records...")
                
    print(f"Finished parsing. Found {len(all_metrics)} distinct metric types.")
    return all_metrics

def generate_statistics(metrics):
    stats = {}
    for name, data in metrics.items():
        # Filter for numeric data only for stats
        numeric_values = [d['value'] for d in data if isinstance(d['value'], (int, float))]
        
        if not numeric_values:
            stats[name] = {
                'count': len(data),
                'type': 'categorical/text',
                'first_record': data[0]['date'] if data else None,
                'last_record': data[-1]['date'] if data else None
            }
            continue

        stats[name] = {
            'count': len(data),
            'min': min(numeric_values),
            'max': max(numeric_values),
            'avg': sum(numeric_values) / len(numeric_values),
            'unit': data[0]['unit'],
            'first_record': data[0]['date'],
            'last_record': data[-1]['date']
        }
    return stats

def generate_plots(metrics):
    if not os.path.exists(PLOTS_DIR):
        os.makedirs(PLOTS_DIR)
        
    print("Generating plots...")
    for name, data in metrics.items():
        # Only plot numeric types with enough data points
        numeric_data = [(datetime.strptime(d['date'], '%Y-%m-%d %H:%M:%S'), d['value']) 
                        for d in data if isinstance(d['value'], (int, float))]
        
        if len(numeric_data) < 2:
            continue
            
        numeric_data.sort(key=lambda x: x[0])
        dates = [x[0] for x in numeric_data]
        values = [x[1] for x in numeric_data]
        
        plt.figure(figsize=(10, 5))
        plt.plot(dates, values, marker='.', linestyle='none', markersize=2, alpha=0.5)
        
        plt.title(name)
        plt.xlabel('Date')
        plt.ylabel(data[0]['unit'])
        plt.grid(True, alpha=0.3)
        plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m'))
        plt.gcf().autofmt_xdate()
        
        clean_filename = name.lower().replace(' ', '_') + '.png'
        save_path = os.path.join(PLOTS_DIR, clean_filename)
        plt.savefig(save_path, dpi=100, bbox_inches='tight')
        plt.close()
        # print(f"Saved plot for {name}")

if __name__ == "__main__":
    try:
        data = process_health_data(XML_FILE)
        
        # Save raw-ish data (might be huge, maybe we summarize for dashboard? 
        # User said "every data point", but browser might crash with 1GB JSON.
        # For now, let's save a full version for Python/Report generation
        # and maybe a lighter version for the dashboard later if needed.
        # Actually, let's save the full thing.
        
        with open(OUTPUT_JSON, 'w') as f:
            json.dump(data, f)
        print(f"Saved full data to {OUTPUT_JSON}")

        # Generate Stats
        stats = generate_statistics(data)
        with open('health_stats.json', 'w') as f:
            json.dump(stats, f, indent=2)
        print("Saved statistics to health_stats.json")
        
        # Generate Plots
        generate_plots(data)
        print("Plots generated.")

    except Exception as e:
        print(f"An error occurred: {e}")
        import traceback
        traceback.print_exc()
