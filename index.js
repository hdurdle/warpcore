require('console-stamp')(console, {
    pattern: 'yyyy-mm-dd HH:MM:ss',
    label: false
});

const plexRequest = require('request');
const warpRequest = require('request');

const warpcoreHost = '10.0.0.1'; // host or IP for the device running the Arduino code
const tautulliHost = 'tautulli.local';
const tautulliKey = 'api-key-from-tautulli';
const interval = 5 * 60 * 1000;

var activeStreams;

function checkWarp() {

    plexRequest(`https://${tautulliHost}/api/v2?apikey=${tautulliKey}&cmd=get_activity`, {
        json: true
    }, (err, res) => {
        if (err) {
            return console.log(err);
        } else {
            activeStreams = res.body.response.data.stream_count;

            var uri = `http://${warpcoreHost}/warp`;

            warpRequest(uri, {
                json: true
            }, (err, res, body) => {
                if (err && err.errno != 'ECONNRESET') {
                    return console.log(err);
                } else {

                    var warp = 1;

                    if (activeStreams < 9) {
                        warp = activeStreams;
                    } else if (activeStreams >= 9) {
                        warp = 9;
                    }

                    uri += "/" + warp;

                    console.log(`Streams: ${activeStreams} - setting Warp ${warp}.`);
                    console.log(`Calling ${uri}`);

                    warpRequest(uri, {
                        json: true
                    }, (err, res, body) => {
                        if (err && err.errno != 'ECONNRESET') {
                            return console.log(err);
                        }

                        console.log(`Pausing for ${interval/1000} seconds...`);
                    });

                }

            });

        }

    });

}

checkWarp();
setInterval(checkWarp, interval);