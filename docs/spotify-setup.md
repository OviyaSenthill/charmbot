# Spotify setup

The Spotify app needs three values in `firmware/include/config.h`:
`SPOTIFY_CLIENT_ID`, `SPOTIFY_CLIENT_SECRET` and `SPOTIFY_REFRESH_TOKEN`.
You only do this once.

1. Go to the [Spotify Developer Dashboard](https://developer.spotify.com/dashboard) and create an app.
   - Redirect URI: `http://127.0.0.1:8888/callback` (Spotify no longer accepts `localhost`)
   - API: tick **Web API**
2. Copy the **Client ID** and **Client Secret** from the app's settings.
3. On your laptop, run:
   ```bash
   python3 tools/spotify_auth.py
   ```
   Paste the ID and secret, log in when the browser opens, and the script prints your
   **refresh token**.
4. Put all three values into `config.h`, then rebuild and upload.

## Notes

- **Play/pause/skip need Spotify Premium.** Now-playing info works on free accounts.
  Without Premium you'll see "Controls need Premium".
- Something has to be playing on *some* device (phone, laptop) for anything to show up.
- Spotify changes its developer rules from time to time. If the dashboard asks for
  something not listed here, follow what it says.
- Never commit `config.h`. It's in `.gitignore` for a reason.
