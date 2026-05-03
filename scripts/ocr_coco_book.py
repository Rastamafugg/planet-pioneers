"""One-shot OCR for docs/reference/Assembly Language Programming for the Color Computer.pdf."""
import pytesseract
from pdf2image import convert_from_path
from pypdf import PdfReader

PDF = r"docs/reference/Assembly Language Programming for the Color Computer.pdf"
OUT = r"docs/reference/Assembly Language Programming for the Color Computer.md"

POPPLER = r"C:\Users\19029\AppData\Local\Microsoft\WinGet\Packages\oschwartz10612.Poppler_Microsoft.Winget.Source_8wekyb3d8bbwe\poppler-25.07.0\Library\bin"
pytesseract.pytesseract.tesseract_cmd = r"C:\Program Files\Tesseract-OCR\tesseract.exe"

with open(OUT, "w", encoding="utf-8") as f:
    f.write("# Assembly Language Programming for the Color Computer\n\n")
    f.write("_OCR-extracted text. Source: docs/reference/Assembly Language Programming for the Color Computer.pdf_\n\n")

    CHUNK = 5
    n = len(PdfReader(PDF).pages)
    print(f"Total pages: {n}", flush=True)

    for start in range(1, n + 1, CHUNK):
        end = min(start + CHUNK - 1, n)
        print(f"  Pages {start}-{end}...", flush=True)
        images = convert_from_path(PDF, dpi=300, first_page=start, last_page=end, poppler_path=POPPLER)
        for i, img in enumerate(images):
            page_no = start + i
            text = pytesseract.image_to_string(img)
            f.write(f"\n\n---\n\n## Page {page_no}\n\n")
            f.write(text.strip())
            f.flush()

print("Done.")
