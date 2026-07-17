# T-Skylten / Subway Sign

T-Skylten is a departure display for Stockholm public transport. It combines a
Node.js web server using SL's Transport API with firmware and enclosure files for
a 64×32 LED matrix sign.

## Features

- Search for an SL station and select lines or directions to display.
- Configure the departure forecast, row limit, and service-deviation messages.
- Show departures in a browser with SL-inspired line colours and a live clock.
- Optionally show **Spår/läge**, using SL's stop-point designation for railway
  platforms and bus stop positions.
- Serve a compact text response that remains compatible with the ESP8266 sign.

## Project structure

- `server/` – Express server, configuration page, and browser display.
- `subway_sign/` – ESP8266 firmware and OpenSCAD enclosure model.

## Run the server

Node.js 22 or later is recommended.

```bash
cd server
npm ci
npm start
```

Open [http://localhost:8080](http://localhost:8080), choose a station and the
lines to include, then use **Öppna display** to launch the web display.

### Docker

```bash
docker build -t subway-sign ./server
docker run --rm -p 8080:8080 subway-sign
```

## Configuration format

The generated sign configuration is a semicolon-separated string:

```text
forecast;siteId;includeDeviations;maxSecondRow;line[:direction];...
```

Example:

```text
120;9001;true;3;10;11:2
```

The physical sign reads `/text?config=...`. The browser uses the structured
`/display-data?config=...` endpoint so it can also render platform and stop
position information. The web-only `showStop=true` query parameter controls the
visibility of the **Spår/läge** column and does not change the firmware format.

## Version

Current version: **1.1.0**

### 1.1.0

- Added the optional Spår/läge column to the web display.
- Added structured web-display departure data from SL's API.
- Preserved the existing physical-sign text protocol.
- Corrected SL departure-filter parameter handling.

### 1.0.0

- Initial server, browser display, ESP8266 firmware, and enclosure design.

## License

ISC
