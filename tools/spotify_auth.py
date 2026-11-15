#!/usr/bin/env python3
"""One-time helper: get a Spotify refresh token for Charmbot.

Usage: python3 tools/spotify_auth.py
Prereq: a Spotify developer app with redirect URI http://127.0.0.1:8888/callback
No extra packages needed (standard library only).
"""
import base64
import json
import secrets
import urllib.parse
import urllib.request
import webbrowser
from http.server import BaseHTTPRequestHandler, HTTPServer

REDIRECT_URI = "http://127.0.0.1:8888/callback"
SCOPES = "user-read-currently-playing user-read-playback-state user-modify-playback-state"


def main():
    client_id = input("Client ID: ").strip()
    client_secret = input("Client secret: ").strip()
    state = secrets.token_urlsafe(16)
    result = {}

    class Handler(BaseHTTPRequestHandler):
        def do_GET(self):
            url = urllib.parse.urlparse(self.path)
            if url.path != "/callback":
                self.send_response(404)
                self.end_headers()
                return
            params = urllib.parse.parse_qs(url.query)
            if params.get("state", [""])[0] != state:
                result["error"] = "state mismatch"
            elif "error" in params:
                result["error"] = params["error"][0]
            else:
                result["code"] = params.get("code", [""])[0]
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.end_headers()
            self.wfile.write(b"<h2>Done! You can close this tab and go back to the terminal.</h2>")

        def log_message(self, *args):
            pass

    auth_url = "https://accounts.spotify.com/authorize?" + urllib.parse.urlencode({
        "client_id": client_id,
        "response_type": "code",
        "redirect_uri": REDIRECT_URI,
        "scope": SCOPES,
        "state": state,
    })
    server = HTTPServer(("127.0.0.1", 8888), Handler)
    print("\nOpening your browser to log in to Spotify...\nIf it doesn't open, visit:\n" + auth_url + "\n")
    webbrowser.open(auth_url)
    while not result:
        server.handle_request()

    if "error" in result:
        raise SystemExit("Authorization failed: " + result["error"])

    basic = base64.b64encode(f"{client_id}:{client_secret}".encode()).decode()
    req = urllib.request.Request(
        "https://accounts.spotify.com/api/token",
        data=urllib.parse.urlencode({
            "grant_type": "authorization_code",
            "code": result["code"],
            "redirect_uri": REDIRECT_URI,
        }).encode(),
        headers={"Authorization": "Basic " + basic,
                 "Content-Type": "application/x-www-form-urlencoded"},
    )
    with urllib.request.urlopen(req) as resp:
        tokens = json.load(resp)

    print("Success! Put this in firmware/include/config.h:\n")
    print(f'#define SPOTIFY_REFRESH_TOKEN  "{tokens["refresh_token"]}"')


if __name__ == "__main__":
    main()
