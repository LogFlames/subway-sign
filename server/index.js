// Import required modules
const express = require('express');
const bodyParser = require('body-parser');
const fs = require('fs');
const path = require('path');

const app = express();

const SL_STATIONS_URL = "https://transport.integration.sl.se/v1/sites?expand=false";
const SL_LINES_URL = "https://transport.integration.sl.se/v1/lines";
const SL_DEPARTURES_URL_START = "https://transport.integration.sl.se/v1/sites/";
const SL_DEPARTURES_URL_END = "/departures";

const dirname = path.dirname(__filename);


app.use(bodyParser.json()); // To parse incoming JSON requests
app.use(bodyParser.urlencoded({ extended: true })); // To parse URL-encoded data

app.get('/', async (req, res) => {
    let html = fs.readFileSync(path.join(dirname, "config.html")).toString();

    let sites = await (await fetch(SL_STATIONS_URL)).json();

    let lines = await (await fetch(SL_LINES_URL)).json();

    html = html.replace("/* <!-- STATIONS --> */", JSON.stringify(sites));
    html = html.replace("/* <!-- LINES --> */", JSON.stringify(lines));

    res.status(200).send(html);
});

app.post('/sign', (req, res) => {
    res.json({
        message: 'Data received',
        receivedData: data
    });
});

const PORT = 8080;
app.listen(PORT, "127.0.0.1", () => {
    console.log(`Server is running on http://127.0.0.1:${PORT}`);
});

