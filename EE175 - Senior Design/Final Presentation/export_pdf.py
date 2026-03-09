import asyncio
import os
import io
from pathlib import Path
from playwright.async_api import async_playwright
from PIL import Image

async def generate_pdf(html_path, output_pdf):
    file_url = Path(html_path).resolve().as_uri()
    
    print(f"Loading {file_url}...")
    
    async with async_playwright() as p:
        browser = await p.chromium.launch(headless=True)
        # Use a high deviceScaleFactor for retina quality (crisp text)
        page = await browser.new_page(
            viewport={'width': 1600, 'height': 900},
            device_scale_factor=2
        )
        
        await page.goto(file_url, wait_until="networkidle")
        
        print("Waiting for 3D models and animations to settle...")
        await page.wait_for_timeout(5000)
        
        slide_locators = await page.locator('.slide').all()
        total_slides = len(slide_locators)
        print(f"Found {total_slides} slides.")
        
        screenshots = []
        for i in range(total_slides):
            print(f"Processing slide {i+1}/{total_slides}...")
            
            # Wait a moment for any transitions to settle
            await page.wait_for_timeout(800)
            
            # Take screenshot of the currently active slide
            # We screenshot the whole viewport, or the active slide
            active_slide = page.locator('.slide.active')
            screenshot_bytes = await active_slide.screenshot(type="png")
            
            img = Image.open(io.BytesIO(screenshot_bytes))
            
            if img.mode in ('RGBA', 'LA') or (img.mode == 'P' and 'transparency' in img.info):
                bg = Image.new('RGB', img.size, (3, 4, 5))
                bg.paste(img, mask=img.split()[3]) 
                screenshots.append(bg)
            else:
                screenshots.append(img.convert('RGB'))
                
            # Move to next slide
            if i < total_slides - 1:
                await page.keyboard.press("ArrowRight")
                
        await browser.close()
        
        if screenshots:
            print(f"Exporting to '{output_pdf}'...")
            # Save the first image, then append the rest as a multi-page PDF
            screenshots[0].save(
                output_pdf,
                format='PDF',
                resolution=100.0, # Adjusts perceived pixel density in some readers
                save_all=True,
                append_images=screenshots[1:]
            )
            print(f"Successfully generated {output_pdf}")
        else:
            print("No slides found to convert.")

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.abspath(__file__))
    html_file = os.path.join(script_dir, "slides.html")
    pdf_file = os.path.join(script_dir, "BMS_Final_Design_Presentation.pdf")
    asyncio.run(generate_pdf(html_file, pdf_file))
