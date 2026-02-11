#!/usr/bin/env python3
import sys
import os
import socketserver
from http.server import SimpleHTTPRequestHandler

class Handler(SimpleHTTPRequestHandler):
    extensions_map = {
        "": "application/octet-stream",
        ".css": "text/css",
        ".html": "text/html",
        ".jpg": "image/jpg",
        ".js": "application/x-javascript",
        ".json": "application/json",
        ".manifest": "text/cache-manifest",
        ".png": "image/png",
        ".wasm": "application/wasm",
        ".xml": "application/xml",
    }

    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        super().end_headers()


def run_server(host="localhost", port=8000):
    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)

    url = f"http://{host}:{port}"
    socketserver.TCPServer.allow_reuse_address = True

    with socketserver.TCPServer((host, port), Handler) as httpd:
        print(f"Server running at {url}")
        print(f"Serving directory: {script_dir}")
        print("Press Ctrl+C to stop.")

        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nShutting down server...")
            httpd.shutdown()
            httpd.server_close()
            sys.exit(0)

if __name__ == "__main__":
    port_arg = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    run_server(port=port_arg)