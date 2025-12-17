import re

def main():
    input_path = r"C:\Users\kaush\Documents\UCR-Computer-Science-Electrical-Engineering\Personal Study\Emails\intel_batch_input.txt"
    output_path = r"C:\Users\kaush\Documents\UCR-Computer-Science-Electrical-Engineering\Personal Study\Emails\intel_linkedin_details.txt"
    
    with open(input_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Regex to find blocks. 
    # Current format seems to be:
    # 1. Name
    # 
    # Job Title: ...
    # 
    # Email: ...
    # 
    # LinkedIn: ...
    
    # We can split by double newlines or just iterate lines.
    lines = content.split('\n')
    
    contacts = []
    current_contact = {}
    
    for line in lines:
        line = line.strip()
        if not line:
            continue
            
        # Check for start of new contact (Number followed by Name) "1. Name"
        if re.match(r'^\d+\.\s+', line):
            if current_contact:
                contacts.append(current_contact)
            current_contact = {'Name': line.split('.', 1)[1].strip()}
        
        elif line.startswith("LinkedIn:"):
            current_contact['LinkedIn'] = line.split('LinkedIn:', 1)[1].strip()
            
    if current_contact:
        contacts.append(current_contact)
        
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write("Intel Contact LinkedIn Profiles\n")
        f.write("===============================\n\n")
        for c in contacts:
            name = c.get('Name', 'Unknown')
            linkedin = c.get('LinkedIn', 'Not Found')
            f.write(f"Name: {name}\nLinkedIn: {linkedin}\n\n")
            
    print(f"Extracted {len(contacts)} profiles to {output_path}")

if __name__ == "__main__":
    main()
