#!/bin/bash
cd "$(dirname "$0")"
python3 -c '
import http.server

class H(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        super().end_headers()

print("Visit this link to run the example: http://localhost:8080/mkfw_run_example.html")
http.server.HTTPServer(("", 8080), H).serve_forever()
'
