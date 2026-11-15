# Phone upload page (coming in v0.3)

This folder will hold the small web page the ESP32 serves so you can send text and
photos from your phone. See `docs/roadmap.md`.

Plan:
- `index.html`: text box + photo picker
- Photos are resized to 320×240 and converted to JPEG **in the phone's browser**
  (canvas API) before upload, so the ESP32 only ever handles small files.
