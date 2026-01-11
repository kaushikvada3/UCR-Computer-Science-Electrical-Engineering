
import json
import os

FULL_DATA = 'full_health_data.json'
OUTPUT_DIR = 'dashboard/data'

def split_data():
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)
        
    print(f"Loading {FULL_DATA}...")
    with open(FULL_DATA, 'r') as f:
        data = json.load(f)
        
    print(f"Splitting {len(data)} metrics into {OUTPUT_DIR}...")
    
    for metric_name, entries in data.items():
        # Clean filename
        safe_name = metric_name.lower().replace(' ', '_')
        filename = f"{safe_name}.json"
        
        filepath = os.path.join(OUTPUT_DIR, filename)
        
        # Optimize size: minimal JSON
        # Just [timestamp, value] might be enough if unit is known, 
        # but let's keep it simple: list of {d: date, v: value} to save specific keys
        optimized_entries = []
        for e in entries:
             # Parse date to shorter string or timestamp if needed, but keeping ISO is safer
             optimized_entries.append({'d': e['date'], 'v': e['value']})
             
        with open(filepath, 'w') as out:
            json.dump(optimized_entries, out)
            
    print("Done splitting.")

if __name__ == "__main__":
    split_data()
