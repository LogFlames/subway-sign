// Import required modules
const express = require('express');
const bodyParser = require('body-parser');
const fs = require('fs');
const path = require('path');

const app = express();

const SL_STATIONS_URL = "https://transport.integration.sl.se/v1/sites?expand=false";
const SL_LINES_URL = "https://transport.integration.sl.se/v1/lines?transport_authority_id=1";

function SL_DEPARTURES_URL(site_id, forecast = 1200, transport = null, line_id = null, direction_code = null) {
    let url = "https://transport.integration.sl.se/v1/sites/" + site_id + "/departures";
    const  params = [];
    if (transport) {
        params.push(`transport=${transport}`);
    }
    if (line_id) {
        params.push(`line_id=${line_id}`);
    }
    if (direction_code) {
        params.push(`direction_code=${direction_code}`);
    }
    params.push(`forecast=${forecast}`);

    if (params.length == 0) {
        return url;
    }

    return url + "?" + params.join("&");
}

const dirname = path.dirname(__filename);


app.use(bodyParser.json()); // To parse incoming JSON requests
app.use(bodyParser.urlencoded({ extended: true })); // To parse URL-encoded data

app.get('/', async (req, res) => {
    let html = fs.readFileSync(path.join(dirname, "config.html")).toString();

    let sites = await (await fetch(SL_STATIONS_URL)).json();

    let lines = await (await fetch(SL_LINES_URL)).json();

    html = html.replace("/* <!-- STATIONS --> */", JSON.stringify(sites));
    html = html.replace("/* <!-- LINES --> */", JSON.stringify(lines));

    return res.status(200).send(html);
});

app.get('/departures', async (req, res) => {
    let site_id = req.query.site_id;
    let transport = req.query.transport;
    let line_id = req.query.line_id;
    let direction_code = req.query.direction_code;

    let url = SL_DEPARTURES_URL(site_id, transport, line_id, direction_code);
    let departures = await (await fetch(url)).json();
    return res.json(departures);
});

app.get('/text', async (req, res) => {
    if (!req.query.config) {
        return res.status(400).send("No config specified");
    }

    const parts = req.query.config.split(";");

    if (parts.length < 3) {
        return res.status(400).send("Invalid config");
    }

    const forecast = parts[0];
    const siteId = parts[1];

    if (!siteId || !forecast) {
        return res.status(400).send("Missing forecast or site id");
    }

    const includeDeviations = parts[2] == "true";
    const includeLines = parts.slice(3);

    let url = SL_DEPARTURES_URL(siteId, forecast);
    let departures = await (await fetch(url)).json();

    const filteredDepartures = departures.departures.filter(departure => {
        if (includeLines.indexOf(`${departure.line.id}`) != -1) {
            return true;
        }

        if (includeLines.indexOf(`${departure.line.id}:${departure.direction_code}`) != -1) {
            return true;
        }

        return false;
    });

    let text = "";

    if (filteredDepartures.length > 0) {
        text += `${filteredDepartures[0].line.designation} ${filteredDepartures[0].destination}$${filteredDepartures[0].display}\n`;

        if (filteredDepartures[0].deviations) {
            for (let i = 0; i < filteredDepartures[0].deviations.length; i++) {
                text += `${filteredDepartures[0].deviations[i].message};`;
            }
        }
    }

    for (let i = 1; i < filteredDepartures.length; i++) {
        const departure = filteredDepartures[i];
        text += `${departure.line.designation} ${departure.destination}$${departure.display};`;
    }

    if (includeDeviations) {
        departures.stop_deviations.forEach(stop_deviation => {
            text += `${stop_deviation.message};`;
        });
    }

    text += "\n";

    return res.status(200).type("text/plain").send(text);
});

const PORT = 8080;
const IP = "0.0.0.0";
app.listen(PORT, IP, () => {
    console.log(`Server is running on http://${IP}:${PORT}`);
});


