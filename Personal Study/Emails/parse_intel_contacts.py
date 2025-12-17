import re
import json

def parse_intel_contacts(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Split by the numbering pattern "N. Name"
    # We look for a digit followed by a dot and a space at the start of a line
    entries = re.split(r'^\d+\.\s+', content, flags=re.MULTILINE)[1:] # Skip the first empty split

    contacts = []
    
    for entry in entries:
        lines = entry.strip().split('\n')
        name = lines[0].strip()
        
        job_title = ""
        email = ""
        linkedin = ""
        
        for line in lines[1:]:
            if line.startswith("Job Title:"):
                job_title = line.replace("Job Title:", "").strip()
            elif line.startswith("Email:"):
                email = line.replace("Email:", "").strip()
            elif line.startswith("LinkedIn:"):
                linkedin = line.replace("LinkedIn:", "").strip()
                
        # Name cleanup (handling "Yi-Chin Y" -> try to guess full name from email if needed, 
        # but for now let's keep the name as is or infer from email if last name is initial)
        if len(name.split()[-1]) == 1 and email and "@" in email:
             # Basic inference: yi-chin.yan@intel.com -> Yi-Chin Yan
             # But the user list actually gives names like "Yi-Chin Y"
             # Let's try to extract a better name from email if the provided name ends in an initial
             local_part = email.split('@')[0]
             parts = re.split(r'[._]', local_part)
             if len(parts) >= 2:
                 inferred_first = parts[0].capitalize()
                 inferred_last = parts[-1].capitalize()
                 # If the provided name is close to the inferred one, update it?
                 # Actually, "Yi-Chin Y" vs "yi-chin.yan". Yan is likely the full last name.
                 # Let's just use the name parser from email logic for everyone to be safe and formal, 
                 # or trust the list but expand initials.
                 pass

        contacts.append({
            "name": name,
            "job_title": job_title,
            "email": email,
            "linkedin": linkedin
        })
        
    return contacts

if __name__ == "__main__":
    contacts = parse_intel_contacts(r"C:\Users\kaush\Documents\UCR-Computer-Science-Electrical-Engineering\Personal Study\Emails\intel_batch_input.txt")
    print(json.dumps(contacts, indent=2))
