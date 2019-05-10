var express = require('express');
var bodyParser = require('body-parser');
var app  = express();

app.use(bodyParser.urlencoded({ extended: false }));
app.use(bodyParser.json());

app.post('/data', (req, res) => {
    var data = req.body.data;
    console.log("Got " + data + " from ESP8266");
    res.send("OK\n");
});

app.get('/', (req, res) => {
    res.send("Hello world!\n");
});

var server = app.listen(8081, () => {
    var host = server.address().address;
    var port = server.address().port;

    console.log("ZAP Server listening at http://%s:%s", host, port)
})

