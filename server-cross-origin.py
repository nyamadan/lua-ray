#!/usr/bin/env python3
# Based on:
# https://gist.github.com/pezcode/d51926bdbadcbd4f22f5a5d2fb8e0394
# https://stackoverflow.com/a/21957017
# https://gist.github.com/HaiyangXu/ec88cbdce3cdbac7b8d5

import sys
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
    url = f"http://{host}:{port}"
    with socketserver.TCPServer((host, port), Handler) as httpd:
        print(f"Server running at {url}")
        httpd.serve_forever()


if __name__ == "__main__":
    port_arg = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    run_server(port=port_arg)