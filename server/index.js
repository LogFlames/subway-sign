const express = require('express');
const bodyParser = require('body-parser');
const fs = require('fs');
const path = require('path');

const app = express();

const SL_STATIONS_URL = "https://transport.integration.sl.se/v1/sites?expand=false";
const SL_LINES_URL = "https://transport.integration.sl.se/v1/lines?transport_authority_id=1";

const requests = {};
const cache = {};

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

app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));

setInterval(() => {
    for (req in requests) {
        if (new Date() - requests[req] < 30000) {
            const parts = req.split(";");

            if (parts.length < 4) {
                return;
            }

            const forecast = parts[0];
            const siteId = parts[1];
            const includeDeviations = parts[2] == "true";
            const maxSecondRow = parseInt(parts[3]);
            const includeLines = parts.slice(4);
            console.log(`Updating cache for: ${req}`);
            get_and_update_cache(req, siteId, forecast, maxSecondRow, includeDeviations, includeLines);
        } else {
            delete requests[req]
        }
    }
}, 15000);

app.get('/', async (req, res) => {
    let html = fs.readFileSync(path.join(dirname, "config.html")).toString();

    let sites = await (await fetch(SL_STATIONS_URL)).json();

    let lines = await (await fetch(SL_LINES_URL)).json();

    html = html.replace("/* <!-- STATIONS --> */", JSON.stringify(sites));
    html = html.replace("/* <!-- LINES --> */", JSON.stringify(lines));

    return res.status(200).send(html);
});

app.get('/station', async (req, res) => {
    if (!req.query.site_id) {
        console.error("No site_id specified");
        return res.status(400).send("No site_id specified");
    }

    let sites = await (await fetch(SL_STATIONS_URL)).json();
    sites = sites.filter(site => site.id == req.query.site_id);
    if (sites.length == 0) {
        return res.status(404).send("No station found");
    }

    sites = sites[0];
    return res.status(200).json(sites);
});

app.get('/departures', async (req, res) => {
    let site_id = req.query.site_id;
    let transport = req.query.transport;
    let line_id = req.query.line_id;
    let direction_code = req.query.direction_code;

    let url = SL_DEPARTURES_URL(site_id, transport, line_id, direction_code);
    try {
        let departures = await (await fetch(url)).json();
        return res.json(departures);
    } catch (e) {
        console.error(e);
        return res.json({});
    }
});

app.get('/text', async (req, res) => {
    if (!req.query.config) {
        console.error("No config specified");
        return res.status(400).send("No config specified");
    }

    const parts = req.query.config.split(";");

    if (parts.length < 4) {
        console.error("Invalid config");
        return res.status(400).send("Invalid config");
    }

    try {
        const forecast = parts[0];
        const siteId = parts[1];
        const includeDeviations = parts[2] == "true";
        const maxSecondRow = parseInt(parts[3]);
        const includeLines = parts.slice(4);

        const text = await get_or_cache(req.query.config, siteId, forecast, maxSecondRow, includeDeviations, includeLines);
        return res.status(200).type("text/plain").send(text);
    } catch (err) {
        console.error(err);
        return res.status(500).send("Invalid config or unknown error.");
    }
});

app.get("/display", async (req, res) => {
    return res.sendFile(path.join(__dirname, "display.html"));
});

async function get_or_cache(query, siteId, forecast, maxSecondRow, includeDeviations, includeLines) {
    requests[query] = new Date();
    if (query in cache && new Date() - cache[query].lastUpdated < 30000) {
        console.log(`Serving ${query} from cache`);
        return cache[query].result;
    }

    console.log(`No ${query} in cache, pulling.`);
    return await get_and_update_cache(query, siteId, forecast, maxSecondRow, includeDeviations, includeLines);
}

async function get_and_update_cache(query, siteId, forecast, maxSecondRow, includeDeviations, includeLines) {
    let url = SL_DEPARTURES_URL(siteId, forecast);

    var text = "";
    var departures;

    try {
        let result = await fetch(url);
        if (result.ok) {
            departures = await result.json();
        } else {
            console.error(await result.json());
            return "\nUnable to query SL. Invalid config or service is down.\n";
        }
    } catch (e) {
        console.error(e);
        return "\nUnable to query SL. Invalid config or service is down.\n";
    }

    try {
        includeLines = new Set(includeLines);

        const filteredDepartures = departures.departures.filter(departure => {
            if (includeLines.has(`${departure.line.id}`)) {
                return true;
            }

            if (includeLines.has(`${departure.line.id}:${departure.direction_code}`)) {
                return true;
            }

            return false;
        });

        if (filteredDepartures.length > 0) {
            //if (filteredDepartures[0].line.transport_mode == "TRAIN") {
            //    text += `${filteredDepartures[0].stop_point.designation} `;
            //}
            text += `${filteredDepartures[0].line.designation} ${filteredDepartures[0].destination}$${filteredDepartures[0].display}\n`;

            if (filteredDepartures[0].deviations) {
                for (let i = 0; i < filteredDepartures[0].deviations.length; i++) {
                    text += `${filteredDepartures[0].deviations[i].message};`;
                }
            }
        }

        let sr = filteredDepartures.length - 1 > maxSecondRow && maxSecondRow >= 0 ? maxSecondRow + 1 : filteredDepartures.length;
        for (let i = 1; i < sr; i++) {
            const departure = filteredDepartures[i];
            //if (departure.line.transport_mode == "TRAIN") {
            //    text += `${departure.stop_point.designation} `;
            //}
            text += `${departure.line.designation} ${departure.destination}$${departure.display};`;
        }

        if (includeDeviations) {
            departures.stop_deviations.forEach(stop_deviation => {
                text += `${stop_deviation.message};`;
            });
        }

        text += "\n";
        cache[query] = { result: text, lastUpdated: new Date() };
        return text;
    } catch (e) {
        console.error(e);
        return "Error parsing data from SL API.\n";
    }
}

const PORT = 8080;
const IP = "0.0.0.0";
app.listen(PORT, IP, () => {
    console.log(`Server is running on http://${IP}:${PORT}`);
});


