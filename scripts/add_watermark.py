#!/usr/bin/env python3
"""
Add "Preprint" watermark to PDF files
Copyright (C) 2025, Shyamal Suhana Chandra. All rights reserved.
"""

import sys
import os

try:
    from PyPDF2 import PdfReader, PdfWriter
    from reportlab.pdfgen import canvas
    from reportlab.lib.pagesizes import letter, A4
    from reportlab.lib.colors import gray
    from reportlab.lib.utils import ImageReader
    from io import BytesIO
except ImportError:
    try:
        from pypdf import PdfReader, PdfWriter
        from reportlab.pdfgen import canvas
        from reportlab.lib.pagesizes import letter, A4
        from reportlab.lib.colors import gray
        from reportlab.lib.utils import ImageReader
        from io import BytesIO
    except ImportError:
        print("Error: Need PyPDF2/pypdf and reportlab. Install with: pip install PyPDF2 reportlab")
        sys.exit(1)

def create_watermark(text="PREPRINT", angle=45, opacity=0.3):
    """Create a watermark PDF page"""
    packet = BytesIO()
    can = canvas.Canvas(packet, pagesize=A4)
    
    # Set font and size
    can.setFont("Helvetica-Bold", 60)
    can.setFillColor(gray, alpha=opacity)
    
    # Get page dimensions
    width, height = A4
    
    # Rotate and position watermark in center
    can.saveState()
    can.translate(width/2, height/2)
    can.rotate(angle)
    can.drawCentredString(0, 0, text)
    can.restoreState()
    
    can.save()
    packet.seek(0)
    return PdfReader(packet)

def add_watermark(input_pdf, output_pdf, watermark_text="PREPRINT"):
    """Add watermark to all pages of a PDF"""
    try:
        # Read input PDF
        reader = PdfReader(input_pdf)
        writer = PdfWriter()
        
        # Create watermark
        watermark = create_watermark(watermark_text)
        watermark_page = watermark.pages[0]
        
        # Add watermark to each page
        for page in reader.pages:
            page.merge_page(watermark_page)
            writer.add_page(page)
        
        # Write output PDF
        with open(output_pdf, 'wb') as output_file:
            writer.write(output_file)
        
        print(f"✅ Successfully added watermark to {output_pdf}")
        return True
    except Exception as e:
        print(f"❌ Error processing {input_pdf}: {e}")
        return False

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 add_watermark.py <input_pdf> <output_pdf>")
        sys.exit(1)
    
    input_pdf = sys.argv[1]
    output_pdf = sys.argv[2]
    
    if not os.path.exists(input_pdf):
        print(f"❌ Error: Input file {input_pdf} not found")
        sys.exit(1)
    
    success = add_watermark(input_pdf, output_pdf)
    sys.exit(0 if success else 1)

