import io
from flask import Flask, send_file
import os.path

app = Flask(__name__)

@app.route('/firmware.bin')
def firm():
    with open(".pio\\build\\esp-wrover-kit\\firmware.bin", 'rb') as bites:
        print(bites)
        return send_file(
                     io.BytesIO(bites.read()),
                     mimetype='application/octet-stream'
               )

@app.route("/")
def hello():
    return "Hello World!"

# Add a new route to provide version info
@app.route("/version")
def version():
    try:
        # First try to read from versioning file
        with open("versioning", "r") as f:
            build_no = f.read().strip()
        
        # Try to get the full version from version.h if it exists
        if os.path.exists("include/version.h"):
            with open("include/version.h", "r") as f:
                content = f.read()
                import re
                match = re.search(r'VERSION_SHORT\s+"([^"]+)"', content)
                if match:
                    return match.group(1)
        
        # Fallback to constructing version from build number
        return f"v0.1.{build_no}"
    except Exception as e:
        print(f"Error getting version: {e}")
        return "unknown"

if __name__ == '__main__':
    app.run(host='0.0.0.0', ssl_context=('ca_cert.pem', 'ca_key.pem'), debug=True)