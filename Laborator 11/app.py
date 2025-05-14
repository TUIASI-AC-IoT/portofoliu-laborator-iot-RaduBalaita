import os
from flask import Flask, request, jsonify, abort

app = Flask(__name__)

MANAGED_FILES_DIR = "managed_files"

def get_file_path(filename):
    if ".." in filename or filename.startswith("/"):
        return None
    return os.path.join(MANAGED_FILES_DIR, filename)

if not os.path.exists(MANAGED_FILES_DIR):
    os.makedirs(MANAGED_FILES_DIR)
    print(f"Created directory: {MANAGED_FILES_DIR}")

@app.route('/files', methods=['GET'])
def list_directory_contents():
    """Lists all files and subdirectories in the managed directory."""
    try:
        contents = os.listdir(MANAGED_FILES_DIR)
        return jsonify({"directory": MANAGED_FILES_DIR, "contents": contents}), 200
    except Exception as e:
        return jsonify({"error": str(e)}), 500


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

@app.route('/files/<string:filename>', methods=['POST', 'PUT'])
def create_or_update_file(filename):
    file_path = get_file_path(filename)
    if not file_path:
        return jsonify({"error": "Invalid filename"}), 400

    if not request.is_json or 'content' not in request.json:
        return jsonify({"error": "Missing 'content' in JSON payload"}), 400

    content = request.json['content']
    file_exists = os.path.exists(file_path)

    try:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.write(content)

        if request.method == 'POST':
            status_code = 201 if not file_exists else 200
            message = "File created successfully" if not file_exists else "File updated successfully"
        else:
            status_code = 201 if not file_exists else 200
            message = "File created successfully" if not file_exists else "File replaced successfully"

        return jsonify({"message": message, "filename": filename}), status_code
    except Exception as e:
        return jsonify({"error": f"Could not write to file: {str(e)}"}), 500


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

if __name__ == '__main__':
    if not os.path.exists(MANAGED_FILES_DIR):
        os.makedirs(MANAGED_FILES_DIR)
    app.run(debug=True)
