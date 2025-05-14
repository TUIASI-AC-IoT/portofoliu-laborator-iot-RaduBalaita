import os
from flask import Flask, request, jsonify, abort

app = Flask(__name__)

# --- Configuration ---
# Define the directory where files will be managed.
# IMPORTANT: Create this directory in your project folder.
MANAGED_FILES_DIR = "managed_files"

# Helper function to get the full path of a file
def get_file_path(filename):
    # Basic security: prevent directory traversal attacks
    if ".." in filename or filename.startswith("/"):
        return None
    return os.path.join(MANAGED_FILES_DIR, filename)

# --- Ensure the managed directory exists ---
if not os.path.exists(MANAGED_FILES_DIR):
    os.makedirs(MANAGED_FILES_DIR)
    print(f"Created directory: {MANAGED_FILES_DIR}")

# --- API Endpoints ---

# 1. Listing the contents of the directory
@app.route('/files', methods=['GET'])
def list_directory_contents():
    """Lists all files and subdirectories in the managed directory."""
    try:
        contents = os.listdir(MANAGED_FILES_DIR)
        return jsonify({"directory": MANAGED_FILES_DIR, "contents": contents}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# 2. Listing the contents of a text file specified by name
@app.route('/files/<string:filename>', methods=['GET'])
def get_file_contents(filename):
    """Reads and returns the content of a specified text file."""
    file_path = get_file_path(filename)
    if not file_path:
        return jsonify({"error": "Invalid filename"}), 400

    if not os.path.exists(file_path):
        return jsonify({"error": "File not found"}), 404
    if not os.path.isfile(file_path):
        return jsonify({"error": "Path is not a file"}), 400

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        return jsonify({"filename": filename, "content": content}), 200
    except Exception as e:
        return jsonify({"error": f"Could not read file: {str(e)}"}), 500

# 3. Creating a file specified by name and content
# 4. Creating a file specified by content (using filename from URL here for simplicity)
@app.route('/files/<string:filename>', methods=['POST', 'PUT'])
def create_or_update_file(filename):
    """
    Creates a new file with specified name and content,
    or updates an existing file (PUT will replace, POST could append or create).
    For simplicity, POST here will behave like PUT if the file exists,
    otherwise it creates it.
    """
    file_path = get_file_path(filename)
    if not file_path:
        return jsonify({"error": "Invalid filename"}), 400

    if not request.is_json or 'content' not in request.json:
        return jsonify({"error": "Missing 'content' in JSON payload"}), 400

    content = request.json['content']
    file_exists = os.path.exists(file_path)

    try:
        with open(file_path, 'w', encoding='utf-8') as f: # 'w' overwrites/creates
            f.write(content)

        if request.method == 'POST':
            status_code = 201 if not file_exists else 200 # 201 Created, 200 OK (updated)
            message = "File created successfully" if not file_exists else "File updated successfully"
        else: # PUT
            status_code = 201 if not file_exists else 200 # 201 Created (if didn't exist), 200 OK (replaced)
            message = "File created successfully" if not file_exists else "File replaced successfully"

        return jsonify({"message": message, "filename": filename}), status_code
    except Exception as e:
        return jsonify({"error": f"Could not write to file: {str(e)}"}), 500


# 5. Deleting a file specified by name
@app.route('/files/<string:filename>', methods=['DELETE'])
def delete_file_by_name(filename):
    """Deletes a file specified by its name."""
    file_path = get_file_path(filename)
    if not file_path:
        return jsonify({"error": "Invalid filename"}), 400

    if not os.path.exists(file_path):
        return jsonify({"error": "File not found"}), 404
    if not os.path.isfile(file_path):
        return jsonify({"error": "Path is not a file, cannot delete"}), 400

    try:
        os.remove(file_path)
        return jsonify({"message": "File deleted successfully", "filename": filename}), 200
    except Exception as e:
        return jsonify({"error": f"Could not delete file: {str(e)}"}), 500

# 6. Modifying the content of a file specified by name
# This is effectively handled by the PUT request in create_or_update_file.
# If you want a dedicated endpoint or slightly different behavior (e.g., partial update),
# you could create a new route with the PATCH method.
# For this lab, using PUT for modification (full replacement) is standard.

# --- Run the Flask Application ---
if __name__ == '__main__':
    # Make sure the 'managed_files' directory exists
    if not os.path.exists(MANAGED_FILES_DIR):
        os.makedirs(MANAGED_FILES_DIR)
    app.run(debug=True) # debug=True is helpful for development