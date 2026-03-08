import asyncio
from playwright.async_api import async_playwright
from pptx import Presentation
from pptx.util import Inches
import os
import sys
from pathlib import Path

async def generate_pptx(html_path, output_pptx):
    file_url = Path(html_path).resolve().as_uri()
    
    print(f"Loading {file_url}...")
    
    async with async_playwright() as p:
        browser = await p.chromium.launch(headless=True)
        page = await browser.new_page(viewport={'width': 1400, 'height': 8500})
        await page.goto(file_url, wait_until="networkidle")
        
        await page.wait_for_timeout(2000)
        
        prs = Presentation()
        prs.slide_width = Inches(13.3333333333)
        prs.slide_height = Inches(7.5)
        
        blank_slide_layout = prs.slide_layouts[6]

        slide_elements = await page.query_selector_all(".slide")
        print(f"Found {len(slide_elements)} slides.")

        for index, slide_elem in enumerate(slide_elements, start=1):
            print(f"Capturing slide {index}...")
            screenshot_path = f"slide_{index}.png"
            await slide_elem.screenshot(path=screenshot_path)

            slide = prs.slides.add_slide(blank_slide_layout)
            slide.shapes.add_picture(
                screenshot_path,
                0,
                0,
                width=prs.slide_width,
                height=prs.slide_height,
            )
            os.remove(screenshot_path)
        
        await browser.close()
        
        prs.save(output_pptx)
        print(f"\nSuccessfully saved presentation to {output_pptx}")

if __name__ == "__main__":
    if sys.platform == "win32":
        asyncio.set_event_loop_policy(asyncio.WindowsProactorEventLoopPolicy())
    script_dir = os.path.dirname(os.path.abspath(__file__))
    html_file = os.path.join(script_dir, "slides.html")
    pptx_file = os.path.join(script_dir, "BMS_Final_Design_Presentation.pptx")
    asyncio.run(generate_pptx(html_file, pptx_file))
