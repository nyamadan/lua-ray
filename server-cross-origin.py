#!/usr/bin/env python3
import sys
import signal
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

        # SIGTERM/SIGINT を受けたら shutdown を呼ぶ
        def handle_signal(signum, frame):
            print(f"Received signal {signum}, shutting down...")
            httpd.shutdown()

        signal.signal(signal.SIGTERM, handle_signal)
        signal.signal(signal.SIGINT, handle_signal)

        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            # 念のため KeyboardInterrupt でも終了
            httpd.shutdown()


if __name__ == "__main__":
    port_arg = int(sys.argv[1]) if len(sys.argv) > 1 else 8000
    run_server(port=port_arg)
